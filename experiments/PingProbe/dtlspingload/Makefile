#CONTIKI_PROJECT = dtls-server dtls-client
CONTIKI_PROJECT = dtls-server
all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls
#APPS += servreg-hack

#Uncomment to enable null mac and rdc driver
#CFLAGS += -DNULL_DRIVER

CFLAGS += -DPLATFORM_$(TARGET)
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

include $(CONTIKI)/Makefile.include
