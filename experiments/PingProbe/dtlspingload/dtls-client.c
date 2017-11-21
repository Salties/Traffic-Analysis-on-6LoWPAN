/*
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

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "sys/etimer.h"

#include <string.h>

#include "tinydtls.h"

#ifndef DEBUG
#define DEBUG DEBUG_PRINT
#endif
#include "net/ip/uip-debug.h"

#include "debug.h"
#include "dtls.h"
//#include "servreg-hack.h"

#ifdef DTLS_PSK
/* The PSK information for DTLS */
/* make sure that default identity and key fit into buffer, i.e.
 * sizeof(PSK_DEFAULT_IDENTITY) - 1 <= PSK_ID_MAXLEN and
 * sizeof(PSK_DEFAULT_KEY) - 1 <= PSK_MAXLEN
*/

#define PSK_ID_MAXLEN 32
#define PSK_MAXLEN 32
#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
#endif /* DTLS_PSK */

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLIPH_LEN])

#define MAX_PAYLOAD_LEN 120
#define DTLS_SERVICE_ID 217
#define SEND_INTERVAL (15*CLOCK_SECOND)

static struct udp_socket client_conn;
//static struct uip_udp_conn *client_conn;
static dtls_context_t *dtls_context;
static uip_ipaddr_t serverip;
static char buf[200];
static size_t buflen = 0;

static uip_ipaddr_t *
set_global_address (void)
{
    static uip_ipaddr_t ipaddr;
    int i;
    uint8_t state;

    uip_ip6addr (&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid (&ipaddr, &uip_lladdr);
    uip_ds6_addr_add (&ipaddr, 0, ADDR_AUTOCONF);

    printf ("IPv6 addresses: ");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
      {
          state = uip_ds6_if.addr_list[i].state;
          if (uip_ds6_if.addr_list[i].isused &&
              (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
            {
                uip_debug_ipaddr_print (&uip_ds6_if.addr_list[i].ipaddr);
                printf ("\n");
            }
      }

    return &ipaddr;
}

static void
try_send (struct dtls_context_t *ctx, session_t * dst)
{
    int res;
    res = dtls_write (ctx, dst, (uint8 *) buf, buflen);
    if (res >= 0)
      {
          memmove (buf, buf + res, buflen - res);
          buflen -= res;
      }
}

static int
read_from_peer (struct dtls_context_t *ctx,
                session_t * session, uint8 * data, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        PRINTF ("%c", data[i]);
    return 0;
}

static int
send_to_peer (struct dtls_context_t *ctx,
              session_t * session, uint8 * data, size_t len)
{
    struct udp_socket *conn = (struct udp_socket *) dtls_get_app_data (ctx);

    PRINTF ("send to ");
    PRINT6ADDR (&session->addr);
    PRINTF (":%u\n", uip_ntohs (session->port));

    //Sends DTLS payload
    return udp_socket_sendto (conn, data, len, &session->addr, session->port);
}

#ifdef DTLS_PSK
static unsigned char psk_id[PSK_ID_MAXLEN] = PSK_DEFAULT_IDENTITY;
static size_t psk_id_length = sizeof (PSK_DEFAULT_IDENTITY) - 1;
static unsigned char psk_key[PSK_MAXLEN] = PSK_DEFAULT_KEY;
static size_t psk_key_length = sizeof (PSK_DEFAULT_KEY) - 1;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

//This function is the "key store" for tinyDTLS. It is called to
//retrieve a key for the given identity within this particular
//session.
static int
get_psk_info (struct dtls_context_t *ctx UNUSED_PARAM,
              const session_t * session UNUSED_PARAM,
              dtls_credentials_type_t type,
              const unsigned char *id, size_t id_len,
              unsigned char *result, size_t result_length)
{
    switch (type)
      {
      case DTLS_PSK_IDENTITY:
          if (result_length < psk_id_length)
            {
                dtls_warn ("cannot set psk_identity -- buffer too small\n");
                return dtls_alert_fatal_create (DTLS_ALERT_INTERNAL_ERROR);
            }

          memcpy (result, psk_id, psk_id_length);
          return psk_id_length;
      case DTLS_PSK_KEY:
          if (id_len != psk_id_length || memcmp (psk_id, id, id_len) != 0)
            {
                dtls_warn ("PSK for unknown id requested, exiting\n");
                return dtls_alert_fatal_create (DTLS_ALERT_ILLEGAL_PARAMETER);
            }
          else if (result_length < psk_key_length)
            {
                dtls_warn ("cannot set psk -- buffer too small\n");
                return dtls_alert_fatal_create (DTLS_ALERT_INTERNAL_ERROR);
            }

          memcpy (result, psk_key, psk_key_length);
          return psk_key_length;
      default:
          dtls_warn ("unsupported request type: %d\n", type);
      }

    return dtls_alert_fatal_create (DTLS_ALERT_INTERNAL_ERROR);
}
#endif /* DTLS_PSK */

PROCESS (udp_server_process, "UDP server process");
AUTOSTART_PROCESSES (&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
dtls_handle_read (dtls_context_t * ctx)
{
    static session_t session;

    if (uip_newdata ())
      {
          uip_ipaddr_copy (&session.addr, &UIP_IP_BUF->srcipaddr);
          session.port = UIP_UDP_BUF->srcport;
          session.size = sizeof (session.addr) + sizeof (session.port);

          ((char *) uip_appdata)[uip_datalen ()] = 0;
          PRINTF ("Client received message from ");
          PRINT6ADDR (&session.addr);
          PRINTF (":%d\n", uip_ntohs (session.port));

          dtls_handle_message (ctx, &session, uip_appdata, uip_datalen ());
      }
}

/*---------------------------------------------------------------------------*/
static void
print_local_addresses (void)
{
    int i;
    uint8_t state;

    PRINTF ("Client IPv6 addresses: ");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
      {
          state = uip_ds6_if.addr_list[i].state;
          if (uip_ds6_if.addr_list[i].isused &&
              (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
            {
                PRINT6ADDR (&uip_ds6_if.addr_list[i].ipaddr);
                PRINTF ("\n");
            }
      }
}

static void
set_connection_address (uip_ipaddr_t * ipaddr)
{
#ifdef SERVREG
    uip_ipaddr_t *srvaddr;
    
    //Lookup Server's IP
    do
      {
          srvaddr = servreg_hack_lookup (DTLS_SERVICE_ID);
          if(!srvaddr)
           PRINTF("Failed to look up server ip.\n");
      }
    while (!srvaddr);
    printf("Server IP:");
    uip_debug_ipaddr_print(srvaddr);
    printf("\n");
    uip_ip6addr_copy(ipaddr, srvaddr);
    return;
#else
	uip_ip6addr (ipaddr, 0xaaaa, 0, 0, 0, 0x200, 0x0, 0x0, 0x1);
#endif
	return;
}

void
Init (session_t * dst)
{
    static dtls_handler_t cb = {
        .write = send_to_peer,
        .read = read_from_peer,
        .event = NULL,
#ifdef DTLS_PSK
        .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
    };

    PRINTF ("DTLS client started\n");

    print_local_addresses ();
    set_connection_address (&serverip);
    printf("Server IP:");
    uip_debug_ipaddr_print(&serverip);
    printf("\n");

    dtls_set_log_level (DTLS_LOG_DEBUG);

    dtls_context = dtls_new_context (&client_conn);
    udp_socket_register (&client_conn, NULL, NULL);
    udp_socket_bind (&client_conn, 0);
    udp_socket_connect(&client_conn, &serverip, 20220);
    uip_ipaddr_copy (&dtls_context->addr, &serverip);
    dtls_context->port = 20220;
    dtls_context->size = sizeof (dtls_context->addr) + sizeof (dtls_context->port);
    
    if (dtls_context)
        dtls_set_handler (dtls_context, &cb);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD (udp_server_process, ev, data)
{
    static int connected = 0;
    static session_t dst;
    static struct etimer periodtimer;
    
    PROCESS_BEGIN ();

	set_global_address();
 
    dtls_init();
    Init(&dst);
    //serial_line_init();
    //servreg_hack_init();

    if (!dtls_context)
      {
          dtls_emerg ("cannot create context\n");
          PROCESS_EXIT();
      }

	etimer_set(&periodtimer, SEND_INTERVAL);

    while (1)
      {
          PROCESS_YIELD ();
          if (ev == tcpip_event)
            {
				printf("Packet received.\n");
                dtls_handle_read (dtls_context);
            }

          if (etimer_expired(&periodtimer))
            {
				printf("Timer expired.\n");
                if (!connected)
                    connected = dtls_connect (dtls_context, &dst) >= 0;
                try_send (dtls_context, &dst);
                etimer_reset(&periodtimer);
            }
      }

    PROCESS_END ();
}

/*---------------------------------------------------------------------------*/
