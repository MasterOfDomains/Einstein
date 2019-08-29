#include "motors.h"

#include <util/delay.h>
#include <stdlib.h>

#include "../rprintf.h"
#include "lutils.h"

#include "lutils.h"
#include "encoder.h"

#define INTERRUPTER_PORT PORTB
#define INTERRUPTER_PORT_IN PINB
#define INTERRUPTER_DDR DDRB
#define INTERRUPTER_BIT PB0

// Output Compare Registers
#define OCR_LEFT OCR0A
#define OCR_RIGHT OCR0B
// Port
#define MOTORS_PORT PORTC
// Port Bits
#define LEFT_FORWARD PC1
#define LEFT_PWM PD6
#define RIGHT_PWM PD5
#define LEFT_BACKWARD PC0
#define RIGHT_FORWARD PC3
#define RIGHT_BACKWARD PC2

void signalDistanceTraversed();
BOOL isInterrupt();
void reset();

void go(direction dir, u08 speed)
{
    rprintf("go speed=%d", speed); rprintfCRLF();
    if (dir == FORWARD) {
        rprintfProgStrM("  /\\"); rprintfCRLF();
        rprintfProgStrM(" /  \\"); rprintfCRLF();
        rprintfProgStrM("/    \\"); rprintfCRLF();
        rprintfProgStrM("  ||"); rprintfCRLF();
        rprintfProgStrM("  ||"); rprintfCRLF();
    } else if (dir == BACKWARD) {
        rprintfProgStrM("  ||"); rprintfCRLF();
        rprintfProgStrM("  ||"); rprintfCRLF();
        rprintfProgStrM("\\    /"); rprintfCRLF();
        rprintfProgStrM(" \\  /"); rprintfCRLF();
        rprintfProgStrM("  \\/"); rprintfCRLF();
    }
    setSpeed(LEFT, dir, speed);
    setSpeed(RIGHT, dir, speed);
}

void spin(side spinSide, u08 speed)
{
    if (spinSide == LEFT) {
        setSpeed(LEFT, BACKWARD, speed);
        setSpeed(RIGHT, FORWARD, speed);
    } else if (spinSide == RIGHT) {
        setSpeed(LEFT, FORWARD, speed);
        setSpeed(RIGHT, BACKWARD, speed);
    }
}

void move(direction dir, u08 speed, float distance, BOOL stop)
{
    rprintfCRLF();
    rprintf("move distance="); rprintfFloat(4, distance); rprintfCRLF();
    reset();
    float distLeft = 0;
    float distRight = 0;
    float encoderTicks = distance;
    go(dir, speed);
    float avgDist = 0;
    while (fabs(avgDist) < fabs(encoderTicks) && !isInterrupt()) {
#ifdef TICKS_PER_UNIT
        distLeft = getEncoderTicks(LEFT) / TICKS_PER_UNIT;
        distRight = getEncoderTicks(RIGHT) / TICKS_PER_UNIT;
#else
        distLeft = getEncoderTicks(LEFT);
        distRight = getEncoderTicks(RIGHT);
#endif
        avgDist = (distLeft + distRight)/2;
    }
    if (stop) {
        halt();
    }
    signalDistanceTraversed();
}

void twist(side spinSide, u08 speed, float amount)
{
    reset();
    float avgDist = 0;
    spin(spinSide, speed);
    while (avgDist < amount && !isInterrupt()) {
        avgDist = (fabs(getDistanceTraveledLeft()) + fabs(getDistanceTraveledRight()))/2;
    }
    halt();
    signalDistanceTraversed();
}

void setSpeed(side wheel, direction motorDir, u08 speed)
{
    if (wheel == LEFT) {
        if (motorDir == BACKWARD) {
            leftForward = FALSE;
            PORT_ON(MOTORS_PORT, LEFT_BACKWARD);
            PORT_OFF(MOTORS_PORT, LEFT_FORWARD);
        } else if (motorDir == FORWARD) {
            leftForward = TRUE;
            PORT_ON(MOTORS_PORT, LEFT_FORWARD);
            PORT_OFF(MOTORS_PORT, LEFT_BACKWARD);
        }
        OCR_LEFT = speed;
    } else if (wheel == RIGHT) {
        if (motorDir == BACKWARD) {
            rightForward = FALSE;
            PORT_ON(MOTORS_PORT, RIGHT_BACKWARD);
            PORT_OFF(MOTORS_PORT, RIGHT_FORWARD);
        } else if (motorDir == FORWARD) {
            rightForward = TRUE;
            PORT_ON(MOTORS_PORT, RIGHT_FORWARD);
            PORT_OFF(MOTORS_PORT, RIGHT_BACKWARD);
        }
        OCR_RIGHT = speed;
    }
}

void halt(void)
{
    // Hard Stop
    MOTORS_PORT |= (1 << LEFT_FORWARD);
    MOTORS_PORT |= (1 << LEFT_BACKWARD);
    MOTORS_PORT |= (1 << RIGHT_FORWARD);
    MOTORS_PORT |= (1 << RIGHT_BACKWARD);

    /*
    // Soft Stop
    MOTORS_PORT &= ~(1 << LEFT_FORWARD);
    MOTORS_PORT &= ~(1 << LEFT_BACKWARD);
    MOTORS_PORT &= ~(1 << RIGHT_FORWARD);
    MOTORS_PORT &= ~(1 << RIGHT_BACKWARD);
    */

    setSpeed(LEFT, MIDDLE, 0);
    setSpeed(RIGHT, MIDDLE, 0);
    leftForward = FALSE;
    rightForward = FALSE;
}

void initMotors(void)
{
    TCCR0A = 0;
    TCCR0B = 0;

    // Set PWM, Phase Correct
    TCCR0A |= (1 << WGM00);

    // Use internal Clock Source w/ no pre-scaling
    TCCR0B |= (1 << CS00);

    OCR_LEFT = 0;
    OCR_RIGHT = 0;

    // Enable PWM for both pins.
    TCCR0A |= (1 << COM0A1) | (1 << COM0B1);
}

void testMotors()
{
#define MOVE_TIME 3000
#define START_DELAY 500
#define SPEED 175

    while (1) {
        rprintfProgStrM("Moving Forward"); rprintfCRLF();
        _delay_ms(START_DELAY);
        go(FORWARD, SPEED);
        _delay_ms(MOVE_TIME);
        halt();
        rprintfProgStrM("Moving Backward"); rprintfCRLF();
        _delay_ms(START_DELAY);
        go(BACKWARD, SPEED);
        _delay_ms(MOVE_TIME);
        halt();
    }
}

void testMotorsAndEncoders()
{
#define START_DELAY 500
#define SPEED 175
#define DISTANCE 32
#define PAUSE 5000

    while (1) {
        rprintfProgStrM("Moving Forward"); rprintfCRLF();
        _delay_ms(START_DELAY);
        move(FORWARD, SPEED, DISTANCE, TRUE);
        _delay_ms(PAUSE);
        rprintfProgStrM("Forward Distance: "); rprintfFloat(4, getDistanceTraveled()); rprintfCRLF();
        _delay_ms(START_DELAY);
        rprintfProgStrM("Moving Backward"); rprintfCRLF();
        _delay_ms(START_DELAY);
        move(BACKWARD, SPEED, DISTANCE, TRUE);
        _delay_ms(PAUSE);
        rprintfProgStrM("Backward Distance: "); rprintfFloat(4, getDistanceTraveled()); rprintfCRLF();
        _delay_ms(START_DELAY);
    }
}

void testMotorsHalt()
{
    rprintfProgStrM("Halt"); rprintfCRLF();
    halt();
    _delay_ms(5000);
}

void testEncoders()
{
    s32 distLeft = 0;
    s32 distRight = 0;

    while (1) {
        if (encoderGetPosition(LEFT_ENCODER) != distLeft) {
            distLeft = encoderGetPosition(LEFT_ENCODER);
            rprintf("Left: %d", distLeft); rprintfCRLF();
        }
        if (encoderGetPosition(RIGHT_ENCODER) != distRight) {
            distRight = encoderGetPosition(RIGHT_ENCODER);
            rprintf("Right: %d", distRight); rprintfCRLF();
        }
    }
}

BOOL isInterrupt()
{
    return PORT_IS_OFF(INTERRUPTER_PORT_IN, INTERRUPTER_BIT);
}

void signalDistanceTraversed()
{
    if (!isInterrupt()) {
        sbi(INTERRUPTER_DDR, INTERRUPTER_BIT); // Set to output
        PORT_OFF(INTERRUPTER_PORT, INTERRUPTER_BIT);
    }
}

void reset()
{
    cbi(INTERRUPTER_DDR, INTERRUPTER_BIT); // Set to input
    cbi(INTERRUPTER_PORT, INTERRUPTER_BIT); // Disable pull-up
    resetEncoderPositions();
}