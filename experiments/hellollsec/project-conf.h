#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__


#ifdef UIP_CONF_TCP
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0 //Disable TCP for space optimise.
#endif //End of #ifdef UIP_CONF_TCP

//RDC driver
//#undef NETSTACK_CONF_RDC
//#define NETSTACK_CONF_RDC nullrdc_driver
//#define NETSTACK_CONF_RDC contikimac_driver

//MAC driver
//#undef NETSTACK_CONF_MAC
//#define NETSTACK_CONF_MAC nullmac_driver

//Channel check rate
//#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8

//LLSEC configurations.
#ifdef ENABLE_LLSEC

#undef NETSTACK_CONF_LLSEC
#define NETSTACK_CONF_LLSEC noncoresec_driver

#undef LLSEC802154_CONF_SECURITY_LEVEL
#define LLSEC802154_CONF_SECURITY_LEVEL 7

#undef NONCORESEC_CONF_KEY
#define NONCORESEC_CONF_KEY {\
	0x00 , 0x01 , 0x02 , 0x03 ,\
	0x04 , 0x05 , 0x06 , 0x07 ,\
	0x08 , 0x09 , 0x0A , 0x0B ,\
	0x0C , 0x0D , 0x0E , 0x0F \
}

//Uncomment to disable Hardware AES-128 coprosessor.
#ifdef AES_128_CONF
//#undef AES_128_CONF
#endif //End of #ifdef AES_128_CONF

#endif //End of #ifdef ENABLE_LLSEC

#ifdef CONTIKI_TARGET_CC2538DK 
#define PRIu32 "lu" 
#endif //End of #ifdef CONTIKI_TARGET_CC2538DK

#endif //End of #ifdef __PROJECT_CONF_H__
