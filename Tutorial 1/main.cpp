#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

void phaseOneBlink() {
    for (int i = 0; i < 4; i++) {
        led1 = 1; led2 = 1; led3 = 1;
        ThisThread::sleep_for(100ms);
        led1 = 0; led2 = 0; led3 = 0;
        ThisThread::sleep_for(100ms);
    }
}

int main() {
    phaseOneBlink();

    int timer_counter = 0;
    int led1_blink_count = 0;

    while (led1_blink_count < 8) {
        led3 = !led3;

        if (timer_counter % 2 == 0) {
            led2 = !led2;
        }

        if (timer_counter % 4 == 0) {
            led1 = !led1;
            led1_blink_count++;
        }

        timer_counter++;
        ThisThread::sleep_for(250ms);
    }

    led1 = 1;
    led2 = 0;
    led3 = 0;

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
