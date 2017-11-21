all: broadcast-example
#APPS=servreg-hack
CONTIKI=$(CONTIKI_ROOT)

APPS+=servreg-hack

CONTIKI_WITH_IPV6 = 1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -DTARGET_NAME=\"${TARGET}\"

#Enable LLSEC
CFLAGS += -DENABLE_LLSEC
MODULES += core/net/llsec/noncoresec

#Verbose output for aestests
CFLAGS += -DVERBOSE_AESTEST

#Number of Rounds for aestiming.
CFLAGS += -DNROUND=200

#Number of Sample for aestiming.
CFLAGS += -DNSAMPLE=1000

#Calibrate the AES timing framework test.
CFLAGS += -DTIMING_FRAMEWORK_TEST

#Disable HW AES
CFLAGS += -DDISABLE_HW_AES

include $(CONTIKI)/Makefile.include
