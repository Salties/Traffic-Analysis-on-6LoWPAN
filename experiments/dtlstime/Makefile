#CONTIKI_PROJECT = dtls-server dtls-client
CONTIKI_PROJECT = dtls-server
all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#Disable PSK
#CFLAGS += -DDISABLE_PSK

#Disable ECC
CFLAGS += -DDISABLE_ECC

#Response length.
CFLAGS += -DPAYLOAD_LEN=23

include $(CONTIKI)/Makefile.include
