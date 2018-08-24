#include "avrcam.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "uart2.h"
#include "../twi.h"
#include "../rprintf.h"

#define CAM_ADDRESS 0x02

typedef enum {
    ENABLE_TRACKING = 'E',
    DISABLE_TRACKING = 'D',
    PING = 'P'
} commandType;

BOOL isTracking = FALSE;

BOOL pingCamera(void);
void commandCamera(commandType command);

u16 *inputBufferDataLength;

struct blobArray getBlobs(void)
{
    struct blobArray blobs;
    //blobs.length = 0;
    //#ifdef UARTS_MULTIPLEXED
    //selectUartChannel(CAMERA);
    //#endif
    //uartFlushReceiveBuffer(cameraUART);
    //BOOL wasTracking = FALSE;
    //if (isTracking) {
    //wasTracking = TRUE;
    //} else {
    //enableTracking();
    //}
    //_delay_ms(50);
    //if (!uartReceiveBufferIsEmpty(cameraUART)) {
    //u08 syncByte = 0;
    //while (syncByte != 0x0A) {
    //uartReceiveByte(cameraUART, &syncByte);
    //}
    //uartReceiveByte(cameraUART, &blobs.length);
    //for (u08 currBlob = 0; currBlob < blobs.length; currBlob++) {
    //uartReceiveByte(cameraUART, &blobs.contents[currBlob].blobColor);
    //uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.x);
    //uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerUL.y);
    //uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.x);
    //uartReceiveByte(cameraUART, &blobs.contents[currBlob].cornerBR.y);
    //}
    //} else {
    //rprintfProgStrM("--Empty--\r\n");
    //}
    //if (!wasTracking) {
    //disableTracking();
    //}
    return blobs;
}

void enableTracking(void)
{
    if (!isTracking) {
        commandCamera(ENABLE_TRACKING);
        isTracking = TRUE;
    }
}


void disableTracking(void)
{
    if (isTracking) {
        commandCamera(DISABLE_TRACKING);
        isTracking = FALSE;
    }
}

u08 getPictureWidth(void)
{
    return PICTURE_WIDTH;
}

u08 getPictureHeight(void)
{
    return BLOB_GRID_HEIGHT;
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

void commandCamera(commandType command)
{
    u08 sendDataLength = 2;
    u08 sendData[sendDataLength];
    sendData[0] = 0X41; // Command Register
    sendData[1] = command;
    i2cMasterSend(CAM_ADDRESS, sendDataLength, sendData);
    i2cWaitForComplete();
}

BOOL pingCamera(void)
{
    rprintfProgStrM("pingCamera");
    rprintfCRLF();
    commandCamera(PING);
    rprintfProgStrM("NXTCam pinged!");
    rprintfCRLF();
    return TRUE;
}

void initCamera(u08 avrUart)
{
    rprintfProgStrM("Initializing Camera");
    rprintfCRLF();

    while (!pingCamera()) {
        _delay_ms(500);
    }
    rprintfProgStrM("Camera Initialized");
    rprintfCRLF();
}
