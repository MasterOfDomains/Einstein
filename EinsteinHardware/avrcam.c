#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "uart2.h"

#include "../rprintf.h"
#include "avrcam.h"
#ifdef UARTS_MULTIPLEXED
#include "channels.h"
#endif

u08 cameraUART;
BOOL isTracking = FALSE;

BOOL getAck(void);
BOOL pingCamera(void);

u16 *inputBufferDataLength;

void emptyBufferToScreen(void)
{
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    if (*inputBufferDataLength > 0) {
        rprintf("In Buffer: ");
    }
    while (*inputBufferDataLength > 0) {
        u08 byte;
        uartReceiveByte(cameraUART, &byte);
        rprintf("%d, ", byte);
    }
    rprintfCRLF();
}

struct blobArray getTestBlobs(void)
{
    struct blobArray blobs;
#define MIN_BLOB_SIZE 30
    struct point blob0Offset = {20, 50};
    u08 blob0Size = MIN_BLOB_SIZE + 25;
    struct blob blob0;
    blob0.blobColor = RED;
    blob0.cornerBR = (struct point) {
        blob0Offset.x + blob0Size, blob0Offset.y + blob0Size
    };
    blob0.cornerUL = (struct point) {
        blob0Offset.x, blob0Offset.y
    };

    struct point blob1Offset = {52, 82};
    u08 blob1Size = MIN_BLOB_SIZE + 40;
    struct blob blob1;
    blob1.blobColor = GREEN;
    blob1.cornerBR = (struct point) {
        blob1Offset.x + blob1Size, blob1Offset.y + blob1Size
    };
    blob1.cornerUL = (struct point) {
        blob1Offset.x, blob1Offset.y
    };

    struct point blob2Offset = {55, 85};
    u08 blob2Size = MIN_BLOB_SIZE + 20;
    struct blob blob2;
    blob2.blobColor = RED;
    blob2.cornerBR = (struct point) {
        blob2Offset.x + blob2Size, blob2Offset.y + blob2Size
    };
    blob2.cornerUL = (struct point) {
        blob2Offset.x, blob2Offset.y
    };

    struct point blob3Offset = {20, 160};
    u08 blob3Size = MIN_BLOB_SIZE - 5;
    struct blob blob3;
    blob3.blobColor = ORANGE;
    blob3.cornerBR = (struct point) {
        blob3Offset.x + blob3Size, blob3Offset.y + blob3Size
    };
    blob3.cornerUL = (struct point) {
        blob3Offset.x, blob3Offset.y
    };

    struct point blob4Offset = {80, 80};
    u08 blob4Size = MIN_BLOB_SIZE + 5;
    struct blob blob4;
    blob4.blobColor = BLUE;
    blob4.cornerBR = (struct point) {
        blob4Offset.x + blob4Size, blob4Offset.y + blob4Size
    };
    blob4.cornerUL = (struct point) {
        blob4Offset.x, blob4Offset.y
    };

    struct point blob5Offset = {100, 100};
    u08 blob5Size = MIN_BLOB_SIZE + 5;
    struct blob blob5;
    blob5.blobColor = BLUE;
    blob5.cornerBR = (struct point) {
        blob5Offset.x + blob5Size, blob5Offset.y + blob5Size
    };
    blob5.cornerUL = (struct point) {
        blob5Offset.x, blob5Offset.y
    };

    blobs.contents[0] = blob0;
    blobs.contents[1] = blob1;
    blobs.contents[2] = blob2;
    blobs.contents[3] = blob3;
    blobs.contents[4] = blob4;
    blobs.contents[5] = blob5;
    blobs.length = 6;
    return blobs;
}

struct blobArray getBlobs(void)
{
#ifndef SIMULATOR
    struct blobArray blobs;
    blobs.length = 0;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    uartFlushReceiveBuffer(cameraUART);
    BOOL wasTracking = FALSE;
    if (isTracking) {
        wasTracking = TRUE;
    } else {
        enableTracking();
    }
    _delay_ms(50);
    if (!uartReceiveBufferIsEmpty(cameraUART)) {
        u08 syncByte = 0;
        while (syncByte != 0x0A) {
            uartReceiveByte(cameraUART, &syncByte);
        }
        uartReceiveByte(cameraUART, &blobs.length);
        for (u08 currBlob = 0; currBlob < blobs.length; currBlob++) {
            uartReceiveByte(cameraUART, &blobs.contents[currBlob].blobColor);
            uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.x);
            uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.y);
            uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.x);
            uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.y);
        }
    }
    if (!wasTracking) {
        disableTracking();
    }
    return blobs;
#else
    struct blobArray blobs = getTestBlobs();
    return blobs;
#endif
}

void enableTracking(void)
{
    BOOL trackingEnabled = FALSE;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    while (!trackingEnabled) {
        uartSendByte(cameraUART, 'E');
        uartSendByte(cameraUART, 'T');
        uartSendByte(cameraUART, '\r');
        trackingEnabled = getAck();
        if (!trackingEnabled) {
            _delay_us(12345);
        }
    }
    isTracking = TRUE;
    //rprintfProgStrM("Tracking\n\r");
}

void dumpFrame(void)
{
    rprintfProgStrM("Dumping Frame");
    rprintfCRLF();
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif

#define ROW_START_CHAR 0x0B
#define ROW_END_CHAR 0x0F

    u08 data;
    uartSendByte(cameraUART, 'D');
    uartSendByte(cameraUART, 'F');
    uartSendByte(cameraUART, '\r');
    if (getAck()) {
        for (int row = 0; row < PICTURE_HEIGHT_DUMP; row++) {
            while (!uartReceiveByte(cameraUART, &data));
            if (data != ROW_START_CHAR) {
                goto ARVcam_error;
            }
            while (!uartReceiveByte(cameraUART, &data));
            for (int pixel = 0; pixel < PICTURE_WIDTH; pixel++) {
                while (!uartReceiveByte(cameraUART, &data));
            }
            while (!uartReceiveByte(cameraUART, &data));
            if (data != ROW_END_CHAR) {
                goto ARVcam_error;
            }
        }
        rprintfProgStrM("Dumped");
        rprintfCRLF();
    } else {
ARVcam_error:
        signalFatalError(AVRCAM_COMM_ERROR);
    }
}

void disableTracking(void)
{
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    if (isTracking) {
        BOOL trackingDisabled = FALSE;
        while (!trackingDisabled) {
            uartSendByte(cameraUART, 'D');
            uartSendByte(cameraUART, 'T');
            uartSendByte(cameraUART, '\r');
            _delay_ms(250);
            uartFlushReceiveBuffer(cameraUART);
            _delay_ms(50);
            if (*inputBufferDataLength == 0) {
                trackingDisabled = TRUE;
            } else {
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
    return PICTURE_HEIGHT_BLOBS;
}

u08 getPictureCenter(orientation way)
{
    u08 pictureCenter;
    if (way == HORIZONTAL) {
        pictureCenter = getPictureWidth()/2 + (PICTURE_WIDTH/16);
    } else { // VERTICAL
        pictureCenter = getPictureHeight()/2;
    }
    return pictureCenter;
}

char getCK(void)
{
    // Returns '/r' if successful
    unsigned char lastChar = 0;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    BOOL success = FALSE;
    unsigned char ackC, ackK, ackRet;
    u08 charsReceived = 0;
    if (uartReceiveByte_TL(cameraUART, &ackC, 1)) {
        charsReceived++;
        lastChar = ackC;
        if (ackC == 'C') {
            if (uartReceiveByte_TL(cameraUART, &ackK, 1)) {
                charsReceived++;
                if (ackK == 'K') {
                    lastChar = ackK;
                    if (uartReceiveByte_TL(cameraUART, &ackRet, 1)) {
                        charsReceived++;
                        lastChar = ackRet;
                        if (ackRet == '\r') {
                            success = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (!success) {
#define BAD_ACK_PAUSE 5000
        rprintf("Received %d, lastChar = %d\n\r", charsReceived, lastChar);
        _delay_ms(BAD_ACK_PAUSE);
    }
    return lastChar;
}

BOOL getAck(void)
{
    BOOL acked = FALSE, ncked = FALSE;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    unsigned char byte;
    if (uartReceiveByte_TL(cameraUART, &byte, 3)) {
        char currentChar = byte;
        if (currentChar == 'A') { // ACK?
            currentChar = getCK();
            if (currentChar == '\r') {
                acked = TRUE;
            }
        } else if (currentChar == 'N') { // NCK?
            rprintf("N\n\r");
            currentChar = getCK();
            if (currentChar == '\r') {
                ncked = TRUE;
            }
        }
        if (!acked && !ncked) {
            rprintf("No Ack/Nck - currentChar: %d\n\r", currentChar);
            emptyBufferToScreen();
            waitForButton(); // Currently does nothing
        }
    } else { // No byte received
        rprintf("getAck: No Byte\n\r");
    }
    return acked;
}

void printColorName(color cName, BOOL crlf)
{
    switch (cName) {
    case RED:
        rprintfProgStrM("Red");
        break;
    case ORANGE:
        rprintfProgStrM("Ong");
        break;
    case GREEN:
        rprintfProgStrM("Grn");
        break;
    case BLUE:
        rprintfProgStrM("Blu");
        break;
    case YELLOW:
        rprintfProgStrM("Yel");
        break;
    case WHITE:
        rprintfProgStrM("Wht");
        break;
    }
    rprintf(" (%d)", cName);
    if (crlf) {
        rprintfCRLF();
    }
}

BOOL pingCamera(void)
{
    rprintfProgStrM("pingCamera\n\r");
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    BOOL ack = FALSE;
    uartSendByte(cameraUART, 'P');
    uartSendByte(cameraUART, 'G');
    uartSendByte(cameraUART, '\r');

    ack = getAck();
    if (!ack) {
        emptyBufferToScreen();
    }
    return ack;
}

void initCamera(u08 avrUart)
{
    rprintfProgStrM("Initializing Camera");
    rprintfCRLF();
    cameraUART = avrUart;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(CAMERA);
#endif
    cBuffer *camBuffer;
    camBuffer = uartGetRxBuffer(cameraUART);
    inputBufferDataLength = &camBuffer->datalength;

    while (!pingCamera()) {
        _delay_ms(500);
    }
    dumpFrame();
    uartFlushReceiveBuffer(cameraUART);
    rprintfProgStrM("Camera Initialized");
    rprintfCRLF();
}
