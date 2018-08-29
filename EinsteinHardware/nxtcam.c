#include "avrcam.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "../twi.h"
#include "../rprintf.h"

#define CAM_ADDRESS 0x02

u08 objectColorReg[8] = {0x43, 0x48, 0x4D, 0x52, 0x57, 0x5C, 0x61, 0x66};
u08 objectUL_X_Reg[8] = {0x44, 0x49, 0x4E, 0x53, 0x58, 0x5D, 0x62, 0x67};
u08 objectUL_Y_Reg[8] = {0x45, 0x4A, 0x4F, 0x54, 0x59, 0x5E, 0x63, 0x68};
u08 objectBR_X_Reg[8] = {0x46, 0x4B, 0x50, 0x55, 0x5A, 0x5F, 0x64, 0x69};
u08 objectBR_Y_Reg[8] = {0x47, 0x4C, 0x51, 0x56, 0x5B, 0x60, 0x65, 0x6A};

typedef enum {
    ENABLE_TRACKING = 'E',
    DISABLE_TRACKING = 'D',
    PING = 'P',
    OBJECT_TRACKING_MODE = 'B',
    RESET = 'R'
} commandType;

BOOL isTracking = FALSE;

BOOL getAck();
BOOL pingCamera(void);
BOOL commandCamera(commandType command);
u08 readCameraRegister(u08 register);

u16 *inputBufferDataLength;

struct blobArray getBlobs(void)
{
    struct blobArray blobs;
    blobs.length = 0;

    u08 numberOfObjectsReg = 0x42;
    u08 regValue = readCameraRegister(numberOfObjectsReg);
    rprintf("Number: %d", regValue);
    rprintfCRLF();

    if (regValue != 0x41) {
        blobs.length = regValue;

        for (u08 currBlob = 0; currBlob < blobs.length; currBlob++) {
            blobs.contents[currBlob].blobColor = readCameraRegister(objectColorReg[currBlob]);
            blobs.contents[currBlob].cornerUL.x = readCameraRegister(objectUL_X_Reg[currBlob]);
            blobs.contents[currBlob].cornerUL.y = readCameraRegister(objectUL_Y_Reg[currBlob]);
            blobs.contents[currBlob].cornerBR.x = readCameraRegister(objectBR_X_Reg[currBlob]);
            blobs.contents[currBlob].cornerBR.y = readCameraRegister(objectBR_Y_Reg[currBlob]);
        }
    }

    /*
        rprintf("Number of Blobs: %d", blobs.length);
        rprintfCRLF();
        BOOL wasTracking = FALSE;
        if (isTracking) {
            //wasTracking = TRUE;
        } else {
            //enableTracking();
        }

        for (u08 currBlob = 0; currBlob < blobs.length; currBlob++) {
            blobs.contents[currBlob].blobColor = readCameraRegister(objectColorReg[currBlob]);
            blobs.contents[currBlob].cornerUL.x = readCameraRegister(objectUL_X_Reg[currBlob]);
            blobs.contents[currBlob].cornerUL.y = readCameraRegister(objectUL_Y_Reg[currBlob]);
            blobs.contents[currBlob].cornerUL.x = readCameraRegister(objectLR_X_Reg[currBlob]);
            blobs.contents[currBlob].cornerUL.y = readCameraRegister(objectLR_Y_Reg[currBlob]);
        }

        if (!wasTracking) {
            //disableTracking();
        }
    	*/
    return blobs;
}

void enableTracking(void)
{
    rprintfProgStrM("Enabling");
    rprintfCRLF();
    if (!isTracking) {
        isTracking = commandCamera(ENABLE_TRACKING);
    }
}

void resetCamera(void)
{
    rprintfProgStrM("Resetting");
    rprintfCRLF();
    commandCamera(RESET);
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

BOOL commandCamera(commandType command)
{
    u08 sendDataLength = 2;
    u08 sendData[sendDataLength];
    sendData[0] = 0X41; // Command Register
    sendData[1] = command;
    i2cMasterSend(CAM_ADDRESS, sendDataLength, sendData);
    _delay_ms(100);
    return getAck();
}

u08 readCameraRegister(u08 regAddress)
{
    u08 regValue;
    i2cMasterSend(CAM_ADDRESS, 1, &regAddress);
    i2cMasterReceive(CAM_ADDRESS, 1, &regValue);
    return regValue;
}

BOOL pingCamera(void)
{
    rprintfProgStrM("pingCamera");
    rprintfCRLF();
    return commandCamera(PING);
}

BOOL getAck()
{
    BOOL Acked = FALSE;
#define DATA_LENGTH 5
    u08 data[DATA_LENGTH];
    data[0] = 4;
    data[1] = 4;
    data[2] = 4;
    data[3] = 4;
    data[4] = 4;
    i2cMasterReceive(CAM_ADDRESS, DATA_LENGTH, data);
    if (data[1] == 'A' && data[2] == 'C' && data[3] == 'K' && data[4] == '\r') {
        rprintfProgStrM("Ack");
        rprintfCRLF();
        Acked = TRUE;
    } else if (data[1] == 'N' && data[2] == 'C' && data[3] == 'K' && data[4] == '\r') {
        rprintfProgStrM("Neg Ack");
        rprintfCRLF();
    } else {
        rprintf("No ack or nck: %d-%d-%d-%d-%d", data[0], data[1], data[2], data[3], data[4]);
        rprintfCRLF();
    }
    return Acked;
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
