all: hello-world
#APPS=servreg-hack
CONTIKI=$(CONTIKI_ROOT)


CONTIKI_WITH_IPV6 = 1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -DTARGET_NAME=\"${TARGET}\"

#Enable LLSEC
CFLAGS += -DENABLE_LLSEC
MODULES += core/net/llsec/noncoresec

include $(CONTIKI)/Makefile.include
