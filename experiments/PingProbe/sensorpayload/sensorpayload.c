#include "contiki.h"
#include "sys/etimer.h"
#include "sys/timer.h"
#include "sys/ctimer.h"
#include "sys/clock.h"
//#include "watchdog.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/sht11/sht11-sensor.h"
#include "dev/cc2538-sensors.h"
#include "dev/als-sensor.h"
#include "lib/ccm-star.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/rpl/rpl.h"

#include <stdio.h>		/* For printf() */


#define APP_VERSION "8"

#define PERIOD_TIME ((CLOCK_SECOND * 2))

#define UDP_CLIENT_PORT 8775
#define UDP_SERVER_PORT 5688

#define APP_NAME (("Sensorpayload_" APP_VERSION " Process"))

/*---------------------------------------------------------------------------*/
PROCESS(sensorpayload_process, APP_NAME);
AUTOSTART_PROCESSES(&sensorpayload_process);
/*---------------------------------------------------------------------------*/

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

#define MY_AES_128_KEY {\
	0x00, 0x01, 0x02, 0x03,\
	0x04, 0x05, 0x06, 0x07,\
	0x08, 0x09, 0x0A, 0x0B,\
	0x0C, 0x0D, 0x0E, 0x0F\
}

static uint8_t mykey[] = MY_AES_128_KEY;
static uint8_t nonce[16] = {0};

void encrypt(void* data, size_t datalen, uint8_t *key)
{
	CCM_STAR.set_key(key);
	CCM_STAR.ctr(data, datalen, nonce);
	return;
}

static int gettemp()
{
	return cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
}

static int getambientlight()
{
	return als_sensor.value(0);
}

static int getvdd()
{
	return vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
}

inline void Payload()
{
    int data[3] = {0};

    //data[0] = gettemp();
    data[1] = getambientlight();
    data[2] = getvdd();
    printf("Temperature=%d, Ambient Light=%d, VDD=%d\n", data[0], data[1],data[2]);
    encrypt(data, sizeof(data), mykey);
    uip_udp_packet_sendto(client_conn, data, sizeof(data),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));


    return;
}

PROCESS_THREAD(sensorpayload_process, ev, data)
{
    static struct etimer et;


    PROCESS_BEGIN();

    printf("Application Info: %s\n", APP_NAME);

    leds_init();
    
    //Set server IP.
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
	/* new connection with remote host */
    client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
    udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));
	  
    while (1)
      {
	  //Periodically sleep.
	  leds_toggle(LEDS_ALL);
	  etimer_set(&et, ((PERIOD_TIME)));
	  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	  //Do payload.
	  Payload();
      }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
