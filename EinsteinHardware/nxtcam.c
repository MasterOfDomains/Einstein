#include "avrcam.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "../twi.h"
#include "../rprintf.h"

#define CAM_ADDRESS 0x02

const u08 objectColorReg[8] = {0x43, 0x48, 0x4D, 0x52, 0x57, 0x5C, 0x61, 0x66};
const u08 objectUL_X_Reg[8] = {0x44, 0x49, 0x4E, 0x53, 0x58, 0x5D, 0x62, 0x67};
const u08 objectUL_Y_Reg[8] = {0x45, 0x4A, 0x4F, 0x54, 0x59, 0x5E, 0x63, 0x68};
const u08 objectBR_X_Reg[8] = {0x46, 0x4B, 0x50, 0x55, 0x5A, 0x5F, 0x64, 0x69};
const u08 objectBR_Y_Reg[8] = {0x47, 0x4C, 0x51, 0x56, 0x5B, 0x60, 0x65, 0x6A};

typedef enum {
    ENABLE_TRACKING = 'E',
    DISABLE_TRACKING = 'D',
    LOCK_TRACKING = 'J',
    UNLOCK_TRACKING = 'K',
    PING = 'P',
    RESET = 'R'
} commandType;

typedef enum {
    NONE,
    MEDIUM,
    LARGE,
    EXTRA_LARGE
} delayLength;

typedef struct {
    commandType type;
    delayLength delay;
} cameraCommand;

cameraCommand enableTrackingCommand = {ENABLE_TRACKING, LARGE};
cameraCommand disableTrackingCommand = {DISABLE_TRACKING, LARGE};
cameraCommand locktrackingCommand = {LOCK_TRACKING, MEDIUM};
cameraCommand unlocktrackingCommand = {UNLOCK_TRACKING, NONE};
cameraCommand pingCommand = {PING, EXTRA_LARGE};
cameraCommand resetCommand = {RESET, LARGE};

BOOL isTracking = FALSE;
u08 commandSyncByte;
BOOL commandSynced;
#define COMMAND_COUNTER_BOTTOM 150
#define COMMAND_COUNTER_TOP 250

BOOL getAck(void);
void pingCamera(void);
void resetCamera(void);
void commandCamera(cameraCommand command);
u08 readCameraRegister(u08 register);

struct blobArray getBlobs(void)
{
    struct blobArray blobs;
    blobs.length = 0;

    BOOL wasTracking = FALSE;
    if (isTracking) {
        wasTracking = TRUE;
    } else {
        rprintf("isTracking = %d, wasTracking = %d", isTracking, wasTracking);
        rprintfCRLF();
        enableTracking();
    }

    rprintfProgStrM("Locking Tracking");
    rprintfCRLF();
    commandCamera(locktrackingCommand);
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

    rprintfProgStrM("Unlocking Tracking");
    rprintfCRLF();
    commandCamera(unlocktrackingCommand);

    if (!wasTracking) {
        rprintf("isTracking = %d, wasTracking = %d", isTracking, wasTracking);
        rprintfCRLF();
        disableTracking();
    }

    return blobs;
}

void enableTracking(void)
{
    rprintfProgStrM("Enabling Tracking");
    rprintfCRLF();
    commandCamera(enableTrackingCommand);
}

void disableTracking(void)
{
    rprintfProgStrM("Disabling Tracking");
    rprintfCRLF();
    commandCamera(disableTrackingCommand);
}

void resetCamera(void)
{
    rprintfProgStrM("Resetting NXTCam");
    rprintfCRLF();
    commandCamera(resetCommand);
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

void commandCamera(cameraCommand command)
{
    BOOL success = FALSE;
    while (!success) {
        u08 sendDataLength = 2;
        u08 sendData[sendDataLength];
        sendData[0] = 0X41; // Command Register
        sendData[1] = command.type;
        i2cMasterSend(CAM_ADDRESS, sendDataLength, sendData);
        switch (command.delay) {
        case NONE:
            break;
        case MEDIUM:
            _delay_ms(25);
            break;
        case LARGE:
            _delay_ms(100);
            break;
        case EXTRA_LARGE:
            _delay_ms(1000);
            break;
        }
        success = getAck();
        if (!success) {
            _delay_ms(500);
        }
    }
}

u08 readCameraRegister(u08 regAddress)
{
    u08 regValue;
    i2cMasterSend(CAM_ADDRESS, 1, &regAddress);
    i2cMasterReceive(CAM_ADDRESS, 1, &regValue);
    return regValue;
}

void pingCamera(void)
{
    rprintfProgStrM("Pinging NXTCam");
    rprintfCRLF();
    commandCamera(pingCommand);
}

u08 readByte()
{
    i2cReceiveByte(TRUE);
    i2cWaitForComplete();
    _delay_us(10);
    return i2cGetReceivedByte();
}

void incrementSyncByte()
{
    if (commandSyncByte == COMMAND_COUNTER_TOP) {
        commandSyncByte = COMMAND_COUNTER_BOTTOM;
    } else {
        commandSyncByte++;
    }
}

BOOL getAck()
{
    BOOL Acked = TRUE;
#define DATA_LENGTH 5
    u08 data[DATA_LENGTH];
    i2cMasterReceive(CAM_ADDRESS, DATA_LENGTH, data);
    if (!commandSynced) {
        commandSyncByte = data[0];
        commandSynced = TRUE;
    } else {
        incrementSyncByte();
        if (commandSyncByte != data[0]) {
            rprintf("ERROR - commandSync: %d, data: %d", commandSyncByte, data[0]);
            rprintfCRLF();
            //signalFatalError(NXTCAM_PROTOCOL_ERROR);
        }
    }
    if (data[1] == 'A' && data[2] == 'C' && data[3] == 'K' && data[4] == '\r') {
        rprintf("ACK!!! %d", data[0]);
        rprintfCRLF();
    } else if (data[1] == 'N' && data[2] == 'C' && data[3] == 'K' && data[4] == '\r') {
        rprintf("NCK %d", data[0]);
        rprintfCRLF();
        Acked = FALSE;
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

    commandSynced = FALSE;
    pingCamera();
    resetCamera();
    getBlobs();

    rprintfProgStrM("Camera Initialized");
    rprintfCRLF();
}
