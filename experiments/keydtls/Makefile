CONTIKI_PROJECT = dtls-server #dtls-client
all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls

#Uncomment to enable null mac and rdc driver
#CFLAGS += -DNULL_DRIVER

CFLAGS += -DPLATFORM_$(TARGET) -Os
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

include $(CONTIKI)/Makefile.include
