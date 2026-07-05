#include "mbed.h"
#include "arm_book_lib.h"

#define NUMBER_OF_AVG_SAMPLES 100
#define TIME_INCREMENT_MS 10

AnalogIn potentiometer(A0);
AnalogIn lm35(A1);
AnalogIn mq2Analog(A2);
PwmOut passiveSiren(D9);

DigitalOut row1(D0), row2(D1), row3(D2), row4(D3);
DigitalIn col1(D4, PullUp), col2(D5, PullUp), col3(D6, PullUp), col4(D7, PullUp);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

float lm35ReadingsArray[NUMBER_OF_AVG_SAMPLES];
int lm35SampleIndex = 0;

int logEvents[5] = {0, 0, 0, 0, 0};
int logIndex = 0;

char keypadMatrix[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char inputSequence[3] = {'\0', '\0', '\0'};
int digitCount = 0;
int alarmActive = 0;

float getTemperatureCelsius() {
    lm35ReadingsArray[lm35SampleIndex] = lm35.read();
    lm35SampleIndex++;
    if (lm35SampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        lm35SampleIndex = 0;
    }
    float sum = 0.0;
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        sum += lm35ReadingsArray[i];
    }
    float average = sum / NUMBER_OF_AVG_SAMPLES;
    return (average * 330.0);
}

char scanKeypad() {
    DigitalOut* rows[4] = {&row1, &row2, &row3, &row4};
    DigitalIn* cols[4] = {&col1, &col2, &col3, &col4};
    
    for (int r = 0; r < 4; r++) {
        row1 = row2 = row3 = row4 = 1;
        *rows[r] = 0;
        
        for (int c = 0; c < 4; c++) {
            if (!(*cols[c])) {
                delay(40);
                if (!(*cols[c])) {
                    while (!(*cols[c]));
                    return keypadMatrix[r][c];
                }
            }
        }
    }
    return '\0';
}

int main() {
    passiveSiren.write(0.0);
    
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        lm35ReadingsArray[i] = lm35.read();
    }

    int printCounter = 0;

    while (true) {
        float tempC = getTemperatureCelsius();
        float potValue = potentiometer.read();
        float tThreshold = 25.0 + (potValue * 12.0);

        float gasRaw = mq2Analog.read();
        float gasScaled = gasRaw * 100.0;

        char key = scanKeypad();

        if (tempC > tThreshold) {
            alarmActive = 1;
            logEvents[logIndex % 5] = (int)tempC;
            logIndex++;
            
            passiveSiren.period(1.0 / 2000.0);
            passiveSiren.write(0.5);
            
            uartUsb.write("Buzzer Activated - Cause: [Temperature]\r\n", 41);
            uartUsb.write("Enter 3-Digit Code to Deactivate\r\n", 35);
            digitCount = 0;

            int lockPrintCounter = 0;

            while (alarmActive == 1) {
                float lockTempC = getTemperatureCelsius();
                float lockPotValue = potentiometer.read();
                float lockTThreshold = 25.0 + (lockPotValue * 12.0);

                lockPrintCounter++;
                if (lockPrintCounter >= 100) {
                    int tWhole = (int)lockTempC;
                    int tFraction = (int)((lockTempC - tWhole) * 10.0);
                    if (tFraction < 0) tFraction = -tFraction;

                    int limitWhole = (int)lockTThreshold;
                    int limitFraction = (int)((lockTThreshold - limitWhole) * 10.0);
                    if (limitFraction < 0) limitFraction = -limitFraction;

                    char lockBuffer[150];
                    sprintf(lockBuffer, "ALARM ACTIVE | Current Temp: %d.%d C | Trigger Limit: %d.%d C\r\n", 
                            tWhole, tFraction, limitWhole, limitFraction);
                    uartUsb.write(lockBuffer, strlen(lockBuffer));
                    lockPrintCounter = 0;
                }

                char lockKey = scanKeypad();
                if (lockKey != '\0') {
                    if (lockKey == '*') {
                        if (digitCount == 3 && inputSequence[0] == '1' && inputSequence[1] == '2' && inputSequence[2] == '3') {
                            alarmActive = 0;
                            passiveSiren.write(0.0);
                            digitCount = 0;
                            uartUsb.write("Alarm Deactivated. System Resumed.\r\n", 36);
                        } else {
                            uartUsb.write("Wrong Code! Resetting...\r\n", 26);
                            digitCount = 0;
                        }
                    } else if (lockKey != '#' && digitCount < 3) {
                        inputSequence[digitCount] = lockKey;
                        digitCount++;
                        
                        char keyMsg[30];
                        sprintf(keyMsg, "Key Pressed: %c\r\n", lockKey);
                        uartUsb.write(keyMsg, strlen(keyMsg));
                    }
                }
                delay(TIME_INCREMENT_MS);
            }
        }

        if (key == '#') {
            uartUsb.write("--- Temperature Event Log ---\r\n", 31);
            for (int i = 0; i < 5; i++) {
                char logMsg[40];
                sprintf(logMsg, "Log %d: %d C\r\n", i + 1, logEvents[i]);
                uartUsb.write(logMsg, strlen(logMsg));
            }
        }

        printCounter++;
        if (printCounter >= 100) {
            int tempWhole = (int)tempC;
            int tempFraction = (int)((tempC - tempWhole) * 10.0);
            if (tempFraction < 0) tempFraction = -tempFraction;

            int threshWhole = (int)tThreshold;
            int threshFraction = (int)((tThreshold - threshWhole) * 10.0);
            if (threshFraction < 0) threshFraction = -threshFraction;

            int gasWhole = (int)gasScaled;

            char buffer[160];
            sprintf(buffer, "System Normal | Temp: %d.%d C (Limit: %d.%d C) | Gas: %d \r\n", 
                    tempWhole, tempFraction, threshWhole, threshFraction, gasWhole);
            uartUsb.write(buffer, strlen(buffer));
            printCounter = 0;
        }

        delay(TIME_INCREMENT_MS);
    }
}
