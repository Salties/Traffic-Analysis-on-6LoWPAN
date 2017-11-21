#ifdef CONTIKI_TARGET_CC2538DK

#include "contiki.h"
#include "sys/rtimer.h"
#include "lib/random.h"
#include "dev/serial-line.h"
#include "lpm.h"
#include "dev/leds.h"

#include "dev/rfcore.h"
#include "dev/cc2538-rf.h"
#include "dev/soc-adc.h"
#include "dev/sys-ctrl.h"
#include "reg.h"
#include "net/netstack.h"

#include <stdio.h>		/* For printf() */
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS (prng_test_process, "PRNG test");
AUTOSTART_PROCESSES (&prng_test_process);
/*---------------------------------------------------------------------------*/

#define VERBOSE 1
#define WAIT_TIME ((CLOCK_SECOND * 0.1))
#define SEEDLEN ((16))		//Seed length in bytes.

static struct etimer et;

unsigned short
ReadRnd ()
{
  return REG (SOC_ADC_RNDL) | (REG (SOC_ADC_RNDH) << 8);
}

void
printseed (unsigned char *seedval, size_t seedlen)
{
  size_t i;

  if (seedlen <= 0)
    return;

  for (i = 0; i < seedlen; i++)
    printf ("%02X", seedval[i]);
  return;
}

void
RfRnd (unsigned char *seed, size_t seedlen)
{
  int i = 0, j = 0;
  unsigned char s = 0;
  int overheated = 0;
  unsigned char rndiq;
#if VERBOSE
  static unsigned int prnd[4] = { 0 };
  int rssi = 0, freqest = 0;
  unsigned char agc_gain, lna1, lna2, lna3;
#endif

  memset (seed, 0, seedlen);

  /* Make sure the RNG is on */
  REG (SOC_ADC_ADCCON1) &= ~(SOC_ADC_ADCCON1_RCTRL1 | SOC_ADC_ADCCON1_RCTRL0);

  /* Enable clock for the RF Core */
  REG (SYS_CTRL_RCGCRFC) = 1;

  /* Wait for the clock ungating to take effect */
  while (REG (SYS_CTRL_RCGCRFC) != 1);

  /* Infinite RX - FRMCTRL0[3:2] = 10
   *    * This will mess with radio operation - see note above */
  REG (RFCORE_XREG_FRMCTRL0) = 0x00000008;

  /* Turn RF on */
  CC2538_RF_CSP_ISRXON ();

  //This is something evil...
  {
    //Turn on PD_OVERRIDE
    //REG(RFCORE_XREG_PTEST1) = 0x08;
    //Turn off LNA and mixer.
    //REG(RFCORE_XREG_PTEST0) = 0x02 << 2;
    
    //AGC override.
    //To achieve the best bias result, tune AGCs to max and disable AAF attenuation.
    REG(RFCORE_XREG_AGCCTRL0) = 0x00; 	//Disable AAF attenuation.
    //REG (RFCORE_XREG_AGCCTRL1) = 0x00;	//Manipulate refernece voltage.
    REG (RFCORE_XREG_AGCCTRL2) = 0xFF;	//Tune all AGCs to maximum.
  }

  /*
   *    * Wait until "the chip has been in RX long enough for the transients to
   *       * have died out. A convenient way to do this is to wait for the RSSI-valid
   *          * signal to go high."
   *             */
  while (!(REG (RFCORE_XREG_RSSISTAT) & RFCORE_XREG_RSSISTAT_RSSI_VALID));
  /*
   *    * Form the seed by concatenating bits from IF_ADC in the RF receive path.
   *       * Keep sampling until we have read at least 16 bits AND the seed is valid
   *          *
   *             * Invalid seeds are 0x0000 and 0x8003 and should not be used.
   *                */

  for (i = 0; i < seedlen; i++)
    {
      //Read 8 bits from RFRND.
      for (j = 0; j < 8; j++)
	{
	  //Check if RF is working.
	  if (!
	      ((REG (RFCORE_XREG_RSSISTAT) &
		RFCORE_XREG_RSSISTAT_RSSI_VALID)))
	    {
	      //RF core is invalidated. Stop reading.
	      printf ("#RFRND invalidated.\n");
	      overheated = 1;
	      break;
	    }

	  //Read one bit from RFRND.
	  s <<= 1;
	  rndiq = REG (RFCORE_XREG_RFRND);
	  //Use random bit on I channel.
	  s |= rndiq & RFCORE_XREG_RFRND_IRND;
#if VERBOSE
	  prnd[rndiq]++;
#endif
	}
      if (overheated)
	break;

      //Store a random byte.
      seed[i] = s;
      s = 0;
    }

#if VERBOSE
  //Read out RSSI, FREQEST and AGC info.
  if (128 <= (rssi = REG (RFCORE_XREG_RSSI)))
    rssi -= 256;
  if (128 <= (freqest = REG (RFCORE_XREG_FREQEST)))
    freqest -= 256;
  agc_gain = REG (RFCORE_XREG_AGCCTRL2);
  lna1 = ((agc_gain >> 6) & 0x03) * 3;
  lna2 = ((agc_gain >> 3) & 0x07) * 3;
  lna3 = ((agc_gain >> 1) & 0x03) * 3;

  //Print info.
  printf ("#(NBIT,RSSI,FREQEST) %d %d %d\n",
	  i * 8, rssi - 73, (freqest * 7800 - 15000) / 1000);
  printf ("#(LNA1, LNA2, LNA3) %d %d %d\n", lna1, lna2, lna3);
  printf ("#P(RFRND) %u, %u, %u, %u\n", prnd[0], prnd[1], prnd[2], prnd[3]);
#else
  //Print bits read only.
  printf ("#(NBIT) %d\n", i * 8);
#endif

  /* RF Off. NETSTACK_RADIO.init() will sort out normal RF operation */
  CC2538_RF_CSP_ISRFOFF ();

  printseed (seed, i);
  printf ("\n");

  return;
}

PROCESS_THREAD (prng_test_process, ev, data)
{
  static unsigned char seed[SEEDLEN];

  PROCESS_BEGIN ();

  lpm_set_max_pm (LPM_PM0);

  etimer_set (&et, WAIT_TIME);
  for (;;)
    {
      PROCESS_WAIT_EVENT_UNTIL (etimer_expired (&et));
      etimer_restart (&et);
      leds_on (LEDS_ALL);
      RfRnd (seed, sizeof (seed));
      leds_off (LEDS_ALL);
    }

  PROCESS_END ();
}

/*---------------------------------------------------------------------------*/
#endif
