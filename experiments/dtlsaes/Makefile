CONTIKI_PROJECT = aestest_fixed aestest_random

all: $(CONTIKI_PROJECT)

CONTIKI = $(CONTIKI_ROOT)
APPS += tinydtls/aes tinydtls/sha2 tinydtls/ecc tinydtls

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#Platform name.
CFLAGS += -DTARGET_NAME=\"$(TARGET)\"

#Number of rounds per sample.
CFLAGS += -DNROUND=200

#Sample size.
CFLAGS += -DNSAMPLE=1000

#Use verbose output.
CFLAGS += -DVERBOSE_AESTEST

#Calbrate the timing framework.
#CFLAGS += -DCALIBRATE_FRAMEWORK

include $(CONTIKI)/Makefile.include
