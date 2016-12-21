/*
 * CSimpleCommunication.h
 *
 *  Created on: Dec 19, 2016
 *      Author: Lu_Admin
 */

#ifndef CSIMPLECOMMUNICATION_H_
#define CSIMPLECOMMUNICATION_H_

#include <string.h>
#include <stdio.h>
#include <msp430.h>
#include <stdlib.h>

#include "SimpleMessage.h"

#define MAX_JSON_NAMES 10
#define MAX_JSON_VALUES 10

void simpleTXString( char*);
void setupRXTX();
__interrupt void ADC10_ISR(void);

typedef struct CSimpleCommunication {

	void (*send)(char*);
	void (*sendJsonMsg)(struct CSimpleCommunication * c, SimpleMessage * m);

} CSimpleCommunication;

void sendJsonMessage(CSimpleCommunication * c, SimpleMessage * m);

void initializeSimpleCommunication(CSimpleCommunication*);

void initializeSimpleCommunication(CSimpleCommunication* c) {
	// assign object functionality
	c->send = &simpleTXString;
	c->sendJsonMsg = &sendJsonMessage;

	// setup device for xmission
	setupRXTX();
}
char * custom_dtos(double value);

char * custom_dtos(double value) {
	// only need to handle 4 digits before and 4 after decimal place so, excuse this for being sloppy. Recommend implementing differently in the future
	char c = '0';
    char *str = (char *) malloc(sizeof(char) * 10);

	long int beforeDecimal = (long int) value;
	long int afterDecimal = (long int) (value * 10000);

	c = '0' + (beforeDecimal/1000)%10; // thousands
	str[0] = c;

	c = '0' + (beforeDecimal/100)%10; // hundreds
	str[1] = c;

	c = '0' + (beforeDecimal/10)%10; // tens
	str[2] = c;

	c = '0' + (beforeDecimal)%10; // ones
	str[3] = c;

	c = '.';
	str[4] = c;

	c = '0' + (afterDecimal/1000)%10; // thousands
	str[5] = c;

	c = '0' + (afterDecimal/100)%10; // hundreds
	str[6] = c;

	c = '0' + (afterDecimal/10)%10; // tens
	str[7] = c;

	c = '0' + (afterDecimal)%10; // ones
	str[8] = c;

    str[9] = '\0';

	return str;
}
void sendJsonMessage(CSimpleCommunication * c, SimpleMessage * m) {
    char * tmp;

    // make sure the message is up to date
	m->update(m);

	// begin object
	c->send("{");

	// send device name
    c->send("'deviceName':");

    if( m->sender != -1) {
    	tmp = custom_dtos(m->sender);
    	c->send("'ED");
    	c->send(tmp);
    	c->send("'");
    	free(tmp);

    } else {
        c->send("'AP'");
    }



    // send device voltage
	tmp = custom_dtos(m->messageDeviceVoltage);

	c->send(",'deviceVoltage':'");
	c->send(tmp);

	free(tmp); // it was malloced withing the function

	// send device temp
	tmp = custom_dtos(m->messageDeviceTemperatureF);

	c->send("','deviceTemperatureF':");
	c->send(tmp);

	free(tmp); // it was malloced withing the function

	// send device pin 3 voltage
	tmp = custom_dtos(m->messageDevicePin3Voltage);

	c->send("','devicePin3Voltage':'");
	c->send(tmp);

	free(tmp); // it was malloced withing the function

	// send device pin 4 voltage
	tmp = custom_dtos(m->messageDevicePin4Voltage);

	c->send("','devicePin4Voltage':'");
	c->send(tmp);

	free(tmp); // it was malloced withing the function

   //  end object
	c->send("'}");

}

void setupRXTX() {
	   if (CALBC1_1MHZ==0xFF)					// If calibration constant erased
	  {
	    while(1);                               // do not load, trap CPU!!
	  }
	  DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	  DCOCTL = CALDCO_1MHZ;
	  P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
	  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	  UCA0BR0 = 104;                            // 1MHz 9600
	  UCA0BR1 = 0;                              // 1MHz 9600
	  UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	  IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}
void simpleTXString( char* string )
{
  int pointer;
  for( pointer = 0; pointer < strlen(string); pointer++)
  {
    volatile int i;
    UCA0TXBUF = string[pointer];
    while (!(IFG2&UCA0TXIFG));              // USCI_A0 TX buffer ready?
  }
}

/*------------------------------------------------------------------------------
 * ADC10 interrupt service routine
 *----------------------------------------------------------------------------*/
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

#endif /* CSIMPLECOMMUNICATION_H_ */
