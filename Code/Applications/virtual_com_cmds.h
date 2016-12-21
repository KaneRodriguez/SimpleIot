
#include <string.h>

/******************************************************************************/
// Virtual Com Port Communication
/******************************************************************************/
#define MESSAGE_LENGTH 7

void COM_Init(void);
void TXString( char* string, int length );
void transmitData(int addr, signed char rssi,  char msg[MESSAGE_LENGTH] );
void transmitDataString(char data_mode, char addr[4],char rssi[3], char msg[MESSAGE_LENGTH]);
__interrupt void USCI0RX_ISR(void);


