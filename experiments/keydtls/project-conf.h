#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#define UIP_CONF_TCP                   0

//RDC driver
#undef NETSTACK_CONF_RDC
//#define NETSTACK_CONF_RDC nullrdc_driver
#define NETSTACK_CONF_RDC contikimac_driver
//#define NETSTACK_CONF_RDC sicslowmac_driver
//#define NETSTACK_CONF_RDC cxmac_driver

//MAC driver
#if 0
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC nullmac_driver
#endif

//#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8

#endif
