#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "vlo_rand.h"

#include <msp430.h>
#include <string.h>
#include <stdio.h>

#include "SimpleDevice.h"
#include "SimpleMessage.h"
#include "SimpleCommunication.h"

/*------------------------------------------------------------------------------
 * Defines
 *----------------------------------------------------------------------------*/
/* How many times to try a TX and miss an acknowledge before doing a scan */
#define MISSES_IN_A_ROW  2

/*------------------------------------------------------------------------------
 * Prototypes
 *----------------------------------------------------------------------------*/
static void linkTo(void);
void createRandomAddress(void);
__interrupt void Timer_A (void);

/*------------------------------------------------------------------------------
* Globals
------------------------------------------------------------------------------*/
static linkID_t sLinkID1 = 0;
/* Temperature offset set at production */
volatile int * tempOffset = (int *)0x10F4;
/* Initialize radio address location */
char * Flash_Addr = (char *)0x10F0;
/* Work loop semaphores */
static volatile uint8_t sSelfMeasureSem = 0;

/*------------------------------------------------------------------------------
 * Main
 *----------------------------------------------------------------------------*/

CSimpleCommunication console;
CSimpleDevice device;


void main (void)
{
  addr_t lAddr;

  /* Initialize board-specific hardware */
  BSP_Init();

  // initialize console (MUST BE DONE AFTER THE BOARD!!!!!)

  initializeSimpleCommunication(&console);
  initializeSimpleDevice(&device);


  console.send("\r\nInitializing End Device...");


  device.update(&device);

  char buffer[60];

	char * tmp = custom_dtos(device.deviceVoltage);
	console->send("\r\n\nInitial Device Voltage: ");
	console->send(tmp);
	free(tmp); // it was malloced within the function

  /* Check flash for previously stored address */
  if(Flash_Addr[0] == 0xFF && Flash_Addr[1] == 0xFF &&
     Flash_Addr[2] == 0xFF && Flash_Addr[3] == 0xFF )
  {
    createRandomAddress(); // Create and store a new random address
  }

  /* Read out address from flash */
  lAddr.addr[0] = Flash_Addr[0];
  lAddr.addr[1] = Flash_Addr[1];
  lAddr.addr[2] = Flash_Addr[2];
  lAddr.addr[3] = Flash_Addr[3];

  /* Tell network stack the device address */
  SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);

  /* Initialize TimerA and oscillator */
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO
  TACCTL0 = CCIE;                           // TACCR0 interrupt enabled
  TACCR0 = 12000;                           // ~ 1 sec
  TACTL = TASSEL_1 + MC_1;                  // ACLK, upmode

  /* Keep trying to join (a side effect of successful initialization) until
   * successful. Toggle LEDS to indicate that joining has not occurred.
   */
  while (SMPL_SUCCESS != SMPL_Init(0))
  {
    BSP_TOGGLE_LED1();
    BSP_TOGGLE_LED2();
    /* Go to sleep (LPM3 with interrupts enabled)
     * Timer A0 interrupt will wake CPU up every second to retry initializing
     */
    __bis_SR_register(LPM3_bits+GIE);  // LPM3 with interrupts enabled
  }

  /* LEDs on solid to indicate successful join. */
  BSP_TURN_ON_LED1();
  BSP_TURN_ON_LED2();

  /* Unconditional link to AP which is listening due to successful join. */
  linkTo();

  while(1);
}

static void linkTo()
{
  uint8_t msg[3];
#ifdef APP_AUTO_ACK
  uint8_t misses, done;
#endif

  /* Keep trying to link... */
  while (SMPL_SUCCESS != SMPL_Link(&sLinkID1))
  {
    BSP_TOGGLE_LED1();
    BSP_TOGGLE_LED2();
    /* Go to sleep (LPM3 with interrupts enabled)
     * Timer A0 interrupt will wake CPU up every second to retry linking
     */
    __bis_SR_register(LPM3_bits+GIE);
  }

  /* Turn off LEDs. */
  BSP_TURN_OFF_LED1();
  BSP_TURN_OFF_LED2();

  /* Put the radio to sleep */
  SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);

  while (1)
  {
    /* Go to sleep, waiting for interrupt every second to acquire data */
    __bis_SR_register(LPM3_bits);

    /* Time to measure */
    if (sSelfMeasureSem) {
      volatile long temp;
      int degC, volt;
      int results[2];
#ifdef APP_AUTO_ACK
      uint8_t      noAck;
      smplStatus_t rc;
#endif

      device.update(&device);

      SimpleMessage message;

      initializeSimpleMessage(&message);

      message.package(&message,
    		  device.deviceTemperatureF,
			  device.deviceVoltage,
			  device.devicePin3Voltage,
			  device.devicePin4Voltage);

      message.update(&message);



      /* Get radio ready...awakens in idle state */
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);

#ifdef APP_AUTO_ACK
      /* Request that the AP sends an ACK back to confirm data transmission
       * Note: Enabling this section more than DOUBLES the current consumption
       *       due to the amount of time IN RX waiting for the AP to respond
       */
      done = 0;
      while (!done)
      {
        noAck = 0;

        /* Try sending message MISSES_IN_A_ROW times looking for ack */
        for (misses=0; misses < MISSES_IN_A_ROW; ++misses)
        {
          if (SMPL_SUCCESS == (rc=SMPL_SendOpt(sLinkID1, message.messageContents, sizeof(message.messageContents), SMPL_TXOPTION_ACKREQ)))
          {
            /* Message acked. We're done. Toggle LED 1 to indicate ack received. */
            BSP_TURN_ON_LED1();
            __delay_cycles(2000);
            BSP_TURN_OFF_LED1();
            break;
          }
          if (SMPL_NO_ACK == rc)
          {
            /* Count ack failures. Could also fail becuase of CCA and
             * we don't want to scan in this case.
             */
            noAck++;
          }
        }
        if (MISSES_IN_A_ROW == noAck)
        {
          /* Message not acked */
          BSP_TURN_ON_LED2();
          __delay_cycles(2000);
          BSP_TURN_OFF_LED2();
#ifdef FREQUENCY_AGILITY
          /* Assume we're on the wrong channel so look for channel by
           * using the Ping to initiate a scan when it gets no reply. With
           * a successful ping try sending the message again. Otherwise,
           * for any error we get we will wait until the next button
           * press to try again.
           */
          if (SMPL_SUCCESS != SMPL_Ping(sLinkID1))
          {
            done = 1;
          }
#else
          done = 1;
#endif  /* FREQUENCY_AGILITY */
        }
        else
        {
          /* Got the ack or we don't care. We're done. */
          done = 1;
        }
      }
#else

      /* No AP acknowledgement, just send a single message to the AP */
      SMPL_SendOpt(sLinkID1, message.messageContents, sizeof(message.messageContents), SMPL_TXOPTION_NONE);

#endif /* APP_AUTO_ACK */

      /* Put radio back to sleep */
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);

      /* Done with measurement, disable measure flag */
      sSelfMeasureSem = 0;
    }
  }
}

void createRandomAddress()
{
  unsigned int rand, rand2;
  do
  {
    rand = TI_getRandomIntegerFromVLO();    // first byte can not be 0x00 of 0xFF
  }
  while( (rand & 0xFF00)==0xFF00 || (rand & 0xFF00)==0x0000 );
  rand2 = TI_getRandomIntegerFromVLO();

  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
  DCOCTL = CALDCO_1MHZ;
  FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
  FCTL3 = FWKEY + LOCKA;                    // Clear LOCK & LOCKA bits
  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation

  Flash_Addr[0]=(rand>>8) & 0xFF;
  Flash_Addr[1]=rand & 0xFF;
  Flash_Addr[2]=(rand2>>8) & 0xFF;
  Flash_Addr[3]=rand2 & 0xFF;

  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCKA + LOCK;             // Set LOCK & LOCKA bit
}


/*------------------------------------------------------------------------------
 * Timer A0 interrupt service routine
 *----------------------------------------------------------------------------*/
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  sSelfMeasureSem = 1;
  __bic_SR_register_on_exit(LPM3_bits);        // Clear LPM3 bit from 0(SR)
}
