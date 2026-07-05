#include "smart_home_system.h"

DigitalOut relay(D8);
InterruptIn stopButton(D9);

DigitalOut row1(D0), row2(D1), row3(D2), row4(D3);
DigitalIn col1(D4, PullUp), col2(D5, PullUp), col3(D6, PullUp), col4(D7, PullUp);

int motorState = 0;
int interruptTriggered = 0;

char keypadMatrix[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

void stopButtonISR() {
    relay = 1;
    motorState = 0;
    interruptTriggered = 1;
}

char scanKeypad() {
    DigitalOut* rows[4] = {&row1, &row2, &row3, &row4};
    DigitalIn* cols[4] = {&col1, &col2, &col3, &col4};
    
    for (int r = 0; r < 4; r++) {
        row1 = row2 = row3 = row4 = 1;
        *rows[r] = 0;
        
        for (int c = 0; c < 4; c++) {
            if (!(*cols[c])) {
                ThisThread::sleep_for(50ms);
                if (!(*cols[c])) {
                    while (!(*cols[c]));
                    return keypadMatrix[r][c];
                }
            }
        }
    }
    return '\0';
}

void updateLcdDisplay() {
    lcd_clear();
    if (interruptTriggered == 1) {
        lcd_print("INTERRUPT TRG!");
        lcd_set(0, 1);
        lcd_print("Motor Hard Stop");
    } else {
        lcd_print("Motor Status:");
        lcd_set(0, 1);
        if (motorState == 1) {
            lcd_print("RUNNING");
        } else {
            lcd_print("STOPPED");
        }
    }
}

void smartHomeSystemInit() {
    relay = 1;
    motorState = 0;
    interruptTriggered = 0;
    
    stopButton.mode(PullUp);
    stopButton.fall(&stopButtonISR);
    
    lcd_init();
    updateLcdDisplay();
}

void smartHomeSystemUpdate() {
    if (interruptTriggered == 1) {
        updateLcdDisplay();
        ThisThread::sleep_for(3000ms);
        interruptTriggered = 0;
        updateLcdDisplay();
    }

    char key = scanKeypad();
    if (key != '\0') {
        if (key == '1') {
            relay = 0;
            motorState = 1;
            updateLcdDisplay();
        }
        else if (key == '2') {
            relay = 1;
            motorState = 0;
            updateLcdDisplay();
        }
    }
    ThisThread::sleep_for(20ms);
}
