#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include "dev/slip.h"
#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

static uip_ipaddr_t prefix;
static uint8_t prefix_set;

PROCESS(border_router_process, "Border router process");

AUTOSTART_PROCESSES(&border_router_process);

void request_prefix(void)
{
    /* mess up uip_buf with a dirty request... */
    uip_buf[0] = '?';
    uip_buf[1] = 'P';
    uip_len = 2;
    slip_send();
    uip_len = 0;
}

static void print_local_addresses(void)
{
    int i;
    uint8_t state;

    PRINTA("Server IPv6 addresses:\n");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
      {
	  state = uip_ds6_if.addr_list[i].state;
	  if (uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
	    {
		PRINTA(" ");
		uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
		PRINTA("\n");
	    }
      }
}


PROCESS_THREAD(border_router_process, ev, data)
{
    static struct etimer et;
    //static uip_ipaddr_t addr;

    PROCESS_BEGIN();

    prefix_set = 0;
    NETSTACK_MAC.off(0);

    PROCESS_PAUSE();

    SENSORS_ACTIVATE(button_sensor);

    printf("RPL-Border router started\n");

    while (!prefix_set)
      {
	  etimer_set(&et, CLOCK_SECOND);
	  request_prefix();
	  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      }
    NETSTACK_MAC.off(1);
    print_local_address();
    leds_init();
    while (1)
      {
	  PROCESS_YIELD();
	  if (ev == sensors_event && data == &button_sensor)
	    {
		printf("Initiating global repair\n");
		rpl_repair_root(RPL_DEFAULT_INSTANCE);
		leds_toggle(LEDS_ALL);
	    }
      }
    PROCESS_END();
}
