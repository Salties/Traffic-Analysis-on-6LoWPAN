#include "dummysensor.h"

#include "contiki.h"
#include "sys/etimer.h"
#include "sys/timer.h"
#include "sys/ctimer.h"
#include "sys/clock.h"
#include "watchdog.h"
#include "random.h"
#include "net/ip/uip.h"
#include "simple-udp.h"

//#include "dev/als-sensor.h"

#include <stdio.h>		/* For printf() */

#define PAYLOAD	((100))
#define IDLE	((900))

#ifndef SCALE
#ifdef SECOND
#define SCALE CLOCK_SECOND
#else
#define SCALE 1
#endif
#endif

#define EXPECTED_PAYLOAD ((PAYLOAD * SCALE))
#define EXPECTED_IDLE ((IDLE * SCALE))

#define WATCHDOG_RESET_PERIOD (( CLOCK_SECOND/2 ))

//The expected payload cycle will be SCALER/GRANULARITY seconds.
//#define SCALER 1
//#define GRANULARITY 1

#ifdef LED_INDICATOR
#include "dev/leds.h"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(dummysensor_process, "Dummysensor Process");
AUTOSTART_PROCESSES(&dummysensor_process);
/*---------------------------------------------------------------------------*/
static struct etimer idle_timer;
static struct timer payload_timer;
static struct ctimer resetwd_timer;

static struct simple_udp_connection broadcast_connection;

void MyInit()
{
	simple_udp_register(&broadcast_connection, 10217, NULL, 10217, NULL);

	return;
}

inline void Payload()
{
    static clock_time_t payload_start, payload_end;
    
    static const unsigned int space = 1024, count = 100;
    unsigned int i, sum, avg;
    uip_ipaddr_t addr;

    //Payload begin timeing.
    payload_start = clock_time();

    //Payload
    for( i = 0, sum = 0; i < count; i++)
    {
	sum += random_rand() % space;
    }
    avg = sum / count;
    printf("Average = %d\n", avg);
    uip_create_linklocal_allnodes_mcast(&addr);

    //Broadcast reading.
    simple_udp_sendto(&broadcast_connection, &avg, sizeof(avg), &addr);

    //Payload end timing.
    payload_end = clock_time();
    printf("%d ticks elapsed.\n", (int) (payload_end - payload_start));

    return;
}

//Return a random period which expectation is "expected" in clock ticks.
inline clock_time_t GetPeriod(unsigned int expected)
{
    clock_time_t r =
	(expected == 0 ? 0 : (random_rand() % (2 * expected + 1)));
    return r;
}

static void ResetWd(void *arg)
{
    watchdog_periodic();
    ctimer_reset(&resetwd_timer);
    return;
}

PROCESS_THREAD(dummysensor_process, ev, data)
{
    //static unsigned long long count;
    clock_time_t idle_period, payload_period, remain;
    

    PROCESS_BEGIN();

    printf("Hello, world\n");
    ctimer_set(&resetwd_timer, WATCHDOG_RESET_PERIOD, &ResetWd, NULL);

#ifdef LED_INDICATOR
    leds_init();
#endif

    while (1)
      {
	  //Idle period.
#ifdef RAND_IDLE
	  idle_period = GetPeriod(EXPECTED_IDLE);
#else
	  idle_period = EXPECTED_IDLE;
#endif

#ifdef LED_INDICATOR
	  leds_on(LEDS_BLUE);
#endif
	  printf("Enter sleep for %d ticks(%d seconds)...",
		 (int) idle_period, (int) (idle_period / CLOCK_SECOND));
	  etimer_set(&idle_timer, idle_period);
	  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&idle_timer));
	  printf("Exit.\n");
#ifdef LED_INDICATOR
	  leds_off(LEDS_BLUE);
#endif

	  //Payload period.
#ifdef LED_INDICATOR
	  leds_on(LEDS_RED);
#endif
#ifdef RAND_PAYLOAD
	  payload_period = GetPeriod(EXPECTED_PAYLOAD);
#else
	  payload_period = EXPECTED_PAYLOAD; 
#endif
	  printf("Enter busy for %d ticks(%d seconds)...\n",
		 (int) payload_period,
		 (int) (payload_period / CLOCK_SECOND));
	  timer_set(&payload_timer, payload_period);
	  //Start Payload.
	  Payload();
	  remain = timer_remaining(&payload_timer);
	  printf("Time remained:%d\n", (int)remain);
	  //Busy wait for the remaining time.
	  while (!timer_expired(&payload_timer))
	  {
	      clock_wait(2);
	      watchdog_periodic();
	  }
#ifdef LED_INDICATOR
	  leds_off(LEDS_RED);
#endif
      }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
