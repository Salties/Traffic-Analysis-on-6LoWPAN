#CONTIKI_PROJECT = dtls-server dtls-client
CONTIKI_PROJECT = dtls-server
all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls
#APPS += servreg-hack

#Uncomment to enable null mac and rdc driver
#CFLAGS += -DNULL_DRIVER

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__


include $(CONTIKI)/Makefile.include
