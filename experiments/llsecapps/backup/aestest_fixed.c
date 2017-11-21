#include "contiki.h"
#include "lib/aes-128.h"
#include "sys/rtimer.h"
#include "sys/etimer.h"
#include "lib/random.h"

#include <stdio.h>              /* For printf() */
#include <string.h>

#define AES_KEY_LEN 16
#define AES_BLOCK_LEN 16

#ifndef NROUND
#define NROUND 200
#endif

#ifndef NSAMPLE
#define NSAMPLE 100
#endif

static uint8_t Aes128Key[AES_KEY_LEN] = {
    0x01, 0x23, 0x45, 0x67,
    0x89, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78,
    0x9a, 0xbc, 0xde, 0xf0
};

static const uint8_t fixed_data[AES_BLOCK_LEN] = {
    0xda, 0x39, 0xa3, 0xee,
    0x54, 0x6b, 0x4b, 0x0d,
    0x32, 0x55, 0xbf, 0xef,
    0x95, 0x60, 0x18, 0x90
};

static uint8_t datablock[NROUND][AES_BLOCK_LEN] = { {0} };

void PrintBlock(const char *prefix, const uint8_t * block,
                const char *appendix)
{
    int i;

    printf(prefix);
    for (i = 0; i < AES_KEY_LEN; i++)
        printf("%02x ", block[i]);
    printf(appendix);

    return;
}

void finalise()
{
    int i;
#ifdef CONTIKI_TARGET_CC2538DK
    static char garbage = 0;
    uint8_t *gbg = datablock[0];
#endif
    
    printf("#Finalise.\n");
    
    //Reset plaintext.
    for (i = 0; i < NROUND; i++)
    {
        memcpy(datablock[i], fixed_data, AES_BLOCK_LEN);
    }
    
#ifdef CONTIKI_TARGET_CC2538DK
    //Do garbage operations to flush the cache.(?) For unknown reason this is needed for CC2538,  although it does not have cache. Doing the same on TelosB causes timing error.
    for ( i = 0; i < NROUND * AES_BLOCK_LEN; i++)
	garbage ^= *gbg++;
	
    if(garbage != 0)
	return;
#endif

    return;
}

/*---------------------------------------------------------------------------*/
PROCESS(aestest, "aestest");
AUTOSTART_PROCESSES(&aestest);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(aestest, ev, data)
{
    static int i, j;
    unsigned long start, end;
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    printf("#Fixed AES-128 implementation test for %s.\n", TARGET_NAME);
#ifndef AES_128_CONF
    printf("#Using Contiki software implementation.\n");
#else
    printf("#Using Hardware coprocessor.\n");
#endif
    printf("#Rounds in each sample: %d\n", NROUND);
    printf("#Sample size: %d\n", NSAMPLE);
    printf("#Rtimer clock ticks per second on this platform is : %lu\n",
           (unsigned long) RTIMER_SECOND);
    printf("#datablock address: %u\n", (unsigned int) datablock);

    etimer_set(&periodic_timer, (2 * CLOCK_SECOND));

    //Initialise Data
    for (j = 0; j < NROUND; j++)
        memcpy(datablock[j], fixed_data, AES_BLOCK_LEN);

    for (i = 0; i < NSAMPLE; i++) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);

#ifdef VERBOSE_AESTEST
        printf("#Sample %d/%d\n", i + 1, NSAMPLE);
        PrintBlock("#Key\t: ", Aes128Key, "\n");
        PrintBlock("#Plaintext\t: ", datablock[0], "\n");
#endif

        AES_128.set_key(Aes128Key);

        //Start timing.
        start = RTIMER_NOW();
#ifndef TIMING_FRAMEWORK_TEST
        for (j = 0; j < NROUND; j++) {
            AES_128.encrypt(datablock[j]);
        }
#else
        for (j = 0; j < NROUND * 10; j++) {
            random_rand();
        }
#endif
        end = RTIMER_NOW();

        //Print result.
#ifdef VERBOSE_AESTEST
        PrintBlock("#Ciphertext\t: ", datablock[0], "\n");
        printf("#Round\t: %d\n", NROUND);
        printf("#Start\t: %lu\n", start);
        printf("#End\t: %lu\n", end);
        printf("#Time Elapsed\t:\n");
#endif
        printf("%lu\n", end - start);

        finalise();
    }

    printf("#%d tests done for %s.\n", NSAMPLE, TARGET_NAME);

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
