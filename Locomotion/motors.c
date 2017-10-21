#include "motors.h"

#include <util/delay.h>
#include <stdlib.h>

#include "../rprintf.h"
#include "lutils.h"

#include "lutils.h"
#include "encoder.h"

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

void go(direction dir, u08 speed)
{
	setSpeed(LEFT, dir, speed);
	setSpeed(RIGHT, dir, speed);
}

void spin(side spinSide, u08 speed)
{
	if (spinSide == LEFT)
	{
		setSpeed(LEFT, BACKWARD, speed);
		setSpeed(RIGHT, FORWARD, speed);
	}
	else if (spinSide == RIGHT)
	{
		setSpeed(LEFT, FORWARD, speed);
		setSpeed(RIGHT, BACKWARD, speed);
	}
}

void twist(side spinSide, u08 speed, float amount)
{
	resetEncoderPositions();
	s32 avgDist = 0;
	while (avgDist < amount)
	{
		avgDist = (abs(getDistanceTraveledLeft()) + abs(getDistanceTraveledRight()))/2;
	}
	halt();
	encoderOff();
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

void setSpeed(side wheel, direction motorDir, u08 speed)
{
	if (wheel == LEFT)
	{
		if (motorDir == BACKWARD)
		{
			leftForward = FALSE;
			PORT_ON(MOTORS_PORT, LEFT_BACKWARD);
			PORT_OFF(MOTORS_PORT, LEFT_FORWARD);
		}
		else if (motorDir == FORWARD)
		{
			leftForward = TRUE;
			PORT_ON(MOTORS_PORT, LEFT_FORWARD);
			PORT_OFF(MOTORS_PORT, LEFT_BACKWARD);
		}
		OCR_LEFT = speed;
	}
	else if (wheel == RIGHT)
	{
		if (motorDir == BACKWARD)
		{
			rightForward = FALSE;
			PORT_ON(MOTORS_PORT, RIGHT_BACKWARD);
			PORT_OFF(MOTORS_PORT, RIGHT_FORWARD);
		}
		else if (motorDir == FORWARD)
		{
			rightForward = TRUE;
			PORT_ON(MOTORS_PORT, RIGHT_FORWARD);
			PORT_OFF(MOTORS_PORT, RIGHT_BACKWARD);
		}
		OCR_RIGHT = speed;
	}
}

void testMotorsHalt() {
	rprintf("Halt");
	rprintfCRLF();
	halt();
	_delay_ms(5000);
}


void testEncoders() {
	s32 distLeft = 0;
	s32 distRight = 0;
	
	while (1) {
		if (encoderGetPosition(LEFT_ENCODER) != distLeft) {
			distLeft = encoderGetPosition(LEFT_ENCODER);
			rprintf("Left: %d", distLeft);
			rprintfCRLF();
		}
		if (encoderGetPosition(RIGHT_ENCODER) != distRight) {
			distRight = encoderGetPosition(RIGHT_ENCODER);
			rprintf("Right: %d", distRight);
			rprintfCRLF();
		}
	}
}

void testMotors(void)
{
#define SPEED 200
#define TIME 1000
	while(1)
	{
		rprintf("Left Forward 127");
		rprintfCRLF();
		setSpeed(LEFT, FORWARD, SPEED);
		_delay_ms(TIME * 5);
			
		testMotorsHalt();

		rprintf("Right Forward 127");
		rprintfCRLF();
		setSpeed(RIGHT, FORWARD, SPEED);
		_delay_ms(TIME * 5);
			
		testMotorsHalt();
			
		rprintf("Left Backward 127");
		rprintfCRLF();
		setSpeed(LEFT, BACKWARD, SPEED);
		_delay_ms(TIME * 5);
						
		testMotorsHalt();

		rprintf("Right Backward 127");
		rprintfCRLF();
		setSpeed(RIGHT, BACKWARD, SPEED);
		_delay_ms(TIME * 5);
						
		testMotorsHalt();
						
		/*
		rprintf("Going Forward");
		rprintfCRLF();
		go(FORWARD, SPEED);
		_delay_ms(TIME);
		halt();
		_delay_ms(TIME);

		rprintf("Going Backward");
		rprintfCRLF();
		go(BACKWARD, SPEED);
		_delay_ms(TIME);
		halt();
		_delay_ms(TIME);
			
		rprintf("Spinning Left");
		rprintfCRLF();
		spin(LEFT, SPEED);
		_delay_ms(TIME);
		halt();
		_delay_ms(TIME);

		rprintf("Spinning Right");
		rprintfCRLF();
		spin(RIGHT, SPEED);
		_delay_ms(TIME * 2);
		halt();
		_delay_ms(TIME);
			
		rprintf("Spinning Back (Left)");
		rprintfCRLF();
		spin(LEFT, SPEED);
		_delay_ms(TIME);
		halt();
		_delay_ms(TIME);
		*/
			
		rprintf("Done");
		rprintfCRLF();
	}
}

void move(direction dir, u08 speed, float distance, BOOL stop)
{
	resetEncoderPositions();
	s32 distLeft = 0;
	s32 distRight = 0;
	u32 encoderTicks = distance * TICKS_PER_UNIT;
	go(dir, speed);
	s32 avgDist = 0;
	while (abs(avgDist) < encoderTicks)
	{
		distLeft = getEncoderTicks(LEFT);
		distRight = getEncoderTicks(RIGHT);
		avgDist = (distLeft + distRight)/2;
	}
	if (stop) halt();
	encoderOff();
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