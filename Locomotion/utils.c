#include "utils.h"

direction oppositeDir(direction dir)
{
	direction returnVal;
	if (dir == FORWARD)
	returnVal = BACKWARD;
	else
	returnVal = FORWARD;
	return returnVal;
}

side oppositeSide(side pSide)
{
	side returnVal;
	if (pSide == LEFT)
	returnVal = RIGHT;
	else
	returnVal = LEFT;
	return returnVal;
}

void LED_off(void)
{
	PORT_ON(LED_PORT, LED_BIT);
}

void LED_on(void)
{
	PORT_OFF(LED_PORT, LED_BIT);
}

void debugLEDtoggle(void)
{
	FLIP_PORT(DEBUG_LED_PORT, DEBUG_LED_BIT);
}