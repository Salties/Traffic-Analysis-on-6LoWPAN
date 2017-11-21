#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#include "tinydtls.h"

#ifndef DEBUG
#define DEBUG DEBUG_PRINT
#endif
#include "net/ip/uip-debug.h"
#include "net/ip/udp-socket.h"

//#include "debug.h"
#include "dtls.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLIPH_LEN])
#define INVALID_COMMAND "INVALID COMMAND\r\n"

static struct udp_socket server_conn;
static dtls_context_t *dtls_context = NULL;

void DtlsServerCb (struct udp_socket *c, void *ptr,
		   const uip_ipaddr_t * source_addr, uint16_t source_port,
		   const uip_ipaddr_t * dest_addr, uint16_t dest_port,
		   const uint8_t * data, uint16_t datalen);


/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
int
get_psk_info(struct dtls_context_t *ctx, const session_t *session,
	     dtls_credentials_type_t type,
	     const unsigned char *id, size_t id_len,
	     unsigned char *result, size_t result_length) {

  struct keymap_t {
    unsigned char *id; size_t id_length;
    unsigned char *key; size_t key_length;
  } psk[3] = {
    { (unsigned char *)"Client_identity", 15,
      (unsigned char *)"secretPSK", 9 },
    { (unsigned char *)"default identity", 16,
      (unsigned char *)"\x11\x22\x33", 3 },
    { (unsigned char *)"\0", 2,
      (unsigned char *)"", 1 }
  };

  if (type != DTLS_PSK_KEY) {
    return 0;
  }

  if (id) {
    int i;
    for (i = 0; i < sizeof(psk)/sizeof(struct keymap_t); i++) {
      if (id_len == psk[i].id_length && memcmp(id, psk[i].id, id_len) == 0) {
	if (result_length < psk[i].key_length) {
	  //dtls_warn("buffer too small for PSK");
	  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
	}
	memcpy(result, psk[i].key, psk[i].key_length);
	return psk[i].key_length;
      }
    }
  }

  return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}

#define SPACE 20
int
ReadSensors (uint8 * data, size_t * plen)
{
  int count, value, size;

  for (count = 0, value = 0; count < SPACE; count++)
    {
      value += random_rand () % 100;
    }
  value /= SPACE;
  size = snprintf ((char *) data, *plen, "READ: %d\r\n", value);
  *plen = size;

  return size;
}

#define MAX_BUF 128
int
DtlsReadCb (struct dtls_context_t *ctx,
		session_t * session, uint8 * data, size_t len)
{
  uint8 buf[MAX_BUF];
  size_t buflen = MAX_BUF;

  if (!strncasecmp ("GET", (char *) data, 3))
    {
      printf ("Received GET\n");
      memset (buf, 0, MAX_BUF);
      ReadSensors (buf, &buflen);
      printf ("Sending: %s", buf);
      dtls_write (ctx, session, buf, buflen);
    }
  else
    {
      dtls_write (ctx, session, (unsigned char *)INVALID_COMMAND,
		  strlen (INVALID_COMMAND));
    }
  return 0;
}

int
DtlsSendCb (struct dtls_context_t *ctx,
	      session_t * session, uint8 * data, size_t len)
{
  struct udp_socket *conn = (struct udp_socket *) dtls_get_app_data (ctx);

  PRINTF ("send to ");
  PRINT6ADDR (&session->addr);
  PRINTF (":%u\n", uip_ntohs(session->port));

  return udp_socket_sendto(conn, data, len, &session->addr, session->port);
}

PROCESS (udp_server_process, "UDP server process");
AUTOSTART_PROCESSES (&udp_server_process);

void
Init ()
{
  static dtls_handler_t cb = {
    .write = DtlsSendCb,
    .read = DtlsReadCb,
    .event = NULL,
#ifdef DTLS_PSK
    .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
  };

  PRINTF ("DTLS server started\n");
  dtls_context = dtls_new_context (&server_conn);

  //Regist a DTLS port.
  udp_socket_register (&server_conn, dtls_context, DtlsServerCb);
  udp_socket_bind (&server_conn, 20220);

//  dtls_set_log_level (DTLS_LOG_DEBUG);

  if (dtls_context)
    dtls_set_handler(dtls_context, &cb);
}

void
DtlsServerCb (struct udp_socket *sock,
	      void *apparg,
	      const uip_ipaddr_t * remoteaddr,
	      uint16_t remoteport,
	      const uip_ipaddr_t * localaddr,
	      uint16_t localport, const uint8_t * data, uint16_t datalen)
{
  session_t session;
  uint8_t databuf[MAX_BUF] = {0};

  PRINTF ("%u bytes received from ", datalen);
  PRINT6ADDR (localaddr);
  PRINTF(":%u.\n", remoteport);

  //Record remote endpoint.
  uip_ipaddr_copy (&session.addr, remoteaddr);
  session.port = remoteport;
  session.size = sizeof (session.addr) + sizeof (session.port);
  //Move data around...(FIXME)
  memcpy(databuf, data, datalen);
  printf("apparg:%d\n",(int)apparg);
  dtls_handle_message ((dtls_context_t *) apparg, &session, databuf, datalen);

  return;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD (udp_server_process, ev, data)
{
  PROCESS_BEGIN ();

  dtls_init ();
  Init ();

  if (!dtls_context)
    {
      //dtls_emerg ("cannot create context\n");
      PROCESS_EXIT ();
    }

  while (1)
    {
      PROCESS_YIELD ();
    }

  PROCESS_END ();
}

/*---------------------------------------------------------------------------*/
