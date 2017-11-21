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
#include "random.h"

#include "dtls.h"
#include "ecc/ecc.h"
#include "prng.h"

#define LEDS_ECC LEDS_RED
#define LEDS_ALIVE LEDS_BLUE
#define LEDS_OFF_HYSTERISIS (RTIMER_SECOND * 5)

#define KEYSIZE 8

uint32_t sk[KEYSIZE] = { 0 };

void PrintInt256(uint32_t * i256)
{
    int i;
    for (i = KEYSIZE; i > 0; i--)
        printf("%08lX ", i256[i - 1]);
    return;
}

/*---------------------------------------------------------------------------*/
PROCESS(prng_testing_process, "PRNG testing process");
AUTOSTART_PROCESSES(&prng_testing_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(prng_testing_process, ev, data)
{
    static int i;
    static int len;

    PROCESS_BEGIN();

    leds_init();
    lpm_set_max_pm(LPM_PM0);

    dtls_new_context(NULL);

    printf("#PRNG test\n");

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(serial_line_event_message);
        len = atoi((char*)data);
        for (i = 0; i < 100; i++) {
            memset(sk, 0, sizeof(sk));
	    dtls_prng((void*)sk, len);
            PrintInt256(sk);
	    printf(",%d\n",len);
        }
    }


    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
