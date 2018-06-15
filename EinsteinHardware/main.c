#include "hwglobal.h"

#include "../twi.h"
#include "../rprintf.h"
#include "../a2d.h"
#include "../utils.h"

#include "uart2.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "motors.h"
#include "channels.h"
#include "hwutils.h"
#include "arm.h"
#include "servo8t.h"
#include "compass.h"
#include "head.h"

void initRobot(void);
void initPorts(void);
void signalStart();
void testNewArmCode();
void demo();

void wait(u16 miliseconds)
{
    for (u16 i = 0; i < miliseconds; i++) {
        _delay_ms(1);
    }
}

int main(void)
{
    _delay_ms(3000);
    initRobot();

#define PAUSE 1000

    moveArmToPos(HOME);
    _delay_ms(PAUSE);

    //#define HALF_SWING 15
    //side lastHalfSwingSide = LEFT;
    //for (u08 i = 0; i < HALF_SWING; i++) {
    //rotateArm(lastHalfSwingSide);
    //}
    while (1) {
        //side newSide = oppositeSide(lastHalfSwingSide);
        //for (u08 i = 0; i < HALF_SWING * 2; i++) {
        //rotateArm(newSide);
        //}
        //lastHalfSwingSide = newSide;

        headLights(TRUE);
        blob *bestBlob = getBestBlob(NULL);
        headLights(FALSE);
        if (bestBlob != NULL) {
            rprintfCRLF();
            rprintfProgStrM("BEST BLOB:");
            displayBlob(bestBlob);
            rprintfCRLF();
            rprintfCRLF();
        } else {
            rprintfProgStrM("NO BEST BLOB:");
            rprintfCRLF();
        }

        _delay_ms(5000);
    }

    headLights(TRUE);

#define SPEED 100
    move(FORWARD, SPEED, 32);
    _delay_ms(500);
    move(BACKWARD, SPEED, 32);
    _delay_ms(500);
#define TWIST_AMOUNT 32
    twist(LEFT, SPEED, TWIST_AMOUNT);
    _delay_ms(500);
    twist(RIGHT, SPEED, TWIST_AMOUNT * 2);
    _delay_ms(500);
    twist(LEFT, SPEED, TWIST_AMOUNT);

    testNewArmCode();

    return 0;
}

void gripperDemo()
{
    grip(GRIPPER_CLOSED);
    grip(0);
    rotateGripper(GRIPPER_VERTIAL_RIGHT);
    grip(GRIPPER_CLOSED);
    grip(0);

    rotateGripper(GRIPPER_VERTICAL_LEFT);
    grip(GRIPPER_CLOSED);
    grip(0);

    rotateGripper(GRIPPER_LEVEL);
    grip(GRIPPER_OPEN);
    grip(0);
    _delay_ms(PAUSE);
}

void testNewArmCode()
{
    while (1) {
        moveArmToPos(HOME);
        _delay_ms(PAUSE);
        moveArmToPos(CROUCH);
        gripperDemo();
#define ROTATE_DISTANCE 30
        for (u08 i = 0; i < ROTATE_DISTANCE; i++) {
            rotateArm(LEFT);
        }
        for (u08 i = 0; i < ROTATE_DISTANCE * 2; i++) {
            rotateArm(RIGHT);
        }
        for (u08 i = 0; i < ROTATE_DISTANCE; i++) {
            rotateArm(LEFT);
        }
        _delay_ms(PAUSE);
    }
}

void initRobot(void)
{
    initPorts();
    signalStart();
    armOff();

    uartInit();

    setRemoteComm(USB);
    rprintfCRLF();
    rprintfProgStrM("Starting...");
    rprintfCRLF();
#ifndef SIMULATOR
    a2dInit();
    a2dSetPrescaler(ADC_PRESCALE_DIV32);
    a2dSetReference(ADC_REFERENCE_AVCC);

    i2cInit();

#ifndef MAIN_BOARD_ONLY
#ifdef COMPASS
    initCompass();
#endif
#ifndef BREADBOARD
    initMotors();
    initServo8t();
    initHead();
    initArm();
    armOn();
#endif
#endif
#endif
    rprintfProgStrM("Robot Initialized.");
    rprintfCRLF();
}

static void dutyCycleLED(u08 dutyCycle, u08 pulseWidth, u16 *currentTime)
{
    debugLEDoff();
    for (u16 tick = 0; tick < (pulseWidth - dutyCycle); tick++) {
        _delay_ms(1);
        (*currentTime)++;
    }
    debugLEDon();
    for (u16 i = 0; i < dutyCycle; i++) {
        _delay_ms(1);
        (*currentTime)++;
    }
}

void signalStart()
{
    static const int NUMBER_OF_WINKS = 3;
    for (u08 wink = 0; wink < NUMBER_OF_WINKS; wink++) {
        u16 currentTime = 0;
        const u16 endTime = 1000;
        const u08 pulseWidth = 20;
        while (currentTime < endTime) {
            u08 dutyCycle = currentTime/100;
            dutyCycleLED(dutyCycle, pulseWidth, &currentTime);
        }
        _delay_ms(500);
    }
    debugLEDoff();
}

void initPorts(void)
{

    // ANALOG PORTS
    DDRA = 0b00000000;
    //       ||||||||
    //       |||||||\___0: IR 1, Pin 40
    //       ||||||\____1:
    //       |||||\_____2:
    //       ||||\______3:
    //       |||\_______4:
    //       ||\________5:
    //       |\_________6: Left Head Sonar, Pin 34
    //       \__________7: Right Head Sonar, Pin 33

    PORTA = 0b00000000; //make sure pull-up resistors are turned off

    DDRB = 0b11111111;
    //       ||||||||
    //       |||||||\___0: Debug LED, OUTPUT, Pin 1
    //       ||||||\____1: Clock Out, Pin 2
    //       |||||\_____2:							/ Left Motor FORWARD, OUTPUT, Pin 3
    //       ||||\______3:							/ Left Motor PWM (OC0A), OUTPUT, Pin 4
    //       |||\_______4:							/ Right Motor PWM (OC0B), OUTPUT, Pin 5
    //       ||\________5:							/ Left Motor BACKWARD, OUTPUT, Pin 6
    //       |\_________6: Headlights on			/ Right Motor FORWARD, OUTPUT, Pin 7
    //       \__________7: Read head sonar enable	/ Right Motor BACKWARD, OUTPUT, Pin 8

    DDRC = 0b11111011;
    //       ||||||||
    //       |||||||\___0: I2C SCL, I/O,
    //       ||||||\____1: I2C SDA, I/O,
    //       |||||\_____2: Interrupter, Pin 24
    //       ||||\______3: Battery Monitor, Pin 25
    //       |||\_______4:
    //       ||\________5:
    //       |\_________6:
    //       \__________7: Arm Power, Pin 29

    DDRD = 0b11111010;
    //       ||||||||
    //       |||||||\___0: UART 0 Rx (RXD0), INPUT, Pin 14
    //       ||||||\____1: UART 0 Tx (TXD0), OUTPUT, Pin 15
    //       |||||\_____2: UART 1 Rx (RXD1), INPUT, Pin 16
    //       ||||\______3: UART 1 Tx (TXD1), OUTPUT, Pin 17
    //       |||\_______4: UART Select, OUTPUT, Pin 18
    //       ||\________5: UART Select, OUTPUT, Pin 19
    //       |\_________6: UART Select, OUTPUT, Pin 20
    //       \__________7: UART Select, OUTPUT, Pin 21
}
