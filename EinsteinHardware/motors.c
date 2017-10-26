#include "motors.h"

#include <util/delay.h>

#include "../rprintf.h"

#include "hwutils.h"

#ifdef EXTERNAL_MOTOR_CONTROL
#include "../twi.h"
#include "../buffer.h"
#else
// Output Compare Registers
#define OCR_LEFT OCR0A
#define OCR_RIGHT OCR0B
// Port
#define MOTORS_PORT PORTB
// Port Bits
#define LEFT_FORWARD PB2
#define LEFT_BACKWARD PB5
#define RIGHT_FORWARD PB6
#define RIGHT_BACKWARD PB7
#endif

#ifdef EXTERNAL_MOTOR_CONTROL

#define EXT_ADDRESS 0xA0

BOOL distanceReached();
void signalToStopSlave();

float externalGetDistance()
{
	u08 receiveDataLength = 4;
	u08 receiveData[receiveDataLength];


	//i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cMasterReceive(EXT_ADDRESS, receiveDataLength, receiveData);
	i2cWaitForComplete();
	_delay_ms(7);
	
	union
	{
		float fltAmount;
		u08 bytes[4];
	} amountConverter;
	
	amountConverter.bytes[0] = receiveData[0];
	amountConverter.bytes[1] = receiveData[1];
	amountConverter.bytes[2] = receiveData[2];
	amountConverter.bytes[3] = receiveData[3];
	return amountConverter.fltAmount;
}

void externalGo(direction dir, u08 speed)
{
	u08 sendDataLength = 4;
	u08 sendData[sendDataLength];
	sendData[0] = 'G';
	sendData[1] = 'O';

	sendData[2] = dir; // Direction
	sendData[3] = speed; // Speed

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

void externalSpin(side spinSide, u08 speed)
{
	u08 sendDataLength = 4;
	u08 sendData[sendDataLength];
	sendData[0] = 'S';
	sendData[1] = 'P';

	sendData[2] = spinSide; // Side
	sendData[3] = speed; // Speed

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

void externalMove(side moveSide, direction dir, u08 speed, float amount)
{
	// Convert amount to byte array for i2c transfer
	union
	{
		float fltAmount;
		u08 bytes[4];
	} amountConverter;
	amountConverter.fltAmount = amount;

	u08 sendDataLength = 9;
	u08 sendData[sendDataLength];
	sendData[0] = 'M';
	sendData[1] = 'V';

	sendData[2] = moveSide; // Side
	sendData[3] = dir; // Direction
	sendData[4] = amountConverter.bytes[0]; // Amount
	sendData[5] = amountConverter.bytes[1]; // Amount
	sendData[6] = amountConverter.bytes[2]; // Amount
	sendData[7] = amountConverter.bytes[3]; // Amount
	sendData[8] = speed; // Speed

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

void externalTwist(side spinSide, u08 speed, float amount)
{
	u08 sendDataLength = 8;
	u08 sendData[sendDataLength];
	sendData[0] = 'T';
	sendData[1] = 'W';

	sendData[2] = spinSide; // Side
	sendData[3] = amount; // Amount
	sendData[4] = speed; // Speed

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

void externalSoftStop()
{
	u08 sendDataLength = 2;
	u08 sendData[sendDataLength];
	sendData[0] = 'S';
	sendData[1] = 'S';

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

void externalHardStop()
{
	u08 sendDataLength = 2;
	u08 sendData[sendDataLength];
	sendData[0] = 'H';
	sendData[1] = 'S';

	i2cMasterSend(EXT_ADDRESS, sendDataLength, sendData);
	i2cWaitForComplete();
	_delay_ms(7);
}

#endif

void go(direction dir, u08 speed)
{
#ifdef EXTERNAL_MOTOR_CONTROL
	externalGo(dir, speed);
#else
	setSpeed(LEFT, dir, speed);
	setSpeed(RIGHT, dir, speed);
#endif
}

void spin(side spinSide, u08 speed)
{
#ifdef EXTERNAL_MOTOR_CONTROL
	externalSpin(spinSide, speed);
#else
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
#endif
}

void halt(void)
{
#ifdef EXTERNAL_MOTOR_CONTROL
#else
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
#endif
}

void setSpeed(side motorSide, direction motorDir, u08 speed)
{
#ifdef EXTERNAL_MOTOR_CONTROL
#else
	if (motorSide == LEFT)
	{
		if (motorDir == BACKWARD)
		{
			PORT_ON(MOTORS_PORT, LEFT_BACKWARD);
			PORT_OFF(MOTORS_PORT, LEFT_FORWARD);
		}
		else if (motorDir == FORWARD)
		{
			PORT_ON(MOTORS_PORT, LEFT_FORWARD);
			PORT_OFF(MOTORS_PORT, LEFT_BACKWARD);
		}
		OCR_LEFT = speed;
	}
	else if (motorSide == RIGHT)
	{
		if (motorDir == FORWARD)
			{
			PORT_ON(MOTORS_PORT, RIGHT_FORWARD);
			PORT_OFF(MOTORS_PORT, RIGHT_BACKWARD);
			}
		else if (motorDir == BACKWARD)
		{
			PORT_ON(MOTORS_PORT, RIGHT_BACKWARD);
			PORT_OFF(MOTORS_PORT, RIGHT_FORWARD);
		}

		OCR_RIGHT = speed;
	}
#endif
}

void testMotors(void)
{
	
#define SPEED 127
#define TIME 1000

	while(1)
	{
		rprintfProgStrM("Moving Forward");
		rprintfCRLF();
		externalMove(CENTER, FORWARD, SPEED, 32);
		while(!distanceReached());
				
		rprintfProgStrM("Moving Backward");
		rprintfCRLF();
		externalMove(CENTER, BACKWARD, SPEED, 32);
		while(!distanceReached());

		rprintfProgStrM("Moving Forward to be interrupted after 2 secs");
		rprintfCRLF();
		externalMove(CENTER, FORWARD, SPEED, 1000);
		_delay_ms(2000); // Doing something useful
		signalToStopSlave();
		externalSoftStop(); // Ensures stopped and reset
		
		rprintfProgStrM("Should be stopped now for 2 seconds");
		rprintfCRLF();
		rprintfProgStrM("Moved: ");
		rprintfFloat(4, externalGetDistance());
		rprintfCRLF();
		_delay_ms(2000);

		rprintfProgStrM("Moving Backward to be interrupted after 2 secs");
		rprintfCRLF();
		externalMove(CENTER, BACKWARD, SPEED, 1000);
		_delay_ms(2000); // Doing something useful
		signalToStopSlave();
		externalSoftStop(); // Ensures stopped and reset

		rprintfProgStrM("Should be stopped now for 2 seconds");
		rprintfCRLF();
		rprintfProgStrM("Moved: ");
		rprintfFloat(4, externalGetDistance());
		rprintfCRLF();
		_delay_ms(2000);
	}	
}

void initMotors(void)
{
#ifndef EXTERNAL_MOTOR_CONTROL
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
#endif
}

BOOL distanceReached() 
{
	return(PORT_IS_OFF(PORTC, PC2));
}

void signalToStopSlave()
{
	PORT_OFF(PORTC, PC2);
	_delay_ms(100);
}