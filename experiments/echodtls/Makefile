#CONTIKI_PROJECT = dtls-server dtls-client
CONTIKI_PROJECT = dtls-server

all: $(CONTIKI_PROJECT)

DTLSLIB=tinydtls_timing

CONTIKI = $(CONTIKI_ROOT)
APPS += ${DTLSLIB}/aes ${DTLSLIB}/sha2 ${DTLSLIB}/ecc ${DTLSLIB}
#APPS += servreg-hack

#Uncomment to enable null mac and rdc driver
#CFLAGS += -DNULL_DRIVER

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#Non-debug mode.
#CFLAGS += -DNDEBUG

#Disable PSK
#CFLAGS += -DNO_PSK

#Disable ECC
#CFLAGS += -DNO_ECC

ifeq ($(TARGET),wismote)
CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
endif

include $(CONTIKI)/Makefile.include
