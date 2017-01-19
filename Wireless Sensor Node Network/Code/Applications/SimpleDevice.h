#ifndef SIMPLE_DEVICE_C
#define SIMPLE_DEVICE_C

#include <msp430.h>
#include <string.h>

#define NUMBER_OF_DEVICE_PINS 3

typedef struct CSimpleDevice {

	double deviceVoltage;
	double deviceTemperatureF;

	double devicePin3Voltage;
	double devicePin4Voltage;
	double devicePin5Voltage;

	void (*initialize)(struct CSimpleDevice* d);
	void (*update)(struct CSimpleDevice* d);

	double (*getDeviceVoltage)();
	double (*getDevicePinVoltage)(int,double);
	double (*getDeviceTemperatureF)();


} CSimpleDevice;

void initializeSimpleDevice(CSimpleDevice*);
void turnOffADC();
void startADC();
void setupADC(int);
double currentDeviceVoltage();
double currentDevicePinVoltage(int, double);
double currentDeviceTemperatureF();
void updateDevice(CSimpleDevice*);

void initializeSimpleDevice(CSimpleDevice* d) {
	// give the device its functionality
	d->getDeviceVoltage = &currentDeviceVoltage;
	d->getDevicePinVoltage = &currentDevicePinVoltage;
	d->getDeviceTemperatureF = &currentDeviceTemperatureF;
	d->update = &updateDevice;

	// update the values of the device
	d->update(d);
}
void updateDevice(CSimpleDevice* d) {

	// update voltage and temperature
	d->deviceVoltage = d->getDeviceVoltage();
	d->deviceTemperatureF = d->getDeviceTemperatureF();

	// update device pin voltages
	d->devicePin3Voltage = d->getDevicePinVoltage(3, d->deviceVoltage);
	d->devicePin4Voltage = d->getDevicePinVoltage(4, d->deviceVoltage);
	d->devicePin5Voltage = d->getDevicePinVoltage(5, d->deviceVoltage);
}


double currentDeviceVoltage() {
        volatile int volt;
        volatile long adcValue;

        /* Get voltage */
        ADC10CTL1 = INCH_11;                     // AVcc/2
        ADC10CTL0 = SREF_1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
        __delay_cycles(240);
        ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
        __bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled
        adcValue = ADC10MEM;                  // Retrieve result

        turnOffADC();

        volt = (adcValue*25)/512;

        return ( ( (double) volt ) / 10 );
}
void turnOffADC() {
        /* Stop and turn off ADC */
        ADC10CTL0 &= ~ENC;
        ADC10CTL0 &= ~(REFON + ADC10ON);
}

void startADC() {
    ADC10CTL0 |= ENC + ADC10SC;
    /*
     *	ENC		=>	"ADC10 Enable Conversion"
     *	ADC10SC	=>	"ADC10 Start Conversion"
     */
}
void setupADC(int pinNumber) {
	unsigned int currentInch;
	unsigned int currentControl;

	 ADC10CTL0 = SREF_0 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
	  /*
	   * 	ADC10CTL0:
	   *
	   * 	ADC10SHT_3	=>	SampleHoldTime: "16 x ADC10CLKs"
	   * 	ADC10ON		=>	ADC10:			"ADC10 On/Enable"
	   * 	ADC10IE		=>	Interrupts:		"ADC10 Interrupt Enabled"
	   *	REFON		=>	Internal Ref:	"ADC10 Internal Ref On"
	   *	REF2_5V		=>	Ref Volt:		"ADC10 Ref 1: 2.5V"
	   *
	   *	SREF_1		=>		VR+ = AVCC and VR- = AVSS
	   */
	 switch(pinNumber) {
	 case 3: currentInch = INCH_0;
	 	 	 currentControl = 0x01;
	  break;
	 case 4: currentInch = INCH_1;
	 	 	 currentControl = 0x02;
	  break;
	 case 5: currentInch = INCH_2;
	 	 	 currentControl = 0x03;
	  break;
	 default:
		 currentInch = INCH_0;
		 currentControl = 0x01;
	  break;
	 }

	  ADC10CTL1 = currentInch + ADC10DIV_4;

	  ADC10AE0 |= currentControl;
}
double currentDeviceTemperatureF() {
	volatile int * tempOffset = (int *)0x10F4; // required for temperature sensor
    volatile int degC, adcValue;
    volatile double fahrenheit, celsius = 0;

    /* Get temperature */
    ADC10CTL1 = INCH_10 + ADC10DIV_4;       // Temp Sensor ADC10CLK/5
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
    /* Allow ref voltage to settle for at least 30us (30us * 8MHz = 240 cycles)
     * See SLAS504D for settling time spec
     */
    __delay_cycles(240);
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled
    adcValue = ADC10MEM;                  // Retrieve result

    turnOffADC();


	degC = ((adcValue - 673) * 4230) / 1024;
	if( (*tempOffset) != 0xFFFF )
	{
		degC += (*tempOffset);
	}


    int temp = degC;


      temp = (int)(((double)temp)*1.8)+320;


    return ( ( (((double)temp)*1.8)+320 ) / 10 );
}
double currentDevicePinVoltage(int pinNumber, double deviceVoltage) {
	volatile double convertedValue = 0;
    volatile double groundRef = 0;
    volatile double powerRef = deviceVoltage;
    volatile double steps = 1023;
    volatile unsigned int adcValue = 0;

    setupADC(pinNumber);

    startADC();

    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
    __delay_cycles(240);

    while (ADC10CTL1 & BUSY);

    /*
     * 	ADC10MEM:
     *
     * 	Conversion:
     *
     * 	Nadc = 1023 * (Vin - Vr-) / ( Vr+ - Vr- )
     *
     *  Vin = ( Nadc * (Vr+ - Vr-) / 1023 ) + Vr-
     *
     *
     */

    adcValue = ADC10MEM;

    convertedValue = ( ( (double) adcValue ) * ( powerRef - groundRef) / steps ) - groundRef;

    turnOffADC();

    return convertedValue;
}

#endif
