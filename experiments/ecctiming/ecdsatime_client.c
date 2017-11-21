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
#include "tinycrypto.h"
#include "numeric.h"
#include "prng.h"

#define HEX 16

#define DTLS_CV_LENGTH (1 + 1 + 2 + 1 + 1 + 1 + 1 + DTLS_EC_KEY_SIZE + 1 + 1 + DTLS_EC_KEY_SIZE)

#define LEDS_ECC_KEY_GEN LEDS_BLUE
#define LEDS_ECDSA_SIGN LEDS_RED

unsigned char signkey[DTLS_EC_KEY_SIZE] = { 0 };
unsigned char secretkey[DTLS_EC_KEY_SIZE] = { 0 };

unsigned char ephe_key_sk[DTLS_EC_KEY_SIZE] = { 0 };
unsigned char ephe_key_pk_x[DTLS_EC_KEY_SIZE] = { 0 };
unsigned char ephe_key_pk_y[DTLS_EC_KEY_SIZE] = { 0 };

dtls_hash_ctx ecdsa_hash_e;

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

void PrintNonce(uint32_t * nonce)
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

void EcdsaPrepareHash(const unsigned char *client_random,
                      size_t client_random_size,
                      const unsigned char *server_random,
                      size_t server_random_size, const unsigned char *pk_x,
                      size_t pk_x_size, const unsigned char *pk_y,
                      size_t pk_y_size, dtls_hash_ctx * data)
{
    dtls_hash_init(data);
    dtls_hash_update(data, client_random, client_random_size);
    dtls_hash_update(data, server_random, server_random_size);
    dtls_hash_update(data, pk_x, pk_x_size);
    dtls_hash_update(data, pk_y, pk_y_size);
    return;
}

static int PrepareCertVerifyEcdh(const dtls_hash_ctx hash_e,
                                 const unsigned char *priv_key)
{
    /* The ASN.1 Integer representation of an 32 byte unsigned int could be
     *      * 33 bytes long add space for that */
    uint8 buf[DTLS_CV_LENGTH + 2];
    uint8 *p;
    uint32_t point_r[9];
    uint32_t point_s[9];
    uint32_t rand[8];
    dtls_hash_ctx hs_hash;
    unsigned char sha256hash[DTLS_HMAC_DIGEST_SIZE];

    clock_t start, end;

    printf("#Begin Certificate Verify...");
    leds_on(LEDS_ECDSA_SIGN);
    start = RTIMER_NOW();
    /* ServerKeyExchange 
     *      *
     *           * Start message construction at beginning of buffer. */
    p = buf;

    memcpy(&hs_hash, &hash_e, sizeof(hash_e));

    dtls_hash_finalize(sha256hash, &hs_hash);

    /* sign the ephemeral and its paramaters */
    dtls_ecdsa_create_sig_hash_timing(priv_key, DTLS_EC_KEY_SIZE,
                                      sha256hash, sizeof(sha256hash),
                                      point_r, point_s, rand);

    p = dtls_add_ecdsa_signature_elem_wrapper(p, point_r, point_s);
    end = RTIMER_NOW();
    leds_off(LEDS_ECDSA_SIGN);
    //Print result.
    printf("Done.\n");
    PrintInt256("#HashedValue: ", sha256hash);
    printf("#(NonceK, Time):\n");
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
        if (ev == PROCESS_EVENT_TIMER) {
            //Perform ServerKeyExchange
            dtls_prng(clientrand, DTLS_RANDOM_LENGTH);
            dtls_prng(serverrand, DTLS_RANDOM_LENGTH);
            //Generate client and server randomness.
            PrintInt256("#ClientRandomness: ", clientrand);
            PrintInt256("#ServerRandomness: ", serverrand);
            //Generate client side ephemeral keys. 
            //Ref: check_server_hellodone():dtls_send_client_key_exchange().
	    printf("#Generating client ephemeral key...");
	    leds_on(LEDS_ECC_KEY_GEN);
            dtls_ecdsa_generate_key(ephe_key_sk, ephe_key_pk_x,
                                    ephe_key_pk_y, DTLS_EC_KEY_SIZE);
	    leds_off(LEDS_ECC_KEY_GEN);
	    printf("Done.\n");
            PrintInt256("#ClientEphemeralSecretKey: ", ephe_key_sk);
            PrintInt256("#ClientEphemeralPublicKey_X: ", ephe_key_pk_x);
            PrintInt256("#ClientEphemeralPublicKey_Y: ", ephe_key_pk_y);
            //Set ECDSA hash value.
            EcdsaPrepareHash(clientrand, DTLS_RANDOM_LENGTH,
                             serverrand, DTLS_RANDOM_LENGTH,
                             ephe_key_pk_x, DTLS_EC_KEY_SIZE,
                             ephe_key_pk_y, DTLS_EC_KEY_SIZE,
                             &ecdsa_hash_e);
            //Prepare client CERTIFICATE_VERIFY
            //Ref: check_server_hellodone():dtls_send_certificate_verify_ecdh().
            PrepareCertVerifyEcdh(ecdsa_hash_e, signkey);

            //PrepareServerKeyExchange(clientrand, serverrand);
            //Reset timer for next run.
            etimer_set(&periodic_timer, 5 * CLOCK_SECOND);
        }
        if (ev == serial_line_event_message) {
            //Update signing key.
            str2hex(data, signkey, DTLS_EC_KEY_SIZE);
            PrintInt256("#SigningKey Updated: ", signkey);
        }
    }
    PROCESS_END();
}
