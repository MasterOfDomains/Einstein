// HMC6352

// 0 = North (3600)
// 1800 = South
// 900 = East
// 2700 = West

#include <util/delay.h>

#include "compass.h"

#include "../twi.h"
#include "../rprintf.h"
#include "../utils.h"

static u16 homeHeading;

u16 getHeadingDist(u16 heading, side headingSide)
{
	u16 distance = 0;
	u16 currHeading = readCompass();
	if (headingSide == LEFT)
	{
		if (heading > currHeading)
			distance = currHeading + (360 - heading);
		else if (heading < currHeading)
			distance = currHeading - heading;
	}
	else // RIGHT
	{
		if (heading < currHeading)
			distance = heading + (360 - currHeading);
		else if (heading > currHeading)
			distance = heading - currHeading;
	}
	return distance;
}

side getHeadingSide(u16 heading)
{
	side returnVal = CENTER;
	u16 distLeft = getHeadingDist(heading, 	LEFT);
	u16 distRight = getHeadingDist(heading, RIGHT);
	rprintf("distLeft=%d distRight=%d\n\r", distLeft, distRight);
	if (distLeft < distRight)
		returnVal = LEFT;
	else if (distRight < distLeft)
		returnVal = RIGHT;
	return returnVal;
}

u16 setHomeHeading(void)
{
	homeHeading = readCompass();
	rprintf("homeHeading: %d\n\r", homeHeading);
	return homeHeading;
}

BOOL headingReached(u16 startHeading, u16 currHeading, u16 destHeading, side spinSide)
{
	BOOL returnVal = FALSE;
	BOOL crossZero = FALSE;
	BOOL zeroCrossed = FALSE;
	// Start and Dest Headings
	if (spinSide == RIGHT)
	{
		if (destHeading < startHeading)
			crossZero = TRUE;
	}
	else if (spinSide == LEFT)
	{
		if  (destHeading > startHeading)
			crossZero = TRUE;
	}
	//rprintf("hR: crossZero=%d start=%d dest=%d\n\r", crossZero, startHeading, destHeading);
	// Start and Curr Headings
	if (spinSide == RIGHT) //  && crossZero
	{
		//if (currHeading < calcHeading(startHeading, LEFT, 5))
		if (currHeading < startHeading)
			zeroCrossed = TRUE;
	}
	else if (spinSide == LEFT) // && crossZero
	{
		//if (currHeading > calcHeading(startHeading, RIGHT, 5))
		if (currHeading > startHeading)
			zeroCrossed = TRUE;
	}
	//rprintf("zeroCrossed=%d start=%d curr=%d\n\r", zeroCrossed, startHeading, currHeading);

	/*	Cross-Zero Truth Table
	 *----------*-----------*---------------*
	 |	Cross	|	Zero	|	Condition	|
	 |  Zero	|	Crossed	|				|
	 |----------*-----------*---------------|
	 |		0	|		1	|		A		|
	 |		1	|		0	|	Not Done	|
	 |		1	|		1	|		B		|
	 |		0	|		0	|		B		|
	 *----------*-----------*---------------*
	 */
	if (!crossZero && zeroCrossed) // A
		returnVal = TRUE;
	else if (!crossZero || zeroCrossed) // B
	{
		//rprintf("AAA: crossZero=%d zeroCrossed=%d\n\r", crossZero, zeroCrossed);
		// Curr and Dest Headings
		if (spinSide == RIGHT)
		{
			if (currHeading >= destHeading)
				returnVal = TRUE;
		}
		else if (spinSide == LEFT)
		{
			if  (currHeading <= destHeading)
				returnVal = TRUE;
		}
	}
	return returnVal;
}

u16 calcHeading(s16 startHeading, side robotSide, s16 amount)
{
	u16 returnVal;
	s16 endHeading;
	if (robotSide == LEFT)
	{
		endHeading = startHeading - amount;
		if (endHeading < 0)
			endHeading = 360 + endHeading;
	}
	else
	{
		endHeading = startHeading + amount;
		//rprintf("startHeading=%d\n\r", startHeading);
		//rprintf("1 endHeading=%d\n\r", endHeading);
		if (endHeading > MAX_HEADING)
			endHeading = endHeading - 360;
		//rprintf("2 endHeading=%d\n\r", endHeading);
	}
	//rprintf("3 endHeading=%d\n\r", endHeading);
	returnVal = (u16)endHeading;
	//rprintf("4 endHeading=%d\n\r", endHeading);
	return returnVal;
}

u16 readCompass()
{
	u08 SEND_DATA_LENGTH = 1;
	u08 RETURN_DATA_LENGTH = 2;

	u08 sendData = 'A';
	u08 readBuffer[RETURN_DATA_LENGTH];
	u16 heading, headingMSB, headingLSB;

	for (u08 i = 0; i < RETURN_DATA_LENGTH; i++)
		readBuffer[i] = 0;

	i2cMasterSend(0x42, SEND_DATA_LENGTH, &sendData);
	_delay_ms(10);
	i2cMasterReceive(0x42, RETURN_DATA_LENGTH, &readBuffer[0]);
	
	headingMSB = readBuffer[0];
	headingLSB = readBuffer[1];
	heading = headingMSB << 8;
	heading = heading + headingLSB;
	heading /= 10;
	return heading;
}

void testCompass()
{
	while (TRUE)
	{
		rprintf("Compass = %d\n\r", readCompass());
		_delay_ms(1000);
	}
}

u16 getHomeHeading(void)
{
	return homeHeading;
}

void calibrateCompass(void)
{
	u08 command;
	command = 'C';
	rprintfProgStrM("Calibrating\n\r");
	i2cMasterSend(0x42, 1, &command);

	for (u08 counter = 60; counter > 0; counter--)
	{
		rprintf("%d ", counter);
		_delay_ms(1000);
	}
	command = 'E';
	i2cMasterSend(0x42, 1, &command);
	_delay_ms(15);
	rprintfProgStrM("Calibration Complete\n\r");
	testCompass();
}

void initCompass()
{
	i2cInit();
	u08 sendData = 'O'; // Update Bridge Offsets
	i2cMasterSend(0x42, 1, &sendData);
	_delay_ms(7);
	homeHeading = readCompass();
	rprintf("Home Heading: %d\n\r", homeHeading);
}
