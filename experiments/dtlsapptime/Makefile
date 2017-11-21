CONTIKI_PROJECT = dtls-server dtls-server-aes dtls-server-len
all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#Disable PSK
#CFLAGS += -DDISABLE_PSK

#Disable ECC
CFLAGS += -DDISABLE_ECC

include $(CONTIKI)/Makefile.include
