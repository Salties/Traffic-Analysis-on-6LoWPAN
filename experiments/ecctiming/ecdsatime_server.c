#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "dev/serial-line.h"
#include "lpm.h"
#include "dev/uart1.h"
#include "dev/leds.h"
#include "sys/rtimer.h"

#include "dtls.h"
#include "ecc/ecc.h"
#include "crypto.h"
#include "numeric.h"
#include "prng.h"

#define HEX 16

#define DTLS_SKEXEC_LENGTH (1 + 2 + 1 + 1 + DTLS_EC_KEY_SIZE + DTLS_EC_KEY_SIZE + 1 + 1 + 2 + 70)

#define LEDS_ECC LEDS_RED

unsigned char signkey[DTLS_EC_KEY_SIZE] = { 0 };
unsigned char secretkey[DTLS_EC_KEY_SIZE] = { 0 };

void PrintInt256(const char *title, unsigned char *i256)
{
    int i;

    printf("%s", title);
    for (i = 0; i < DTLS_EC_KEY_SIZE; i++) {
        printf("%02X", i256[i]);
        if ((i + 1) % 4 == 0)
            printf(" ");
     }
    printf("\n");
    return;
}

void PrintNonce(uint32_t *nonce)
{
        int i;

	for (i = 0; i < DTLS_EC_KEY_SIZE / sizeof(uint32_t); i++) {
	    printf("%08lX ", nonce[i]);
	 }
	return;
}

void str2hex(char *str, unsigned char *hexval, size_t len)
{
    int nvalid = 0;
    size_t count;
    char *cptr;
    char cbuf[3] = { 0 };

    for (cptr = str, count = 0; count < len; count++) {
        //Read two hex characters to cbuf.
        for (nvalid = 0; nvalid < 2; nvalid++, cptr++) {
            //Find the one valid character each loop.
            //Skip redundant characters.
            for (; *cptr == ' ' || *cptr == ','; cptr++);
            //Terminate on '\0', ',' or '\n'.
            if (*cptr == '\0' || *cptr == '\n')
                break;
            //Save to cbuf.
            cbuf[nvalid] = *cptr;
        }
        //Convert to integer.
        hexval[count] = strtoul(cbuf, NULL, HEX);
        //Reset buffer.
        memset(cbuf, 0, sizeof(cbuf));
    }

    return;
}


void PrintSigMsg(const unsigned char *client_random,
                 size_t client_random_size,
                 const unsigned char *server_random,
                 size_t server_random_size,
                 const unsigned char *keyx_params, size_t keyx_params_size)
{

    dtls_hash_ctx data;
    unsigned char sha256hash[DTLS_HMAC_DIGEST_SIZE];

    dtls_hash_init(&data);
    dtls_hash_update(&data, client_random, client_random_size);
    dtls_hash_update(&data, server_random, server_random_size);
    dtls_hash_update(&data, keyx_params, keyx_params_size);
    dtls_hash_finalize(sha256hash, &data);
    //Print the hashed plaintext.
    PrintInt256("#HashedPlaintext: ", sha256hash);

    return;
}

static int PrepareServerKeyExchange(unsigned char *clientrand,
                                    unsigned char *serverrand)
{
    /* The ASN.1 Integer representation of an 32 byte unsigned int could be
     * 33 bytes long add space for that */
    uint8 buf[DTLS_SKEXEC_LENGTH + 2];
    uint8 *p;
    uint8 *key_params;
    uint8 *ephemeral_pub_x;
    uint8 *ephemeral_pub_y;
    uint32_t point_r[9];
    uint32_t point_s[9];
    uint32_t rand[8];
    clock_t start, end;

    /* ServerKeyExchange 
     *
     * Start message construction at beginning of buffer. */
    printf("#Begin ServerKeyExchange...");
    leds_on(LEDS_ECC);
    start = RTIMER_NOW();
    p = buf;

    key_params = p;
    /* ECCurveType curve_type: named_curve */
    dtls_int_to_uint8(p, 3);
    p += sizeof(uint8);

    /* NamedCurve namedcurve: secp256r1 */
    dtls_int_to_uint16(p, TLS_EXT_ELLIPTIC_CURVES_SECP256R1);
    p += sizeof(uint16);

    dtls_int_to_uint8(p, 1 + 2 * DTLS_EC_KEY_SIZE);
    p += sizeof(uint8);

    /* This should be an uncompressed point, but I do not have access to the spec. */
    dtls_int_to_uint8(p, 4);
    p += sizeof(uint8);

    /* store the pointer to the x component of the pub key and make space */
    ephemeral_pub_x = p;
    p += DTLS_EC_KEY_SIZE;

    /* store the pointer to the y component of the pub key and make space */
    ephemeral_pub_y = p;
    p += DTLS_EC_KEY_SIZE;

    dtls_ecdsa_generate_key(secretkey,
                            ephemeral_pub_x, ephemeral_pub_y,
                            DTLS_EC_KEY_SIZE);

    /* sign the ephemeral and its paramaters */
    dtls_ecdsa_create_sig_timing(signkey, DTLS_EC_KEY_SIZE,
                          clientrand, DTLS_RANDOM_LENGTH,
                          serverrand, DTLS_RANDOM_LENGTH,
                          key_params, p - key_params, point_r, point_s, rand);

    p = dtls_add_ecdsa_signature_elem_wrapper(p, point_r, point_s);
    end = RTIMER_NOW();
    printf("Done.\n");
    leds_off(LEDS_ECC);

    //Print variables.
    PrintInt256("#SecretKey: ", secretkey);
    PrintInt256("#PublicKeyX: ", ephemeral_pub_x);
    PrintInt256("#PublicKeyY: ", ephemeral_pub_y);
    PrintSigMsg(clientrand, DTLS_RANDOM_LENGTH,
                serverrand, DTLS_RANDOM_LENGTH,
                key_params, p - key_params);
    printf("#(NonceK, Time):\n ");
    PrintNonce(rand);
    printf(", %lu\n", (unsigned long)(end - start));

    return 0;
}

PROCESS(ecdsa_timing_process, "ECDSA timing process");
AUTOSTART_PROCESSES(&ecdsa_timing_process);

PROCESS_THREAD(ecdsa_timing_process, ev, data)
{
    static struct etimer periodic_timer;
    static unsigned char clientrand[DTLS_RANDOM_LENGTH] = { 0 };
    static unsigned char serverrand[DTLS_RANDOM_LENGTH] = { 0 };
    
    PROCESS_BEGIN();
    
    leds_init();
    lpm_set_max_pm(LPM_PM0);
    
    //Read signing key from serial line.
    printf("#Enter the signing key:");
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    str2hex(data, signkey, DTLS_EC_KEY_SIZE);
    PrintInt256("#SigningKey: ", signkey);
  
    etimer_set(&periodic_timer, 5 * CLOCK_SECOND);
    for (;;) {
	PROCESS_YIELD();
        if(ev == PROCESS_EVENT_TIMER)
	{
            //Perform ServerKeyExchange
	    dtls_prng(clientrand, DTLS_RANDOM_LENGTH);
	    dtls_prng(serverrand, DTLS_RANDOM_LENGTH);
	    PrintInt256("#ClientRandomness: ", clientrand);
	    PrintInt256("#ServerRandomness: ", serverrand);
	    PrepareServerKeyExchange(clientrand, serverrand);
	    //Reset timer for next run.
            etimer_set(&periodic_timer, 5 * CLOCK_SECOND);
	}
	if( ev == serial_line_event_message) 
	{
	    //Update signing key.
	    str2hex(data, signkey, DTLS_EC_KEY_SIZE);
	    PrintInt256("#SigningKey Updated: ", signkey);
	}
    }
    PROCESS_END();
}
