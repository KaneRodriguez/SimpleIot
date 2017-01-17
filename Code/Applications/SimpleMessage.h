/*
 * SimpleMessage.h
 *
 *  Created on: Dec 19, 2016
 *      Author: Kane Rodriguez
 */

#ifndef SIMPLEMESSAGE_H_
#define SIMPLEMESSAGE_H_

#define SIMPLE_MESSAGE_LENGTH 7

#define SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_UB 6
#define SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_LB 5

#define SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_UB 4
#define SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_LB 3

#define SIMPLE_MESSAGE_DEVICE_VOLTAGE 2

#define SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_UB 1
#define SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_LB 0

// conversion factors
#define SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_CF 100
#define SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_CF 100
#define SIMPLE_MESSAGE_DEVICE_VOLTAGE_CF 10
#define SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_CF 10

typedef   unsigned char   uint8_t;

/* SimpleMessage format,  UB = upper Byte, LB = lower Byte
	--------------------------------------------------------------------------------------------
	| deviceF LB | deviceF UB |  deviceV LB |  pin3V LB |	 pin3V UB	| pin4V LB | pin4V UB  |
	--------------------------------------------------------------------------------------------
         0		  	 	1         	2			 3				4			 5	  		6

	CONVERSIONS:
		- deviceV is 10 times its actual value
		- deviceF is 10 times its actual value
		- pin3V is 100 times its actual value
		- pin4V is 100 times its actual value
*/

typedef struct SimpleMessage {
	uint8_t messageContents[SIMPLE_MESSAGE_LENGTH];

	int sender; // only used at the receiving end!!
	int rssiStrength; // only used at the receiving end!!

	double messageDeviceVoltage;
	double messageDeviceTemperatureF;
	double messageDevicePin3Voltage;
	double messageDevicePin4Voltage;

	void (*update)(struct SimpleMessage* m);
	void (*package)(struct SimpleMessage* m, double, double, double, double);
	void (*process)(struct SimpleMessage* m, uint8_t msg[SIMPLE_MESSAGE_LENGTH]);

	double (*getMessageDeviceVoltage)(struct SimpleMessage* m);
	double (*getMessageDeviceTemperatureF)(struct SimpleMessage* m);
	double (*getMessageDevicePin3Voltage)(struct SimpleMessage* m);
	double (*getMessageDevicePin4Voltage)(struct SimpleMessage* m);

} SimpleMessage;


double currentMessageDeviceVoltage(SimpleMessage*);
void updateMessage(SimpleMessage*);
void initializeSimpleMessage(SimpleMessage*);
double currentMessageDeviceTemperatureF(SimpleMessage*);
double currentMessageDevicePin3Voltage(SimpleMessage*);
double currentMessageDevicePin4Voltage(SimpleMessage*);
void packageSimpleMessage(SimpleMessage*, double, double, double, double);
void processSimpleMessage(SimpleMessage* m, uint8_t msg[SIMPLE_MESSAGE_LENGTH]);
signed int calculateRssiStrength(signed char rssi);

void initializeSimpleMessage(SimpleMessage* m) {
	m->getMessageDeviceVoltage = &currentMessageDeviceVoltage;
	m->getMessageDeviceTemperatureF = &currentMessageDeviceTemperatureF;
	m->getMessageDevicePin3Voltage = &currentMessageDevicePin3Voltage;
	m->getMessageDevicePin4Voltage = &currentMessageDevicePin4Voltage;

	m->update = &updateMessage;
	m->package = &packageSimpleMessage;
	m->process =  &processSimpleMessage;

	m->sender = -1;
	m->rssiStrength = 0;
}

double currentMessageDeviceVoltage(SimpleMessage* m) {
	// get the message section with the device voltage, convert to its actual value
	double volt = (double) m->messageContents[SIMPLE_MESSAGE_DEVICE_VOLTAGE];

	volt /= SIMPLE_MESSAGE_DEVICE_VOLTAGE_CF;

	return volt;
}

double currentMessageDeviceTemperatureF(SimpleMessage* m) {
	// get the message sections with the device temperature in fahrenheit, convert to its actual value
	uint8_t UB = m->messageContents[SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_UB];
	uint8_t LB = m->messageContents[SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_LB];
	double tempF = (double) ( ( UB << 8 ) + LB );

	tempF /= SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_CF;

	// conversion if this was in decC (int)(((float)temp)*1.8)+320

	return tempF;
}
double currentMessageDevicePin3Voltage(SimpleMessage* m) {
	// get the message sections with the device pin 3 voltage, convert to its actual value

	uint8_t UB = (double) m->messageContents[SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_UB];
	uint8_t LB = (double) m->messageContents[SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_LB];

	double volt = (double) ( ( UB << 8 ) + LB );

	volt /= SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_CF;

	return volt;
}
double currentMessageDevicePin4Voltage(SimpleMessage* m) {
	// get the message sections with the device pin 3 voltage, convert to its actual value

	uint8_t UB = (double) m->messageContents[SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_UB];
	uint8_t LB = (double) m->messageContents[SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_LB];

	double volt = (double) ( ( UB << 8 ) + LB );

	volt /= SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_CF;

	return volt;
}
void updateMessage(SimpleMessage* m) {
	// update values received from message
	m->messageDeviceVoltage = m->getMessageDeviceVoltage(m);
	m->messageDeviceTemperatureF = m->getMessageDeviceTemperatureF(m);
	m->messageDevicePin3Voltage = m->getMessageDevicePin3Voltage(m);
	m->messageDevicePin4Voltage = m->getMessageDevicePin4Voltage(m);
}

void packageSimpleMessage(SimpleMessage* m, double deviceF, double deviceV, double devicePin3V, double devicePin4V) {
	// package according to the protocol
	int pkgDeviceF = (int) ( deviceF * SIMPLE_MESSAGE_DEVICE_TEMPERATURE_F_CF );
	int pkgDeviceV = (int) ( deviceV * SIMPLE_MESSAGE_DEVICE_VOLTAGE_CF );
	int pkgDeviceP3V = (int) ( devicePin3V * SIMPLE_MESSAGE_DEVICE_PIN_4_VOLTAGE_CF );
	int pkgDeviceP4V = (int) ( devicePin4V * SIMPLE_MESSAGE_DEVICE_PIN_3_VOLTAGE_CF );

	m->messageContents[0] = pkgDeviceF;
	m->messageContents[1] = pkgDeviceF>>8;

	m->messageContents[2] = pkgDeviceV;

	m->messageContents[3] = pkgDeviceP3V;
	m->messageContents[4] = pkgDeviceP3V>>8;

	m->messageContents[5] = pkgDeviceP4V;
	m->messageContents[6] = pkgDeviceP4V>>8;

	// TODO: update the structure ?
	m->update(m);
}

void processSimpleMessage(SimpleMessage* m, uint8_t msg[SIMPLE_MESSAGE_LENGTH]) {
	int count = 0;

	for(count = 0; count < SIMPLE_MESSAGE_LENGTH; ++count) {
		m->messageContents[count] = msg[count];
	}

	m->update(m);
}
signed int calculateRssiStrength(signed char rssi) {
	  volatile signed int rssi_int;
	  rssi_int = (signed int) rssi;
	  rssi_int = rssi_int+128;
	  rssi_int = (rssi_int*100)/256;

	  return rssi_int;
}
#endif /* SIMPLEMESSAGE_H_ */
