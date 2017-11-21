#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip-debug.h"
#include "net/ip/udp-socket.h"
#include "clock.h"
#include "watchdog.h"

#ifdef SERVER_RPL_DAG
#include "net/rpl/rpl.h"
#endif

#include <string.h>
#include "tinydtls.h"
#include "debug.h"
#include "dtls.h"

#ifndef DEBUG
#define DEBUG DEBUG_PRINT
#endif

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLIPH_LEN])
#define INVALID_COMMAND "INVALID COMMAND\r\n"
#define MAX_PAYLOAD_LEN 120
#define MAX_BUF 128

//Enable NULL_REPLY to measure the base response time.
#define NULL_REPLY 0
//Enable to use alternative ReadSensors().
#define READ_SENSORS_ADD 0
#define READ_SENSORS_MUL 0
#define READ_SENSORS_MOD 1

#define SAMPLING_SPACE 10000

static struct udp_socket server_conn;
static dtls_context_t *dtls_context = NULL;

#ifdef SERVER_RPL_DAG
static void create_rpl_dag(uip_ipaddr_t * ipaddr)
{

    struct uip_ds6_addr *root_if;

    root_if = uip_ds6_addr_lookup(ipaddr);
    if (root_if != NULL) {
        rpl_dag_t *dag;
        uip_ipaddr_t prefix;

        rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
        dag = rpl_get_any_dag();
        uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
        rpl_set_prefix(dag, &prefix, 64);
        PRINTF("created a new RPL dag\n");
    } else {
        PRINTF("failed to create a new RPL DAG\n");
    }
    return;
}

static void set_global_address(void)
{
    static uip_ipaddr_t ipaddr;
    int i;
    uint8_t state;

    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

    printf("IPv6 addresses: ");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++) {
        state = uip_ds6_if.addr_list[i].state;
        if (uip_ds6_if.addr_list[i].isused &&
            (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
            uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
            printf("\n");
        }
    }

    return &ipaddr;
}

static uip_ipaddr_t *ipaddr;
#endif

static void DtlsServerCb(struct udp_socket *, void *, const uip_ipaddr_t *,
                         uint16_t, const uip_ipaddr_t *, uint16_t,
                         const uint8_t *, uint16_t);

//This function is the "key store" for tinyDTLS. It is called to
//retrieve a key for the given identity within this particular
//session.
static int get_psk_info(struct dtls_context_t *ctx,
                        const session_t * session,
                        dtls_credentials_type_t type,
                        const unsigned char *id, size_t id_len,
                        unsigned char *result, size_t result_length)
{
    struct keymap_t {
        unsigned char *id;
        size_t id_length;
        unsigned char *key;
        size_t key_length;
    } psk[3] = {
        {
        (unsigned char *) "Client_identity", 15,
                (unsigned char *) "secretPSK", 9}, {
        (unsigned char *) "default identity", 16,
                (unsigned char *) "\x11\x22\x33", 3}, {
        (unsigned char *) "\0", 2, (unsigned char *) "", 1}
    };

    if (type != DTLS_PSK_KEY) {
        return 0;
    }

    if (id) {
        int i;
        for (i = 0; i < sizeof(psk) / sizeof(struct keymap_t); i++) {
            if (id_len == psk[i].id_length
                && memcmp(id, psk[i].id, id_len) == 0) {
                if (result_length < psk[i].key_length) {
                    dtls_warn("buffer too small for PSK");
                    return
                        dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
                }
                memcpy(result, psk[i].key, psk[i].key_length);
                return psk[i].key_length;
            }
        }
    }

    return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}

#if NULL_REPLY
int ReadSensors(uint8 * data, size_t * len) //Null Reply
{
	//Set data to "00\r\n".
	data[0] = '0';
	data[1] = '0';
	data[2] = '\r';
	data[3] = '\n';
    *len = 4;
    return 3;
} 
//End of NULL_REPLY
#elif READ_SENSORS_ADD
int ReadSensors(uint8 * data, size_t * len) //Add
{
    unsigned int i[4], value, size;
    unsigned short random[2];

    random[0] = random_rand();
    random[1] = random_rand();
    
    //Computes an average of some random values...
    for (i[0] = 0, value = 0; i[0] < SAMPLING_SPACE; i[0]++) {
		    value = random[0] + random[1];
        //watchdog_periodic();
    }
    size = snprintf((char *) data, *len, "%02u\r\n", value % 100);
    *len = size;

    return size;
} //End of READ_SENSORS_ADD
#elif READ_SENSORS_MUL
int ReadSensors(uint8 * data, size_t * len) //Multiply
{

    unsigned int i[4], value, size;
    unsigned short random[2];

    random[0] = random_rand();
    random[1] = random_rand();
    
    //Computes an average of some random values...
    for (i[0] = 0, value = 0; i[0] < SAMPLING_SPACE; i[0]++) {
		    value = random[0] * random[1];
        //watchdog_periodic();
    }
    size = snprintf((char *) data, *len, "%02u\r\n", value % 100);
    *len = size;

    return size;
} //End of READ_SENSORS_MUL
#elif READ_SENSORS_MOD
int ReadSensors(uint8 * data, size_t * len) //Modular
{

    unsigned int i[4], value, size;
    unsigned short random[2];

    random[0] = random_rand();
    random[1] = random_rand();
    
    //Computes an average of some random values...
    for (i[0] = 0, value = 0; i[0] < SAMPLING_SPACE; i[0]++) {
		    value = random[0] % random[1];
        //watchdog_periodic();
    }
    size = snprintf((char *) data, *len, "%02u\r\n", value % 100);
    *len = size;

    return size;
} //End of READ_SENSORS_MUL
#else
//Default ReadSensors().
#define SAMPLING_SPACE 2485
int ReadSensors(uint8 * data, size_t * len)
{

    int count, value, size;

    //Computes an average of some random values...
    for (count = 0, value = 0; count < SAMPLING_SPACE; count++) {
        value = (value + random_rand()) % 30;
        watchdog_periodic();
    }
    size = snprintf((char *) data, *len, "%02u\r\n", value);
    *len = size;

    return size;
}
#endif 

//DtlsReadCb() is called by DTLS module after DTLS module decrypts a data packet. This funtion can also be considered as the main function of server message handler.
int DtlsReadCb(struct dtls_context_t *ctx,
               session_t * session, uint8 * data, size_t len)
{
    uint8 buf[MAX_BUF];
    size_t buflen = MAX_BUF;
    clock_time_t begin, end;

    //Recognise "GET" command only.
    if (!strncasecmp("GET\n", (char *) data, 4)) {
        PRINTF("Received GET\n");
        memset(buf, 0, MAX_BUF);
        begin = clock_time();	//Start timing.
        ReadSensors(buf, &buflen);
        end = clock_time();	//End timing.
        printf("ReadSensors Execution Time = %u - %u = %u \n",
               (unsigned int) end, (unsigned int) begin,
               (unsigned int) (end - begin));
        PRINTF("Sending: %s", buf);
        dtls_write(ctx, session, buf, buflen);
    } else {
        dtls_write(ctx, session, (unsigned char *) INVALID_COMMAND,
                   strlen(INVALID_COMMAND));
    }
    return 0;
}

int DtlsSendCb(struct dtls_context_t *ctx,
               session_t * session, uint8 * data, size_t len)
{
    struct udp_socket *conn = (struct udp_socket *) dtls_get_app_data(ctx);

    PRINTF("send to ");
    PRINT6ADDR(&session->addr);
    PRINTF(":%u\n", uip_ntohs(session->port));

    //Sends DTLS payload
    return udp_socket_sendto(conn, data, len, &session->addr,
                             session->port);
}

void Init()
{
    static dtls_handler_t cb = {
        .write = DtlsSendCb,
        .read = DtlsReadCb,
        .event = NULL,
#ifdef DTLS_PSK
        .get_psk_info = get_psk_info,
#endif                          /* DTLS_PSK */
    };

    PRINTF("DTLS server started\n");
    dtls_init();                //DTLS module initialisation.
    dtls_set_log_level(DTLS_LOG_DEBUG); //Print out debug info.
    dtls_context = dtls_new_context(&server_conn);

    //Register a DTLS port. Data received on that port is handled by DtlsServerCb().
    udp_socket_register(&server_conn, dtls_context, DtlsServerCb);
    udp_socket_bind(&server_conn, 20220);

    //Use functions specified in cb as the dtls moduoe callback functions.
    if (dtls_context)
        dtls_set_handler(dtls_context, &cb);
    return;
}

//DtlsServerCb() is called upon packets received.
void DtlsServerCb(struct udp_socket *sock,
                  void *apparg,
                  const uip_ipaddr_t * remoteaddr,
                  uint16_t remoteport,
                  const uip_ipaddr_t * localaddr,
                  uint16_t localport, const uint8_t * data,
                  uint16_t datalen)
{
    session_t session;
    uint8_t databuf[MAX_BUF] = { 0 };

    PRINTF("%u bytes received from ", datalen);
    PRINT6ADDR(remoteaddr);
    PRINTF(":%u.\n", remoteport);

    //Set remote endpoint using source information in the packet.
    uip_ipaddr_copy(&session.addr, remoteaddr);
    session.port = remoteport;
    session.size = sizeof(session.addr) + sizeof(session.port);

    memcpy(databuf, data, datalen);     //Move UDP payload around...(FIXME)

    //Pass the received encrypted packet to DTLS module.
    dtls_handle_message((dtls_context_t *) apparg, &session, databuf,
                        datalen);

    return;
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
PROCESS_THREAD(udp_server_process, ev, data)
{
    PROCESS_BEGIN();
    printf("CLOCK_SECOND=%lu\n", CLOCK_SECOND);

#ifdef SERVER_RPL_DAG           //Enable this if server needed to be a network root.
    ipaddr = set_global_address();
    create_rpl_dag(ipaddr);
#endif

    Init();
    if (!dtls_context) {
        dtls_emerg("cannot create context\n");
        PROCESS_EXIT();
    }

    while (1) {
        PROCESS_YIELD();
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
