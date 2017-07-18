#include "servo8t.h"

#include <stdlib.h>
#include <math.h>
#include <util/delay.h>

#include "../uart2.h"
//#include "rprintf.h"

#ifdef UARTS_MULTIPLEXED
//#include "channels.h"
#endif

// For testTorque
u16 msDelay = 250;
u16 sampleSize = 4;

void getReturn(void);
void sendCommand(u08 servo, char command);

/*
void testTorque(void)
{
	while (TRUE)
	{
		rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("TEST TORQUE");							rprintfCRLF();	rprintfCRLF();
		rprintf("D - Change Delay (%d)", msDelay);				rprintfCRLF();	rprintfCRLF();
		rprintf("S - Change Sample Size (%d)", sampleSize);		rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("R - Read Torque");						rprintfCRLF();	rprintfCRLF();
		//char keyTap = getKeyTap(TRUE);
		char keyTap = 'R';
		switch (keyTap)
		{
			case 'R':
			{
				getTorque(5);
				break;
			}
			case 'D':
				rprintf("Changing Delay\n\r");
				msDelay = getNumber(5);
				break;
			case 'S':
				rprintf("Changing Sample Size\n\r");
				sampleSize = getNumber(3);
				break;
			default:
				rprintfProgStrM("Invalid Key");
		}
	}
}

u16 getTorque(u08 servo)
{
	u16 returnTorque = 0;
	u08 torqueLB, torqueHB;
	u16 torqueHB16;
	uartFlushReceiveBuffer(servo8tUART);
	//delay_ms(500);
	for (u16 sample = 0; sample < sampleSize; sample++)
	{
		sendCommand(servo, 't');
		while (!uartReceiveByte(servo8tUART, &torqueLB));
		while (!uartReceiveByte(servo8tUART, &torqueHB));
		getReturn();
		torqueHB16 = torqueHB;
		returnTorque = torqueHB16 << 8;
		returnTorque = returnTorque + torqueLB;
		//currTorque += torqueLB;
		if (returnTorque > 0)
			break;
		else
		{
			delay_ms(msDelay);
			rprintf("-");
		}
	}
	rprintf("Torque = %d\n\r", returnTorque);
	return returnTorque;
}
*/

u16 getTorque(u08 servo)
{
	u16 returnTorque = 0;
	u08 torqueLB, torqueHB;
	u16 currTorque;
	u16 torqueHB16;
	uartFlushReceiveBuffer(SERVOS_UART);
	BOOL nonZeroFlag = FALSE;
	u08 nonZeroCount = 0;
	u16 highestIndex = 0;
	for (u16 sample = 0; sample < sampleSize; sample++)
	{
		sendCommand(servo, 't');
		while (!uartReceiveByte(SERVOS_UART, &torqueLB));
		while (!uartReceiveByte(SERVOS_UART, &torqueHB));
		getReturn();
		torqueHB16 = torqueHB;
		currTorque = torqueHB16 << 8;
		currTorque = currTorque + torqueLB;
		//currTorque += torqueLB;
		if (currTorque != 0 || nonZeroFlag)
		{
			nonZeroFlag = TRUE;
			if (currTorque > returnTorque)
			{
				returnTorque = currTorque;
				highestIndex = sample;
			}
			if (nonZeroCount++ == 10)
				break;
		}
	}
	rprintf("Torque = %d, Index = %d\n\r", returnTorque, highestIndex);
	return returnTorque;
}

void moveServo(u08 servo, u08 pos)
{
	//rprintf("Moving %d to %d\n\r", servo, pos);
	uartFlushReceiveBuffer(SERVOS_UART);
	sendCommand(servo, 'a');
	uartSendByte(SERVOS_UART, pos);
	getReturn();
}

void getReturn(void)
{
	u08 cr;
	while (!uartReceiveByte(SERVOS_UART, &cr));
	if (cr != '\r')
		rprintf("No CR, received %d\n\r", cr);
}

u08 getServoPos(u08 servo)
{
	u08 pos = 0;

	if (servo < 1 || servo > 8)
		signalFatalError(INVALID_SERVO);

	u16 delayTime = 20; // Can be less?
	_delay_ms(delayTime);

	uartFlushReceiveBuffer(SERVOS_UART);
	sendCommand(servo, 'g');
	_delay_ms(delayTime);
	while (!uartReceiveByte(SERVOS_UART, &pos));
	getReturn();
	return pos;
}

void initServo8t(void)
{
	rprintf("Init servos... ");
	uartSetBaudRate(SERVOS_UART, SERVOS_UART_BAUDRATE);
	for (u08 servo = 1; servo <= 8; servo++)
	{
		sendCommand(servo, 'h');
		getReturn();
	}
	rprintf("Done");
	rprintfCRLF();
}

void sendCommand(u08 servo, char command)
{
#ifdef UARTS_MULTIPLEXED
	selectUartChannel(SERVOS);
#endif
	uartSendByte(SERVOS_UART, '>');
	uartSendByte(SERVOS_UART, '1');
	uartSendByte(SERVOS_UART, '0' + servo);
	uartSendByte(SERVOS_UART, command);
}

void setHome(u08 servo)
{
	uartFlushReceiveBuffer(SERVOS_UART);
	sendCommand(servo, 'c');
	getReturn();
}
