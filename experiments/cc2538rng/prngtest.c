#include "contiki.h"
#include "sys/rtimer.h"
#include "lib/random.h"
#include "dev/serial-line.h"
#include "lpm.h"

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(prng_test_process, "PRNG test");
AUTOSTART_PROCESSES(&prng_test_process);
/*---------------------------------------------------------------------------*/

static unsigned char appearance[8192] =  {0};

unsigned short GetMark(unsigned short val)
{
	return (0xFFFF & (appearance[val/8] & (0x1 << (val % 8)) ));
}

void SetMark(unsigned short val)
{
	appearance[val/8] |= (0x1 << (val % 8));
	return;
}

PROCESS_THREAD(prng_test_process, ev, data)
{
  static unsigned short rndval;
  static unsigned int count;
  static unsigned char *rndptr = (void*) &rndval;
  
  PROCESS_BEGIN();

  lpm_set_max_pm(LPM_PM0);

  printf("#Press Enter to start:");
  PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
  
  for(count = 1; count <= 0xFFFF; count++)
  {
	rndval = random_rand();
	printf("#Count: %d\n", count);
	printf("%02X%02X\n",rndptr[0], rndptr[1]);
	if(GetMark(rndval))
	{
		printf("#Collision found: ");
		printf("%02X%02X\n",rndptr[0], rndptr[1]);
	}
	SetMark(rndval);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
