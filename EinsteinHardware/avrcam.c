#define AT
//#define NIGHT_250WHALOGEN
//#define DAY_CLOUDY

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "uart2.h"

#include "../rprintf.h"
#include "avrcam.h"
#include "channels.h"

u08 cameraUART;
BOOL isTracking = FALSE;

BOOL getAck(void);
BOOL pingCamera(void);
void clearColorMap(void);

u16 *inputBufferDataLength;

struct trackingColorArray
{
	struct colorStruct contents[8];
	u08 length;
} trackingColors;

#ifdef NIGHT_250WHALOGEN
//                                        R    |    G    |    B
//                                      L    U |  L    U |  L    U
struct colorStruct orange =	{ ORANGE,  160, 240,  16,  64,  16,  16 };
struct colorStruct yellow =	{ YELLOW,  208, 240, 128, 176,  16,  16 };
struct colorStruct red =    { RED,      64,  96,  16,  32,  16,  32 };
struct colorStruct blue =   { BLUE,     32,  96,  48,  96,  32, 240 };
struct colorStruct green =  { GREEN,    32,  32,  64, 240,  16,  16 };
struct colorStruct brown =  { BROWN,     0,   0,   0,   0,   0,   0 };
struct colorStruct black =  { BLACK,     0,   0,   0,   0,   0,   0 };
struct colorStruct white =  { WHITE,     0,   0,   0,   0,   0,   0 };
#endif

#ifdef DAY_CLOUDY
//                                        R    |    G    |    B
//                                      L    U |  L    U |  L    U
struct colorStruct orange =	{ ORANGE,  160, 240,  16,  64,  16,  16 };
struct colorStruct yellow =	{ YELLOW,  208, 240, 128, 176,  16,  16 };
struct colorStruct red =    { RED,      64,  96,  16,  32,  16,  32 };
struct colorStruct blue =   { BLUE,     32,  96,  48,  96,  32, 240 };
struct colorStruct green =  { GREEN,    32,  32,  32, 240,  16,  16 };
struct colorStruct brown =  { BROWN,     0,   0,   0,   0,   0,   0 };
struct colorStruct black =  { BLACK,     0,   0,   0,   0,   0,   0 };
struct colorStruct white =  { WHITE,     0,   0,   0,   0,   0,   0 };
#endif

#ifdef AT
struct colorStruct orange =	{ ORANGE,  224, 240,  48,  224,  16,  16 };
struct colorStruct yellow =	{ YELLOW,  240, 240, 240,  240,  16,  16 };
struct colorStruct red =    { RED,     144, 176,  16,   32,  16,  16 };
struct colorStruct blue =   { BLUE,     32, 128,  48,  192,  80, 240 };
struct colorStruct green =  { GREEN,    96, 128, 160,  208,  48,  64 };
struct colorStruct brown =  { BROWN,   128, 176,  96,  112,  16,  32 };
struct colorStruct black =  { BLACK,     0,   0,   0,    0,   0,   0 };
struct colorStruct white =  { WHITE,     0,   0,   0,    0,   0,   0 };
#endif

u08 validateColorLimit(u08 limit)
{
	u08 remainder = limit % 16;
	if (remainder != 0)
	{
		if (remainder > 7) // Round to next 16-divisable value
			limit = limit + (16 - remainder);
		else
			limit = limit -remainder;
	}
	return limit;
}

void createColorMap(void)
{
	clearColorMap();
	u08 colorMapIndex = 0;
	for (u08 channel = 0; channel < 3; channel++) // For R, G, and B
	{
		for (u08 boundCounter = 0; boundCounter < 16; boundCounter++)
		{ // Iterate by 16 up to 240
			u08 mapValue = 0;
			u08 channelBound = boundCounter * 16;
			u08 upperBound, lowerBound;
			for (u08 colorIndex = 0; colorIndex < trackingColors.length; colorIndex++)
			{ // For each color tracked
				struct colorStruct currentColor = trackingColors.contents[colorIndex];
				switch (channel)
				{
					case 0: // R
						lowerBound = currentColor.r_lower;
						upperBound = currentColor.r_upper;
						break;
					case 1: // G
						lowerBound = currentColor.g_lower;
						upperBound = currentColor.g_upper;
						break;
					case 2: // B
						lowerBound = currentColor.b_lower;
						upperBound = currentColor.b_upper;
						break;
				}
				if ((channelBound >= lowerBound) && (channelBound <= upperBound))
					mapValue += (0b10000000 >> currentColor.name);
			}
			colorMap[colorMapIndex++] = mapValue;
		}
	}
}

BOOL setColorMap(void)
{
	BOOL success = FALSE;
	//rprintfProgStrM("Setting Color Map\n\r");
	while (!success)
	{
		uartSendByte(cameraUART, 'S');
		uartSendByte(cameraUART, 'M');
		for (u08 index = 0; index < COLOR_MAP_LENGTH; index++)
		{
			char str[20];
			char* strMapValue = utoa(colorMap[index], str, 10);
			uartSendByte(cameraUART, ' ');
			while (*strMapValue)
				uartSendByte(cameraUART, *strMapValue++);
		}
		uartSendByte(cameraUART, '\r');
		success = getAck();
		if (!success)
			rprintfProgStrM("Error Setting Color Map\n\r");
	}
	return success;
}

void clearColorMap(void)
{
	for (u08 index = 0; index < COLOR_MAP_LENGTH; index++)
		colorMap[index] = 0;
}

void clearTrackingColors(void)
{
	trackingColors.length = 0;
}

void addTrackingColor(color colorName)
{
	struct colorStruct trackingColor = getColor(colorName);
	trackingColors.contents[trackingColors.length++] = trackingColor;
}

void emptyBufferToScreen(void)
{
	if (*inputBufferDataLength > 0)
		rprintf("In Buffer: ");
	while (*inputBufferDataLength > 0)
	{
		u08 byte;
		uartReceiveByte(cameraUART, &byte);
		rprintf("%d, ", byte);
	}
	rprintfCRLF();
}

struct blobArray getBlobs(void)
{
	struct blobArray blobs;
	blobs.length = 0;
	uartFlushReceiveBuffer(cameraUART);
	BOOL wasTracking = FALSE;
	if (isTracking)
		wasTracking = TRUE;
	else
		enableTracking();
	_delay_ms(50);
	if (!uartReceiveBufferIsEmpty(cameraUART))
	{
		u08 syncByte = 0;
		while (syncByte != 0x0A)
			uartReceiveByte(cameraUART, &syncByte);
		uartReceiveByte(cameraUART, &blobs.length);
		for (u08 currBlob = 0; currBlob < blobs.length; currBlob++)
		{
			uartReceiveByte(cameraUART, &blobs.contents[currBlob].blobColor);
			uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.x);
			uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.y);
			uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.x);
			uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.y);
		}
	}
	if (!wasTracking)
		disableTracking();
	return blobs;
}

void enableTracking(void)
{
	BOOL trackingEnabled = FALSE;
	while (!trackingEnabled)
	{
		uartSendByte(cameraUART, 'E');
		uartSendByte(cameraUART, 'T');
		uartSendByte(cameraUART, '\r');
		trackingEnabled = getAck();
		if (!trackingEnabled)
			_delay_us(12345);
	}
	isTracking = TRUE;
	//rprintfProgStrM("Tracking\n\r");
}

void disableTracking(void)
{
	if (isTracking)
	{
		BOOL trackingDisabled = FALSE;
		while (!trackingDisabled)
		{
			uartSendByte(cameraUART, 'D');
			uartSendByte(cameraUART, 'T');
			uartSendByte(cameraUART, '\r');
			_delay_ms(250);
			uartFlushReceiveBuffer(cameraUART);
			_delay_ms(50);
			if (*inputBufferDataLength == 0)
				trackingDisabled = TRUE;
			else
			{
				rprintf("Not Empty: %d\n\r", *inputBufferDataLength);
				waitForButton();
			}
		}
		isTracking = FALSE;
	}
}

u08 getPictureWidth(void)
{
	return PICTURE_WIDTH;
}

u08 getPictureHeight(void)
{
	return PICTURE_HEIGHT;
}

u08 getPictureCenter(orientation way)
{
	u08 pictureCenter;
	if (way == HORIZONTAL)
		pictureCenter = getPictureWidth()/2 + (PICTURE_WIDTH/16);
	else // VERTICAL
		pictureCenter = getPictureHeight()/2;
	return pictureCenter;
}

char getCK(void)
{ // Returns '/r' if successful
	unsigned char lastChar = 0;
	BOOL success = FALSE;
	unsigned char ackC, ackK, ackRet;
	u08 charsReceived = 0;
	if (uartReceiveByte_TL(cameraUART, &ackC, 1))
	{
		charsReceived++;
		lastChar = ackC;
		if (ackC == 'C')
		{
			if (uartReceiveByte_TL(cameraUART, &ackK, 1))
			{
				charsReceived++;
				if (ackK == 'K')
				{
					lastChar = ackK;
					if (uartReceiveByte_TL(cameraUART, &ackRet, 1))
					{
						charsReceived++;
						lastChar = ackRet;
						if (ackRet == '\r')
							success = TRUE;
					}
				}
			}
		}
	}
		
	if (!success)
	{
		#define BAD_ACK_PAUSE 5000
		rprintf("Received %d, lastChar = %d\n\r", charsReceived, lastChar);
		_delay_ms(BAD_ACK_PAUSE);
	}
	return lastChar;
}

BOOL getAck(void)
{
	BOOL acked = FALSE, ncked = FALSE;
	unsigned char byte;
	if (uartReceiveByte_TL(cameraUART, &byte, 3))
	{
		char lastChar = byte;
		if (lastChar == 'A') // ACK?
		{
			lastChar = getCK();
			if (lastChar == '\r')
				acked = TRUE;
		}
		else if (lastChar == 'N') // NCK?
		{
			rprintf("N\n\r");
			lastChar = getCK();
			if (lastChar == '\r')
				ncked = TRUE;
		}
		if (!acked && !ncked)
		{
			rprintf("No Ack/Nck\n\r");
			waitForButton();
		}
	}
	else // No byte received
		rprintf("getAck: No Byte\n\r");
	return acked;
}

void printColorName(color cName, BOOL crlf)
{
	switch (cName)
	{
		case RED:
			rprintfProgStrM("Red");
			break;
		case ORANGE:
			rprintfProgStrM("Orange");
			break;
		case YELLOW:
			rprintfProgStrM("Yellow");
			break;
		case GREEN:
			rprintfProgStrM("Green");
			break;
		case BLUE:
			rprintfProgStrM("Blue");
			break;
		case BROWN:
			rprintfProgStrM("Brown");
			break;
		case WHITE:
			rprintfProgStrM("White");
			break;
		case BLACK:
			rprintfProgStrM("Black");
			break;
	}
	//rprintf(" (%d)", cName);
	if (crlf)
		rprintfCRLF();
}

BOOL pingCamera(void)
{
	BOOL ack = FALSE;
	uartSendByte(cameraUART, 'P');
	uartSendByte(cameraUART, 'G');
	//uartSendByte(cameraUART, 'A');
	//uartSendByte(cameraUART, 'C');
	//uartSendByte(cameraUART, 'K');
	uartSendByte(cameraUART, '\r');

	ack = getAck();
	if (!ack)
		emptyBufferToScreen();
	return ack;
}

void initCamera(u08 avrUart)
{	
	rprintfProgStrM("Initializing Camera");
	rprintfCRLF();
	cameraUART = avrUart;
	selectUartChannel(CAMERA);
	cBuffer *camBuffer;
	camBuffer = uartGetRxBuffer(cameraUART);
	inputBufferDataLength = &camBuffer->datalength;
	
	while (!pingCamera())
		_delay_ms(10);
		
	rprintfProgStrM("Comm Successful");
	rprintfCRLF();

	clearTrackingColors();
	addTrackingColor(YELLOW);
	addTrackingColor(ORANGE);
	addTrackingColor(RED);
	addTrackingColor(BLUE);
	addTrackingColor(GREEN);
	createColorMap();
	setColorMap();

	rprintfProgStrM("Camera Initialized");
	rprintfCRLF();
}

struct colorStruct getColor(color colorName)
{
	struct colorStruct returnStruct;
	switch (colorName)
	{
		case RED:
			returnStruct = red;
			break;
		case ORANGE:
			returnStruct = orange;			
			break;
		case YELLOW:
			returnStruct = yellow;
			break;
		case GREEN:
			returnStruct = green;
			break;
		case BLUE:
			returnStruct = blue;
			break;
		case BROWN:
			returnStruct = brown;
			break;
		case WHITE:
			returnStruct = white;
			break;
		case BLACK:
			returnStruct = black;
			break;
	}
	return returnStruct;
}

struct colorStruct * getColorPointer(color structColor)
{
	struct colorStruct *returnStruct;
	switch (structColor)
	{
		case RED:
			returnStruct = &red;
			break;
		case ORANGE:
			returnStruct = &orange;			
			break;
		case YELLOW:
			returnStruct = &yellow;
			break;
		case GREEN:
			returnStruct = &green;
			break;
		case BLUE:
			returnStruct = &blue;
			break;
		case BROWN:
			returnStruct = &brown;
			break;
		case WHITE:
			returnStruct = &white;
			break;
		case BLACK:
			returnStruct = &black;
			break;
	}
	return returnStruct;
}

void displayColor(struct colorStruct displayColor)
{
	printColorName(displayColor.name, TRUE);
	rprintf("R  L: %d  H: %d", displayColor.r_lower, displayColor.r_upper);
	rprintfCRLF();
	rprintf("G  L: %d  H: %d", displayColor.g_lower, displayColor.g_upper);
	rprintfCRLF();
	rprintf("B  L: %d  H: %d", displayColor.b_lower, displayColor.b_upper);
	rprintfCRLF();
}

void displayColors(void)
{
	displayColor(yellow);
	displayColor(orange);
	displayColor(red);
	displayColor(blue);
	displayColor(green);
	displayColor(brown);
	displayColor(black);
	displayColor(white);
}

