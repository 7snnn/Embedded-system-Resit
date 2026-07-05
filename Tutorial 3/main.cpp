#include "mbed.h"
#include "arm_book_lib.h"

DigitalOut alarmLed(LED1);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

bool gasAlarmState = OFF;
bool overTempAlarmState = OFF;

void outputsInit();
void uartTask();
void availableCommands();

int main()
{
    outputsInit();
    while (true) {
        uartTask();
    }
}

void outputsInit()
{
    alarmLed = OFF;
}

void uartTask()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
        if ( receivedChar == '1' ) {
            gasAlarmState = !gasAlarmState;
            if ( gasAlarmState ) {
                uartUsb.write( "Gas alarm activated\r\n", 21 );
            } else {
                uartUsb.write( "Gas alarm deactivated\r\n", 23 );
            }
        } else if ( receivedChar == '2' ) {
            if ( gasAlarmState ) {
                uartUsb.write( "Gas alarm state: ON\r\n", 21 );
            } else {
                uartUsb.write( "Gas alarm state: OFF\r\n", 22 );
            }
        } else if ( receivedChar == '3' ) {
            overTempAlarmState = !overTempAlarmState;
            if ( overTempAlarmState ) {
                uartUsb.write( "Over-temperature alarm activated\r\n", 34 );
            } else {
                uartUsb.write( "Over-temperature alarm deactivated\r\n", 36 );
            }
        } else if ( receivedChar == '4' ) {
            if ( overTempAlarmState ) {
                uartUsb.write( "Over-temperature alarm state: ON\r\n", 34 );
            } else {
                uartUsb.write( "Over-temperature alarm state: OFF\r\n", 35 );
            }
        } else {
            availableCommands();
        }
        alarmLed = gasAlarmState || overTempAlarmState;
    }
}

void availableCommands()
{
    uartUsb.write( "Available commands:\r\n", 21 );
    uartUsb.write( "Press '1' to toggle gas alarm\r\n", 31 );
    uartUsb.write( "Press '2' to get gas alarm state\r\n", 34 );
    uartUsb.write( "Press '3' to toggle over-temperature alarm\r\n", 44 );
    uartUsb.write( "Press '4' to get over-temperature alarm state\r\n\r\n", 49 );
}
