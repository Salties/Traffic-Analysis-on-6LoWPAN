#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#define UIP_CONF_TCP                   0

#ifdef DISABLE_PSK //Disable PSK
#ifdef DTLS_PSK
#undef DTLS_PSK
#endif
#endif

#ifdef DISABLE_ECC //Disable ECC
#ifdef DTLS_ECC
#undef DTLS_ECC
#endif
#endif

#endif
