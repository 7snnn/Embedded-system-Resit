#include "mbed.h"

DigitalIn btn1(D2);
DigitalIn btn2(D3);
DigitalIn btn3(D4);
DigitalIn btn4(D5);

DigitalOut ledGreen(LED1);
DigitalOut ledBlue(LED2);
DigitalOut ledRed(LED3);

int readInput() {
    if (btn1.read() == 1) { ThisThread::sleep_for(20ms); if (btn1.read() == 1) { while(btn1.read() == 1); return 1; } }
    if (btn2.read() == 1) { ThisThread::sleep_for(20ms); if (btn2.read() == 1) { while(btn2.read() == 1); return 2; } }
    if (btn3.read() == 1) { ThisThread::sleep_for(20ms); if (btn3.read() == 1) { while(btn3.read() == 1); return 3; } }
    if (btn4.read() == 1) { ThisThread::sleep_for(20ms); if (btn4.read() == 1) { while(btn4.read() == 1); return 0; } }
    return -1;
}

int main() {
    btn1.mode(PullDown);
    btn2.mode(PullDown);
    btn3.mode(PullDown);
    btn4.mode(PullDown);

    ledGreen = 0;
    ledBlue = 0;
    ledRed = 1;

    int wrong_attempts = 0;
    int warningJustEnded = 0;

    int pass[3] = {1, 2, 3};
    int admin[3] = {0, 0, 0};
    int input[3] = {-1, -1, -1};

    Timer totalLockdownTimer;
    Timer ledToggleTimer;

    while (true) {
        int digit_count = 0;

        while (digit_count < 3) {
            int pressed_key = readInput();
            if (pressed_key != -1) {
                input[digit_count] = pressed_key;
                digit_count++;
            }
            ThisThread::sleep_for(10ms);
        }

        int is_pass_correct = 1;

        for (int i = 0; i < 3; i++) {
            if (input[i] != pass[i]) is_pass_correct = 0;
        }

        if (is_pass_correct == 1) {
            wrong_attempts = 0;
            warningJustEnded = 0;
            ledRed = 0;
            ledGreen = 1;
            ThisThread::sleep_for(5000ms);
            ledGreen = 0;
            ledRed = 1;
        } else {
            if (warningJustEnded == 1) {
                wrong_attempts = 3;
            } else {
                wrong_attempts++;
            }
            warningJustEnded = 0;

            if (wrong_attempts == 2) {
                for (int i = 0; i < 20; i++) {
                    ledRed = !ledRed;
                    ThisThread::sleep_for(500ms);
                }
                ledRed = 1;
                warningJustEnded = 1;
            }
            else if (wrong_attempts >= 3) {
                ledBlue = 1;
                warningJustEnded = 0;

                int admin_digit_count = 0;
                int admin_input[3] = {-1, -1, -1};

                totalLockdownTimer.reset();
                totalLockdownTimer.start();
                ledToggleTimer.reset();
                ledToggleTimer.start();

                while (totalLockdownTimer.elapsed_time() < 120s) {
                    if (ledToggleTimer.elapsed_time() >= 500ms) {
                        ledRed = !ledRed;
                        ledToggleTimer.reset();
                    }

                    int immediate_press = -1;
                    if (btn1.read() == 1) immediate_press = 1;
                    else if (btn2.read() == 1) immediate_press = 2;
                    else if (btn3.read() == 1) immediate_press = 3;
                    else if (btn4.read() == 1) immediate_press = 0;

                    if (immediate_press != -1) {
                        ThisThread::sleep_for(20ms);
                        while (btn1.read() == 1 || btn2.read() == 1 || btn3.read() == 1 || btn4.read() == 1);

                        admin_input[0] = immediate_press;
                        admin_digit_count = 1;

                        while (admin_digit_count < 3) {
                            int pressed_key = readInput();
                            if (pressed_key != -1) {
                                admin_input[admin_digit_count] = pressed_key;
                                admin_digit_count++;
                            }
                            ThisThread::sleep_for(10ms);
                        }

                        int verify_admin = 1;
                        for (int k = 0; k < 3; k++) {
                            if (admin_input[k] != admin[k]) verify_admin = 0;
                        }

                        if (verify_admin == 1) {
                            wrong_attempts = 0;
                            break;
                        } else {
                            admin_digit_count = 0;
                        }
                    }

                    ThisThread::sleep_for(10ms);
                }

                totalLockdownTimer.stop();
                ledToggleTimer.stop();
                ledBlue = 0;
                ledRed = 1;
            }
        }

        ThisThread::sleep_for(50ms);
    }
}
