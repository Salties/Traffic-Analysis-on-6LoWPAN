CONTIKI_PROJECT = ecctiming ecdsatime_server ecdsatime_client prngtest
all: $(CONTIKI_PROJECT)

CONTIKI = ${CONTIKI_ROOT}

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

DTLSLIB=tinydtls_timing

APPS+= ${DTLSLIB} ${DTLSLIB}/aes ${DTLSLIB}/sha2 ${DTLSLIB}/ecc

include $(CONTIKI)/Makefile.include
