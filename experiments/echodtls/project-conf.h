#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#ifdef UIP_CONF_TCP
#undef UIP_CONF_TCP
#define UIP_CONF_TCP                   0
#endif

//Disable Watchdog for ECC timeout
#define WATCHDOG_CONF_ENABLE 0

//Change Packet buffer setting for exceedingly large handshake packets.
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          64
#endif

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    256
#endif

#ifndef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  120
#endif

//Disable PSK
#ifdef NO_PSK
#ifdef DTLS_PSK
#undef DTLS_PSK
#endif //End of DTLS_PSK
#endif //End of NO_PSK

//Disable ECC
#ifdef NO_ECC
#ifdef DTLS_ECC
#undef DTLS_ECC
#endif //End of DTLS_ECC
#endif //End of NO_ECC

#define ECDSA_TIMING
#define ECDSA_PRNG_ATTACK

#endif
