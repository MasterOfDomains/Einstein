/*! \file encoder.c \brief Quadrature Encoder reader/driver. */
//*****************************************************************************
//
// File Name	: 'encoder.c'
// Title		: Quadrature Encoder reader/driver
// Author		: Pascal Stang - Copyright (C) 2003-2004
// Created		: 2003.01.26
// Revised		: 2004.06.25
// Version		: 0.3
// Target MCU	: Atmel AVR Series
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "../global.h"
#include "encoder.h"

// Program ROM constants

// Global variables
volatile EncoderStateType EncoderState[NUM_ENCODERS];

// Functions

u08 getEncoderNumber(side robotSide)
{
	if (robotSide == LEFT) return 0;
	else return 1;
}


u32 getEncoderTicks(side robotSide)
{
	volatile EncoderStateType ticks = EncoderState[getEncoderNumber(robotSide)];
	return ticks.position;
}

float getDistanceTraveled()
{
	volatile EncoderStateType leftEncoder = EncoderState[getEncoderNumber(LEFT)];
	volatile EncoderStateType rightEncoder = EncoderState[getEncoderNumber(RIGHT)];
	u32 ticksTraveled = (u64) (leftEncoder.position + rightEncoder.position)/2;
	return ((float) ticksTraveled) / TICKS_PER_UNIT;
}

float getDistanceTraveledLeft()
{
	volatile EncoderStateType leftEncoder = EncoderState[getEncoderNumber(LEFT)];
	return ((float) leftEncoder.position);
}

float getDistanceTraveledRight()
{
	volatile EncoderStateType rightEncoder = EncoderState[getEncoderNumber(RIGHT)];
	return ((float) rightEncoder.position);
}

void resetEncoderPositions()
{
	EncoderState[getEncoderNumber(LEFT)].position = 0;
	EncoderState[getEncoderNumber(RIGHT)].position = 0;
}

// initEncoders() initializes hardware and encoder position readings
void initEncoders()
{
	// Joe Rogers
	// To adapt this code to handle single signal encoders
	leftForward = TRUE;
	rightForward = TRUE;

	u08 i;

	// initialize/clear encoder data
	for(i = 0; i < NUM_ENCODERS; i++)
	{
		EncoderState[i].position = 0;
		//EncoderState[i].velocity = 0;		// NOT CURRENTLY USED
	}

	// configure direction and interrupt I/O pins:
	// - for input
	// - apply pullup resistors
	// - any-edge interrupt triggering
	// - enable interrupt

	#ifdef ENC0_SIGNAL
		// set interrupt pins to input and apply pullup resistor
		cbi(ENC0_PHASEA_DDR, ENC0_PHASEA_PIN);
		sbi(ENC0_PHASEA_PORT, ENC0_PHASEA_PIN);
		// set encoder direction pin for input and apply pullup resistor
		//cbi(ENC0_PHASEB_DDR, ENC0_PHASEB_PIN);
		//sbi(ENC0_PHASEB_PORT, ENC0_PHASEB_PIN);
		// configure interrupts for any-edge triggering
		sbi(ENC0_ICR, ENC0_ISCX0);
		cbi(ENC0_ICR, ENC0_ISCX1);
		// enable interrupts
		sbi(IMSK, ENC0_INT);	// ISMK is auto-defined in encoder.h
	#endif
	#ifdef ENC1_SIGNAL
		// set interrupt pins to input and apply pullup resistor
		cbi(ENC1_PHASEA_DDR, ENC1_PHASEA_PIN);
		sbi(ENC1_PHASEA_PORT, ENC1_PHASEA_PIN);
		// set encoder direction pin for input and apply pullup resistor
		//cbi(ENC1_PHASEB_DDR, ENC1_PHASEB_PIN);
		//sbi(ENC1_PHASEB_PORT, ENC1_PHASEB_PIN);
		// configure interrupts for any-edge triggering
		sbi(ENC1_ICR, ENC1_ISCX0);
		cbi(ENC1_ICR, ENC1_ISCX1);
		// enable interrupts
		sbi(IMSK, ENC1_INT);	// ISMK is auto-defined in encoder.h
	#endif
	
	// enable global interrupts
	sei();
}

// encoderOff() disables hardware and stops encoder position updates
void encoderOff(void)
{
	// disable encoder interrupts
	#ifdef ENC0_SIGNAL
		// disable interrupts
		cbi(IMSK, ENC0_INT);	// ISMK is auto-defined in encoder.h
	#endif
	#ifdef ENC1_SIGNAL
		// disable interrupts
		cbi(IMSK, ENC1_INT);	// ISMK is auto-defined in encoder.h
	#endif
	#ifdef ENC2_SIGNAL
		// disable interrupts
		cbi(IMSK, ENC2_INT);	// ISMK is auto-defined in encoder.h
	#endif
	#ifdef ENC3_SIGNAL
		// disable interrupts
		cbi(IMSK, ENC3_INT);	// ISMK is auto-defined in encoder.h
	#endif
	//cli();
}

// encoderGetPosition() reads the current position of the encoder 
s32 encoderGetPosition(u08 encoderNum)
{
	// sanity check
	if(encoderNum < NUM_ENCODERS)
		return EncoderState[encoderNum].position;
	else
		return 0;
}

// encoderSetPosition() sets the current position of the encoder
void encoderSetPosition(u08 encoderNum, s32 position)
{
	// sanity check
	if(encoderNum < NUM_ENCODERS)
		EncoderState[encoderNum].position = position;
	// else do nothing
}

#ifdef ENC0_SIGNAL
//! Encoder 0 interrupt handler
SIGNAL(ENC0_SIGNAL)
{
	// encoder has generated a pulse
	// check the relative phase of the input channels
	// and update position accordingly
	//if( ((inb(ENC0_PHASEA_PORTIN) & (1<<ENC0_PHASEA_PIN)) == 0) ^
	//	((inb(ENC0_PHASEB_PORTIN) & (1<<ENC0_PHASEB_PIN)) == 0) )
	if( ((inb(ENC0_PHASEA_PORTIN) & (1<<ENC0_PHASEA_PIN)) == 0) ^ leftForward )
	{
		EncoderState[0].position++;
	}
	else
	{
		EncoderState[0].position--;
	}
}
#endif

#ifdef ENC1_SIGNAL
//! Encoder 1 interrupt handler
SIGNAL(ENC1_SIGNAL)
{
	// encoder has generated a pulse
	// check the relative phase of the input channels
	// and update position accordingly
	//if( ((inb(ENC1_PHASEA_PORTIN) & (1<<ENC1_PHASEA_PIN)) == 0) ^
	//	((inb(ENC1_PHASEB_PORTIN) & (1<<ENC1_PHASEB_PIN)) == 0) )
	if( ((inb(ENC1_PHASEA_PORTIN) & (1<<ENC1_PHASEA_PIN)) == 0) ^ rightForward )
	{
		EncoderState[1].position++;
	}
	else
	{
		EncoderState[1].position--;
	}
}
#endif

