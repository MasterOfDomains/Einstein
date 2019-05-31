//#include "global.h"

#include <util/delay.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#include "../rprintf.h"
#include "uart.h"

#include "lutils.h"

#define LED_PORT PORTB
#define LED_BIT PB1

BOOL ignoreWaitForButton = FALSE;

void clearMiskey(void); // For typing errors

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
    FLIP_PORT(LED_PORT, LED_BIT);
}

void signalFatalError(errorCode error)
{
    while (1) {
        rprintf("Fatal Error: %d", error); rprintfCRLF();
        LED_on();
        _delay_ms(200);
        LED_off();
        _delay_ms(800);
    }
}

/*
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

void clearMiskey(void)
{
	_delay_ms(100);
	uartFlushReceiveBuffer();
}

char getKeyTap(BOOL crlf)
{
	unsigned char keyTap;
	uartFlushReceiveBuffer();
	while (!uartReceiveByte(&keyTap));
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

BOOL waitForButton(void)
{
	BOOL escape = FALSE;
	BOOL prevLEDstate = PORT_IS_ON(PORTB, LED_BIT);

	if (!ignoreWaitForButton)
	{
		rprintfProgStrM("Waiting...\n\r");
		u08 byte;
		uartFlushReceiveBuffer();
		while (1)
		{
			if (uartReceiveByte(&byte))
			{
				clearMiskey();
				if (byte == ASCII_ESCAPE)
					escape = TRUE;
				break;
			}
			_delay_ms(1);
			FLIP_PORT(LED_PORT, PB6);
		}
		_delay_ms(500); // Bounce delay
	}

	if (prevLEDstate)
		PORT_ON(LED_PORT, LED_BIT);
	else
		PORT_OFF(LED_PORT, LED_BIT);

#ifdef COMPETITION_ROUND
		ignoreWaitForButton = TRUE;
#endif
	return escape;
}

void LED_off(void)
{
	PORT_ON(LED_PORT, LED_BIT);
}

void LED_on(void)
{
	PORT_OFF(LED_PORT, LED_BIT);
}

void signalFatalError(errorCode error)
{
	while (1)
	{
		rprintf("Fatal Error: %d\n\r", error);
		LED_on();
		_delay_ms(100);
		LED_off();
		_delay_ms(1000);
	}
}
*/
