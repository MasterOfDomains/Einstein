#include "hwutils.h"

#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <util/delay.h>

#include "../twi.h"
#include "../a2d.h"
#include "../utils.h"

//#include "rprintf.h"
//#include "uart2.h"
//#include "uart.h"

#ifdef UARTS_MULTIPLEXED
//#include "channels.h"
#endif

#define IR_LEFT_PIN 6
#define IR_FRONT_PIN 7
#define IR_RIGHT_PIN 8
#define IR_ARM_PIN 9
#define SONAR_LEFT_PIN 5
#define SONAR_RIGHT_PIN 4

#define SONAR_PORT PORTF
#define SONAR_DDR DDRF
#define SONAR_PORT_IN PINF

#define CM_PER_INCH 2.54
#define INCHES_PER_CM 0.3937

#define SONAR_CYCLES_PER_CM 13.8
#define MAX_SONAR_CYCLES 4140

BOOL ignoreWaitForButton = FALSE;

float sharp_IR_interpret_GP2D120_float(int value);
float sharp_IR_interpret_GP2Y0A21YK_float(int value);
void clearMiskey(void); // For typing errors

void printSide(side printSide, BOOL crlf)
{
	if (printSide == LEFT)
		rprintfProgStrM("LEFT");
	else if (printSide == RIGHT)
		rprintfProgStrM("RIGHT");
	if (crlf)
		rprintfCRLF();
}

BOOL menu_getConfirm(void)
{
	BOOL confirmed;
	rprintfProgStrM("Confirm Y/N\n\r");
	char c;
getYesOrNo:
	c = getKeyTap(TRUE);
	if (c == 'Y')
		confirmed = TRUE;
	else if (c == 'N')
		confirmed = FALSE;
	else
		goto getYesOrNo;
	return confirmed;
}

char *getString(u08 maxLength)
{
	char *returnStringPtr = malloc(maxLength + 1 + 4); // +1 for null terminator
	char returnString[maxLength + 1 + 4]; // +4 is necessary, don't know why
	u08 length = 0;
	rprintf("Enter up to %d characters", maxLength);
	rprintfCRLF();
	char keytap = getKeyTap(FALSE);
	while (keytap != '\r')
	{
		returnString[length++] = keytap;
		if (length == maxLength)
		{
			rprintfCRLF();
			break;
		}
		keytap = getKeyTap(FALSE);
	}
	returnString[length] = 0; // Null terminator
	returnStringPtr = returnString;
	return returnStringPtr;
}

u32 getNumber(u08 maxDigits)
{
	rprintfProgStrM("Enter Number\n\r");
	u16 returnVal = 0;
	u08 place = 0;
	u08 digits[maxDigits];
	char keytap = getKeyTap(FALSE);
	while (keytap != '\r')
	{
		u08 digit = keytap - 48;
		digits[place] = digit;
		place++;
		keytap = getKeyTap(FALSE);
	}
	u08 length = place;
	for (u08 currPlace = 0; currPlace < length; currPlace++)
	{
		returnVal += ceil(digits[currPlace] * pow(10, (place - 1)));
		place--;
	}
	return returnVal;
}

BOOL uartReceiveByte_TL(u08 uart, u08 *byte, u08 seconds)
{
	BOOL success = FALSE;
	for (u08 second = 0; second < seconds; second++)
	{
		for (u16 miliSecond = 0; miliSecond < 1000; miliSecond++)
		{
			if (uartReceiveByte(uart, byte))
			{
				success = TRUE;
				goto endReceive;
			}
			_delay_ms(1);
		}
	}
	endReceive:
	return success;
}

distSensor getSideDistSensor(side sensorSide, distSensorType sensorType)
{
	distSensor returnSensor;
	if (sensorSide == LEFT)
	{
		if (sensorType == SONAR)
			returnSensor = SONAR_LEFT;
		else // IR
			returnSensor = IR_LEFT;
	}
	else // RIGHT
	{
		if (sensorType == SONAR)
			returnSensor = SONAR_RIGHT;
		else // IR
			returnSensor = IR_RIGHT;
	}
	return returnSensor;
}

void clearMiskey(void)
{
	_delay_ms(100);
	uartFlushReceiveBuffer(getRemoteComm());
}

char getKeyTap(BOOL crlf)
{
	unsigned char keyTap;
	uartFlushReceiveBuffer(getRemoteComm());
	while (!uartReceiveByte(getRemoteComm(), &keyTap));
	rprintfChar(keyTap);
	if (keyTap == '\r' || crlf)
		rprintfCRLF();
	clearMiskey();
	return toupper(keyTap);
}

side menu_chooseSide(void)
{
	side returnSide;
	BOOL validKey = FALSE;
	rprintfProgStrM("CHOOSE SIDE: Left (L)  Right (R)");
	rprintfCRLF();
	u08 keyTap;
	while (!validKey)
	{
		keyTap = getKeyTap(TRUE);;
		if (toupper(keyTap) == 'L')
		{
			validKey = TRUE;
			returnSide = LEFT;
		}
		else if (toupper(keyTap) == 'R')
		{
			validKey = TRUE;
			returnSide = RIGHT;
		}
	}
	return returnSide;
}

distSensor menu_chooseDistSensor(void)
{
	distSensor returnSensor;
	BOOL validKey = FALSE;
	rprintfProgStrM("DISTANCE SENSORS");
	rprintfCRLF();
	rprintfProgStrM("A - Left Sonar   B - Right Sonar");
	rprintfCRLF();
	rprintfProgStrM("C - Left IR   D - Front IR   E - Right IR");
	rprintfCRLF();
	rprintfProgStrM("F - Arm IR");
	rprintfCRLF();
	char keyTap;
	while (!validKey)
	{
		keyTap = getKeyTap(TRUE);
		if (keyTap == 'A')
		{
			validKey = TRUE;
			returnSensor = SONAR_LEFT;
		}
		else if (keyTap == 'B')
		{
			validKey = TRUE;
			returnSensor = SONAR_RIGHT;
		}
		else if (keyTap == 'C')
		{
			validKey = TRUE;
			returnSensor = IR_LEFT;
		}
		else if (keyTap == 'D')
		{
			validKey = TRUE;
			returnSensor = IR_FRONT;
		}
		else if (keyTap == 'E')
		{
			validKey = TRUE;
			returnSensor = IR_RIGHT;
		}
		else if (keyTap == 'F')
		{
			validKey = TRUE;
			returnSensor = IR_ARM;
		}
	}
	return returnSensor;
}

BOOL waitForButton(void)
{
	//BOOL escape = FALSE;
	//BOOL prevLEDstate = PORT_IS_ON(DEBUG_LED_PORT, DEBUG_LED_BIT);
//
//#ifdef COMPETITION_ROUND
	//#define TIMER 250
//#else
	//#define TIMER 1000
//#endif
//
	//if (!ignoreWaitForButton)
	//{
		//rprintfProgStrM("  Press Button to Continue...\n\r");
		//BOOL wait = TRUE;
		//u08 byte;
		//uartFlushReceiveBuffer(getRemoteComm());
		//while (wait)
		//{
			//for (u16 timer = 0; timer < TIMER; timer++)
			//{
				//if (uartReceiveByte(getRemoteComm(), &byte))
				//{
					//wait = FALSE;
					//clearMiskey();
					//if (byte == ASCII_ESCAPE)
						//escape = TRUE;
					//break;
				//}
//#ifdef INPUT_BUTTON
				//if (PORT_IS_ON(INPUT_BUTTON_PORT, INPUT_BUTTON_BIT))
				//{
					//wait = FALSE;
					//break;
				//}
//#endif
				//_delay_ms(1);
			//}
			//debugLEDtoggle();
		//}
		//_delay_ms(500); // Bounce delay
	//}
	//if (prevLEDstate)
		//PORT_ON(DEBUG_LED_PORT, DEBUG_LED_BIT);
	//else
		//PORT_OFF(DEBUG_LED_PORT, DEBUG_LED_BIT);
//
//#ifdef COMPETITION_ROUND
		//ignoreWaitForButton = TRUE;
//#endif
	//return escape;
	return 0;
}

void debugLEDoff(void)
{
	PORT_ON(DEBUG_LED_PORT, DEBUG_LED_BIT);
}

void debugLEDon(void)
{
	PORT_OFF(DEBUG_LED_PORT, DEBUG_LED_BIT);
}

void debugLEDtoggle(void)
{
	FLIP_PORT(DEBUG_LED_PORT, DEBUG_LED_BIT);
}

u16 button_pressed(void)
{
	BOOL returnVal = FALSE;
#ifdef INPUT_BUTTON_NC
	returnVal = PORT_IS_ON(INPUT_BUTTON_PORT, INPUT_BUTTON_BIT);
#endif
#ifdef INPUT_BUTTON_NO
	returnVal = PORT_IS_OFF(INPUT_BUTTON_PORT, INPUT_BUTTON_BIT);
#endif
	return returnVal;
}

signed int angtable[73]={100,100,98,97,94,91,87,82,77,71,64,57,50,42,34,26,17,9,0,-9,-17,-26,-34,-42,-50,-57,-64,-71,-77,-82,-87,-91,-94,-97,-98,-100,
						 -100,-100,-98,-97,-94,-91,-87,-82,-77,-71,-64,-57,-50,-42,-34,-26,-17,-9,0,9,17,26,34,42,50,57,64,71,77,82,87,91,94,97,98,100,100};

signed int cos_SoR(long signed int degrees) //returns cos*100
{
	if (degrees >= 0) // positive angles
		return angtable[degrees/5];
	else
		return -angtable[72-(-degrees)/5];
}

signed int sin_SoR(long signed int degrees) //returns sin*100
{
	degrees=degrees - 90; // phase shift 90 degrees

	if (degrees >= 0) //positive angles
		return angtable[degrees/5];
	else
		return -angtable[72-(-degrees)/5];
}

signed int tan_SoR(long signed int degrees) // returns tan * 10
{
	//tan(x) = sin(x)/cos(x)
	if (degrees == 90 || degrees == -90 || degrees == 270 || degrees == -270)//blows up
		return 0;//what else should I return?!?!?
	return sin_SoR(degrees)/cos_SoR(degrees)*10;
}

void signalFatalError(errorCode error)
{
	while (1)
	{
		rprintf("Fatal Error: %d\n\r", error);
		debugLEDon();
		_delay_ms(100);
		debugLEDoff();
		_delay_ms(1000);
	}
}

void flipSign(s32 *number)
{
	*number = (*number ^ -1) + 1;
}

u08 inchesToCM(float inches)
{
	u08 returnVal;
	returnVal = round(inches * CM_PER_INCH);
	return returnVal;
}

float cmToInches(float cm)
{
	float returnVal;
	returnVal = cm * INCHES_PER_CM;
	return returnVal;
}

int IRCompare(const void *aa, const void *bb)
{ // Used for qsort to find Median
    const u16 *a = aa, *b = bb;
    return (*a < *b) ? -1 : (*a > *b);
}

u08 getIRprox(u08 pin)
{
	u08 medianReading;
	#define READS 20
	u16 rawReadings[READS];
	for (u08 read = 0; read < READS; read++)
	{
		_delay_us(2);
		rawReadings[read] = a2dConvert8bit(pin);
	}
	u08 arrayLength = sizeof(rawReadings)/sizeof(u16);
	qsort(rawReadings, arrayLength, sizeof(u16), IRCompare);
	//for (u08 read = 0; read < READS; read++)
	//	rprintf("%d: %d\n\r", read, rawReadings[read]);
	medianReading = rawReadings[READS/2];
	return medianReading;
}

u16 getSonarProx(u08 pin)
{
	u16 sonarCycles = 0;
	/*
	sbi(SONAR_DDR, pin); // Set to output (high)
	PORT_ON(SONAR_PORT, pin); // Begin Trigger Pulse
	_delay_us(5); // Length of Pulse
	PORT_OFF(SONAR_PORT, pin); // End Trigger Pulse
	cbi(SONAR_DDR, pin); // Set to input (low)
	while (bit_is_clear(SONAR_PORT_IN, pin)); // Wait for Input Pulse
	while (bit_is_set(SONAR_PORT_IN, pin))
	{
		sonarCycles += 1;
		if (sonarCycles == MAX_SONAR_CYCLES)
			break;
	}
	//rprintf("Raw Sonar: %d\n\r", sonarCycles);
	*/
	return sonarCycles;
}

float sharp_IR_interpret_GP2D120_float(int value)
{
	float distance;
	#define GP2D120_INCH_MULTIPLIER 200.74
	#define GP2D120_EXPONENT -0.9554
	#define GP2D120_OFFSET 0
	distance = (GP2D120_INCH_MULTIPLIER * pow(value, GP2D120_EXPONENT)) + GP2D120_OFFSET;
	return distance;
}

float sharp_IR_interpret_GP2Y0A21YK_float(int value)
{
	float distance;
	#define GP2Y0A21YK_INCH_MULTIPLIER 291.09
	#define GP2Y0A21YK_EXPONENT -0.8105
	#define GP2Y0A21YK_OFFSET -1.3
	distance = (GP2Y0A21YK_INCH_MULTIPLIER * pow(value, GP2Y0A21YK_EXPONENT)) + GP2Y0A21YK_OFFSET;
	return distance;
}

float getDistance(distSensor sensor)
{
	// Speed of sound is 13.397 in/ms
	float distance = 10000; // In inches
	#define SONAR_CYCLES_PER_INCH 41
	#define ECHO_DELAY 5 //10
	#define SONAR_OFFSET 0.25 // Amount sensor protrudes from side
	u16 rawReading;
	switch (sensor)
	{
		case IR_ARM:
			rawReading = getIRprox(IR_ARM_PIN);
			distance = sharp_IR_interpret_GP2D120_float(rawReading);
			break;
		case IR_FRONT:
			rawReading = getIRprox(IR_FRONT_PIN);
			distance = sharp_IR_interpret_GP2Y0A21YK_float(rawReading);
			break;
		case IR_LEFT:
			rawReading = getIRprox(IR_LEFT_PIN);
			distance = sharp_IR_interpret_GP2D120_float(rawReading);
			break;
		case IR_RIGHT:
			rawReading = getIRprox(IR_RIGHT_PIN);
			distance = sharp_IR_interpret_GP2D120_float(rawReading);
			break;
		case SONAR_LEFT:
			rawReading = getSonarProx(SONAR_LEFT_PIN);
			distance = (float)rawReading/SONAR_CYCLES_PER_INCH;
			distance -= SONAR_OFFSET;
			_delay_ms(ECHO_DELAY);
			break;
		case SONAR_RIGHT:
			rawReading = getSonarProx(SONAR_RIGHT_PIN);
			distance = (float)rawReading/SONAR_CYCLES_PER_INCH;
			distance -= SONAR_OFFSET;
			_delay_ms(ECHO_DELAY);
			break;
	}
	return distance;
}

void testDistSensor(distSensor sensor)
{
	while (TRUE)
	{
		float distance;
		distance = getDistance(sensor);
		rprintfProgStrM("Distance: ");
		rprintfFloat(3, distance);
		rprintfCRLF();
		waitForButton();
	}
}

u16 getAccel(orientation way)
{
	u16 accel;
	if (way == HORIZONTAL) // "X"
		accel = a2dConvert10bit(10);
	else // way == VERTICAL // "Y"
		accel = a2dConvert10bit(11);
	return accel;
}

void testAccel(void)
{
	u16 accelX, accelY;
	while (1)
	{
		accelX = a2dConvert10bit(10);
		accelY = a2dConvert10bit(11);
		rprintf("X=%d Y=%d\n\r", accelX, accelY);
		_delay_ms(1000);
	}
}

void initUtils(void)
{
#ifndef UARTS_MULTIPLEXED
	
#endif
}
