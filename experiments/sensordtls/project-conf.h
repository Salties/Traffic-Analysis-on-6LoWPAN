#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#ifdef UIP_CONF_TCP
#undef UIP_CONF_TCP
#define UIP_CONF_TCP                   0
#endif

//Disable Watchdog for ECC timeout
#define WATCHDOG_CONF_ENABLE 0


#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          64
#endif

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    256
#endif

#ifndef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  120
#endif

//RDC driver
//#undef NETSTACK_CONF_RDC
//#define NETSTACK_CONF_RDC nullrdc_driver
//#define NETSTACK_CONF_RDC contikimac_driver
//#define NETSTACK_CONF_RDC sicslowmac_driver
//#define NETSTACK_CONF_RDC cxmac_driver

//MAC driver
#if 0
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC nullmac_driver
#endif

//#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8

//#define SERVER_RPL_DAG
//


#endif
