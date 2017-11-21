#include "contiki.h"
#include "sys/etimer.h"
#include "sys/timer.h"
#include "sys/ctimer.h"
#include "sys/clock.h"
//#include "watchdog.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/sht11/sht11-sensor.h"
#include "dev/light-sensor.h"

#include <stdio.h>		/* For printf() */


/*---------------------------------------------------------------------------*/
PROCESS(pingload_process, "randpayload Process");
AUTOSTART_PROCESSES(&pingload_process);
/*---------------------------------------------------------------------------*/

static int gettemp()
{
	return ((sht11_sensor.value(SHT11_SENSOR_TEMP)));
}

static int getambientlight()
{
	return light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
}

inline void Payload()
{
    int temperature, ambientlight;

    temperature = gettemp();
    ambientlight = getambientlight();
    printf("Temperature:%d\nAmbient Light:%d\n", temperature, ambientlight);

    return;
}

PROCESS_THREAD(pingload_process, ev, data)
{
    static struct etimer et;


    PROCESS_BEGIN();

    printf("Hello, world\n");

    leds_init();
    SENSORS_ACTIVATE(light_sensor);
    SENSORS_ACTIVATE(sht11_sensor);

    while (1)
      {
	  //Periodically sleep.
	  leds_on(LEDS_BLUE);
	  etimer_set(&et, ((1*CLOCK_SECOND)));
	  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	  leds_off(LEDS_BLUE);
	  //Do payload.
	  leds_on(LEDS_RED);
	  Payload();
	  leds_off(LEDS_RED);
      }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
