/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

//This file is tuned for CC2538.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "contiki.h"
#include "dev/serial-line.h"
#include "lpm.h"
#include "dev/uart1.h"
#include "dev/leds.h"

#include "ecc/ecc.h"

#define LEDS_ECC LEDS_RED
#define LEDS_ALIVE LEDS_BLUE
#define LEDS_OFF_HYSTERISIS (RTIMER_SECOND * 5)

#define KEYSIZE 8

uint32_t sk[KEYSIZE] = { 0 };
uint32_t pkx[KEYSIZE] = { 0 };
uint32_t pky[KEYSIZE] = { 0 };

void PrintInt256(uint32_t * i256)
{
    int i;
    for (i = KEYSIZE; i > 0; i--)
        printf("%08lX ", i256[i - 1]);
    return;
}

void str2hex(char *str, uint32_t * hexval)
{
    int i;
    char *cptr = str;
    char strbuf[9] = { 0 };

    for (i = KEYSIZE; i > 0; i--) {
        //Skip spaces.
        while (*cptr == ' ')
            cptr++;
        if (*cptr == '\0' || *cptr == '\n')
            break;
        strncpy(strbuf, cptr, 8);
        hexval[i - 1] = strtoll(strbuf, NULL, 16);
        cptr += 8;
    }
    return;
}

void EccTiming(char *inputkey)
{
    clock_t start, end;

    str2hex((char *) inputkey, sk);
    printf("#Parsed Secret Key: \t");
    PrintInt256(sk);
    printf("\n");
    //Start timing
    leds_on(LEDS_ECC);
    printf("#Begin ecc_gen_pubkey() on secp256r1...\n");
    start = RTIMER_NOW();
    ecc_gen_pub_key(sk, pkx, pky);
    end = RTIMER_NOW();
    printf("Done.\n");
    leds_off(LEDS_ECC);
    //Print result
    printf("#Secret Key: \t");
    PrintInt256(sk);
    printf("\n");
    printf("#Public Key:\n");
    printf("#\tQ_x:\t");
    PrintInt256(pkx);
    printf("\n");
    printf("#\tQ_y:\t");
    PrintInt256(pky);
    printf("\n");
    printf("#Time elapsed:\n %lu\n", (unsigned long)(end - start));
}

/*---------------------------------------------------------------------------*/
PROCESS(ecc_timing_process, "ECC timing process");
AUTOSTART_PROCESSES(&ecc_timing_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ecc_timing_process, ev, data)
{
    PROCESS_BEGIN();

    //serial_line_init();
    //uart1_set_input(serial_line_input_byte);  
    leds_init();
    lpm_set_max_pm(LPM_PM0);
    
    printf("#ECC Timing\n");

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	printf("#Received line:%s\n", (char *) data);
	EccTiming(data);
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
