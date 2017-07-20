#include "motors.h"

#include <util/delay.h>

#include "../rprintf.h"

#include "utils.h"
#include "encoder.h"

// Output Compare Registers
#define OCR_LEFT OCR0A
#define OCR_RIGHT OCR0B
// Port
#define MOTORS_PORT PORTB
// Port Bits
#define LEFT_FORWARD PB2
#define LEFT_PWM PB3
#define RIGHT_PWM PB4
#define LEFT_BACKWARD PB5
#define RIGHT_FORWARD PB6
#define RIGHT_BACKWARD PB7

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
}

void setSpeed(side motorSide, direction motorDir, u08 speed)
{
	if (motorSide == LEFT)
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
	else if (motorSide == RIGHT)
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

void testMotors(void)
{
	while(1)
	{
		rprintf("F");
		rprintfCRLF();
#define SPEED 200
#define TIME 1000

		while(1)
		{
			go(FORWARD, SPEED);
			_delay_ms(TIME);
			halt();
			_delay_ms(TIME / 2);

			go(BACKWARD, SPEED);
			_delay_ms(TIME);
			halt();
			_delay_ms(TIME / 2);
			
			spin(LEFT, SPEED);
			_delay_ms(TIME);
			halt();
			_delay_ms(TIME / 2);

			spin(RIGHT, SPEED);
			_delay_ms(TIME * 2);
			halt();
			_delay_ms(TIME / 2);
			
			spin(LEFT, SPEED);
			_delay_ms(TIME);
			halt();
			_delay_ms(TIME / 2);
		}

		while(1);
		/*
		_delay_ms(3000);
		halt();
		_delay_ms(1000);

		//debugLEDoff();
		rprintf("B");
		setSpeed(LEFT, BACKWARD, 255);
		setSpeed(RIGHT, BACKWARD, 255);
		_delay_ms(1000);
		halt();
		_delay_ms(1000);
		*/
	}
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