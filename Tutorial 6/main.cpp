#include "mbed.h"
#include "arm_book_lib.h"
#include "lcd.h"

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
int logEvents[5] = {0, 0, 0, 0, 0}, logIndex = 0;

char keypadMatrix[4][4] = {
    {'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'}
};
char inputSequence[3] = {'\0', '\0', '\0'};
int digitCount = 0, alarmActive = 0;

float getTemperatureCelsius() {
    lm35ReadingsArray[lm35SampleIndex] = lm35.read();
    lm35SampleIndex++;
    if (lm35SampleIndex >= NUMBER_OF_AVG_SAMPLES) lm35SampleIndex = 0;
    float sum = 0.0;
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) sum += lm35ReadingsArray[i];
    return ((sum / NUMBER_OF_AVG_SAMPLES) * 330.0);
}

char scanKeypad() {
    DigitalOut* rows[4] = {&row1, &row2, &row3, &row4};
    DigitalIn* cols[4] = {&col1, &col2, &col3, &col4};
    for (int r = 0; r < 4; r++) {
        row1 = row2 = row3 = row4 = 1; *rows[r] = 0;
        for (int c = 0; c < 4; c++) {
            if (!(*cols[c])) {
                delay(40);
                if (!(*cols[c])) { while (!(*cols[c])); return keypadMatrix[r][c]; }
            }
        }
    }
    return '\0';
}

int main() {
    passiveSiren.write(0.0); lcd_init();
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        lm35ReadingsArray[i] = lm35.read(); delay(TIME_INCREMENT_MS);
    }
    lcd_clear(); lcd_set(0, 0); lcd_print("Enter Code to");
    lcd_set(0, 1); lcd_print("Deactivate Alarm");

    int printCounter = 0, alarmCooldownCounter = 0, activeDisplayMode = 0;

    while (true) {
        float tempC = getTemperatureCelsius(), potValue = potentiometer.read();
        float tThreshold = potValue * 100.0;
        float gasScaled = mq2Analog.read() * 100.0;
        char key = scanKeypad();

        if (alarmCooldownCounter > 0) alarmCooldownCounter--;
        if (key == '2' || key == '3') { activeDisplayMode = key - '0'; printCounter = 100; }

        int gasDetected = (gasScaled > tThreshold) ? 1 : 0;
        int tempWarning = (tempC > tThreshold) ? 1 : 0;

        if ((tempWarning || gasDetected) && alarmCooldownCounter == 0) {
            alarmActive = 1; logEvents[logIndex % 5] = (int)tempC; logIndex++;
            passiveSiren.period(1.0 / 2000.0); passiveSiren.write(0.5);
            
            lcd_clear(); lcd_set(0, 0);
            if (tempWarning && gasDetected) { lcd_print("TEMP & GAS"); lcd_set(0, 1); lcd_print("CRITICAL WARNING"); }
            else if (tempWarning) { lcd_print("TEMP WARNING!"); lcd_set(0, 1); lcd_print("EXCEEDED LIMIT"); }
            else { lcd_print("GAS WARNING!"); lcd_set(0, 1); lcd_print("LEAK DETECTED"); }
            
            delay(1500); lcd_clear(); lcd_set(0, 0); lcd_print("Enter Code:"); digitCount = 0;

            int lockPrintCounter = 0;

            while (alarmActive == 1) {
                float lockTempC = getTemperatureCelsius(), lockPotValue = potentiometer.read();
                float lockTThreshold = lockPotValue * 100.0;
                float lockGasScaled = mq2Analog.read() * 100.0;

                lockPrintCounter++;
                if (lockPrintCounter >= 100) {
                    int tW = (int)lockTempC, tF = (int)((lockTempC - tW) * 10.0);
                    int thW = (int)lockTThreshold, thF = (int)((lockTThreshold - thW) * 10.0);
                    if (tF < 0) tF = -tF; if (thF < 0) thF = -thF;

                    char lockBuffer[160];
                    sprintf(lockBuffer, "ALARM ACTIVE | Temp: %d.%d C | Gas: %d | Limit: %d.%d\r\n", 
                            tW, tF, (int)lockGasScaled, thW, thF);
                    uartUsb.write(lockBuffer, strlen(lockBuffer));
                    lockPrintCounter = 0;
                }

                char lockKey = scanKeypad();
                if (lockKey != '\0') {
                    if (lockKey == '*') {
                        if (digitCount == 3 && inputSequence[0] == '1' && inputSequence[1] == '2' && inputSequence[2] == '3') {
                            alarmActive = 0; passiveSiren.write(0.0); activeDisplayMode = 0; digitCount = 0;
                            lcd_clear(); lcd_set(0, 0); lcd_print("Code Correct");
                            delay(1000); lcd_clear(); lcd_set(0, 0); lcd_print("Enter Code to");
                            lcd_set(0, 1); lcd_print("Deactivate Alarm");
                            
                            for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
                                lm35ReadingsArray[i] = lm35.read(); delay(TIME_INCREMENT_MS);
                            }
                            alarmCooldownCounter = 300;
                        } else {
                            lcd_clear(); lcd_set(0, 0); lcd_print("Wrong Code!"); delay(1000);
                            lcd_clear(); lcd_set(0, 0); lcd_print("Enter Code:"); digitCount = 0;
                        }
                    } else if (lockKey != '#' && digitCount < 3) {
                        inputSequence[digitCount++] = lockKey;
                        lcd_set(digitCount - 1, 1); lcd_print("*");
                    }
                }
                delay(TIME_INCREMENT_MS);
            }
        } else {
            passiveSiren.write(0.0);
            if (key == '#') {
                for (int i = 0; i < 5; i++) {
                    char logMsg[40]; sprintf(logMsg, "Log %d: %d C\r\n", i + 1, logEvents[i]);
                    uartUsb.write(logMsg, strlen(logMsg));
                }
            }
            printCounter++;
            if (printCounter >= 100) {
                int tW = (int)tempC, tF = (int)((tempC - tW) * 10.0);
                int thW = (int)tThreshold, thF = (int)((tThreshold - thW) * 10.0);
                if (tF < 0) tF = -tF; if (thF < 0) thF = -thF;

                if (activeDisplayMode == 2) {
                    lcd_clear(); lcd_set(0, 0); char gStr[16]; sprintf(gStr, "Gas Level: %d", (int)gasScaled);
                    lcd_print(gStr); lcd_set(0, 1); lcd_print("Gas Status: OK");
                } else if (activeDisplayMode == 3) {
                    lcd_clear(); lcd_set(0, 0); char tStr[16]; sprintf(tStr, "Temp: %d.%d C", tW, tF);
                    lcd_print(tStr); lcd_set(0, 1); char hStr[16]; sprintf(hStr, "Thresh: %d.%d C", thW, thF);
                    lcd_print(hStr);
                }

                char buffer[160];
                sprintf(buffer, "Temperature = %d.%d C  |  Gas Level = %d  |  Current Threshold = %d.%d  |  Status = Normal\r\n", 
                        tW, tF, (int)gasScaled, thW, thF);
                uartUsb.write(buffer, strlen(buffer));
                printCounter = 0;
            }
        }
        delay(TIME_INCREMENT_MS);
    }
}
