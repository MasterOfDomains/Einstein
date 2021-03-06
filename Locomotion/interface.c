#include "interface.h"

#include "../twi.h"
#include "../rprintf.h"
#include "lutils.h"
#include "encoder.h"

#define LOC_ADDRESS 0xA0

#define LB_ARRAY_SIZE 10
unsigned char localBuffer[LB_ARRAY_SIZE];

volatile BOOL newReceiveFlag = FALSE;

union {
    float fltAmount;
    u08 bytes[4];
} amountConverter;

void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData);
u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData);

locomotionCommandName getName(void);

struct commandStruct waitForCommand(void)
{
    struct commandStruct returnVal;
    rprintf("waiting..."); rprintfCRLF();
    while (!newReceiveFlag);
    newReceiveFlag = FALSE;
    locomotionCommandName name = getName();
    returnVal.name = name;
    switch (name) {
    case GO:
        returnVal.commandSide = CENTER;
        returnVal.commandDir = localBuffer[2];
        returnVal.distance = 0;
        returnVal.speed = localBuffer[3];
        break;
    case MOVE:
        returnVal.commandSide = localBuffer[2];
        returnVal.commandDir = localBuffer[3];
        amountConverter.bytes[0] = localBuffer[4];
        amountConverter.bytes[1] = localBuffer[5];
        amountConverter.bytes[2] = localBuffer[6];
        amountConverter.bytes[3] = localBuffer[7];
        returnVal.distance = amountConverter.fltAmount;
        returnVal.speed = localBuffer[8];
        break;
    case SPIN:
        returnVal.commandSide = localBuffer[2];
        returnVal.commandDir = MIDDLE;
        returnVal.distance = 0;
        returnVal.speed = localBuffer[3];
        break;
    case TWIST:
        returnVal.commandSide = localBuffer[2];
        returnVal.commandDir = MIDDLE;
        amountConverter.bytes[0] = localBuffer[3];
        amountConverter.bytes[1] = localBuffer[4];
        amountConverter.bytes[2] = localBuffer[5];
        amountConverter.bytes[3] = localBuffer[6];
        returnVal.distance = amountConverter.fltAmount;
        returnVal.speed = localBuffer[7];
        break;
    case HARD_STOP:
        returnVal.commandSide = 0;
        returnVal.commandDir = 0;
        returnVal.distance = 0;
        returnVal.speed = 0;
        break;
    case SOFT_STOP:
        returnVal.commandSide = 0;
        returnVal.commandDir = 0;
        returnVal.distance = 0;
        returnVal.speed = 0;
        break;
    }
    return returnVal;
}

locomotionCommandName getName(void)
{
    locomotionCommandName returnVal;
    u08 char1, char2;
    char1 = localBuffer[0];
    char2 = localBuffer[1];

    rprintfChar(char1); rprintfChar(char2); rprintfCRLF();

    if (char1 == 'G' && char2 == 'O') {
        returnVal = GO;
    } else if (char1 == 'M' && char2 == 'V') {
        returnVal = MOVE;
    } else if (char1 == 'S' && char2 == 'P') {
        returnVal = SPIN;
    } else if (char1 == 'T' && char2 == 'W') {
        returnVal = TWIST;
    } else if (char1 == 'S' && char2 == 'S') {
        returnVal = SOFT_STOP;
    } else if (char1 == 'H' && char2 == 'S') {
        returnVal = HARD_STOP;
    } else {
        returnVal = HARD_STOP;
        signalFatalError(I2C_COMM_ERROR);
    }
    return returnVal;
}

void initInterface(void)
{
    i2cInit();
    i2cSetLocalDeviceAddr(LOC_ADDRESS, TRUE);
    i2cSetSlaveReceiveHandler(i2cSlaveReceiveService);
    i2cSetSlaveTransmitHandler(i2cSlaveTransmitService);
}

void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData)
{
    u08 i;
    for (i = 0; i < receiveDataLength; i++) {
        localBuffer[i] = *receiveData++;
    }
    newReceiveFlag = TRUE;
}

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
    u08 i;
    for (i = 0; i < LB_ARRAY_SIZE; i++) {
        localBuffer[0] = amountConverter.bytes[0];
    }
    amountConverter.fltAmount = getDistanceTraveled();
    localBuffer[0] = amountConverter.bytes[0];
    localBuffer[1] = amountConverter.bytes[1];
    localBuffer[2] = amountConverter.bytes[2];
    localBuffer[3] = amountConverter.bytes[3];
    for (i = 0; i < LB_ARRAY_SIZE; i++) {
        *transmitData++ = localBuffer[i];
    }
    localBuffer[0]++;
    return LB_ARRAY_SIZE;
}