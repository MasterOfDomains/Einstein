
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "dynamixel.h"

#define TXRX_DIRECTION_PORT PORTB
#define	TXRX_DIRECTION_BIT PB7

volatile u08 dynamixelTxPacket[DYNAMIXEL_PACKET_SIZE];
volatile u08 dynamixelRxPacket[DYNAMIXEL_PACKET_SIZE];
volatile u08 dynamixelRxIndex = 0;

u08 servoUART;

void initDynamixel(u08 avrUart)
{
    rprintfProgStrM("Initializing Dynamixel"); rprintfCRLF();

    servoUART = avrUart;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(SERVOS);
#endif

    dynamixelPing(1);

    rprintfProgStrM("Camera Initialized"); rprintfCRLF();
}

void dynamixelSetTx(void)
{
    PORT_ON(TXRX_DIRECTION_PORT, TXRX_DIRECTION_BIT);
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(SERVOS);
#endif
}

void dynamixelSetRx(void)
{
    // Wait for TX complete flag before turning the bus around
    //while (PORT_IS_OFF(USART_CONTROL_AND_STATUS_REGISTER, TRANSMIT_COMPLETE));
    while (!uartTxReady(servoUART));

    _delay_us(1);

    // Set UART direction pins
    //PORTD &= ~(1 << PD2);
    //PORTD |= (1 << PD3);
    PORT_OFF(TXRX_DIRECTION_PORT, TXRX_DIRECTION_BIT);

    // Reset rx index
    //dynamixel_rxindex = 0;
#ifdef UARTS_MULTIPLEXED
    selectUartChannel(SERVOS);
#endif
}

void dynamixelWrite(u08 c)
{
    //while(bit_is_clear(USART_CONTROL_AND_STATUS_REGISTER, UDRE0));
    //UDR0 = c;

    uartSendByte(servoUART, c);
}

u08 dynamixelCalculateChecksum(volatile u08* packet)
{
    u16 checksum = 0;

    for(u08 i = DYNAMIXEL_ID; i <= (packet[DYNAMIXEL_LENGTH] + 2); i++) {
        checksum += packet[i];
    }

    return ~(checksum % 256);
}

u08 dynamixelWritePacket(volatile u08* txPacket, u08 packetLength)
{
    for (u08 i = 0; i < packetLength; i++) {
        dynamixelWrite(txPacket[i]);
    }

    return DYNAMIXEL_SUCCESS;
}

u08 dynamixelReadPacket(volatile u08* rxPacket, u08 packetLength)
{
    u16 timeoutCounter = 0;
    while (dynamixelRxIndex < packetLength) {
        if (timeoutCounter++ > 10000) {
            return DYNAMIXEL_RX_TIMEOUT;
        }
    }

    if ((rxPacket[0] != 255) || (rxPacket[1] != 255)) {
        return DYNAMIXEL_RX_CORRUPT;
    }

    if (rxPacket[packetLength - 1] != dynamixelCalculateChecksum(rxPacket)) {
        return DYNAMIXEL_RX_CORRUPT;
    }

    return DYNAMIXEL_SUCCESS;
}

u08 dynamixelTxRx(volatile u08* txPacket, volatile u08* rxPacket)
{
    u08 rxLength = 0;
    u08 txLength = dynamixelTxPacket[DYNAMIXEL_LENGTH] + 4;

    txPacket[0] = (u08) 0xff;
    txPacket[1] = (u08) 0xff;
    txPacket[txLength - 1] = (u08) dynamixelCalculateChecksum(txPacket);

    dynamixelSetTx();
    dynamixelWritePacket(txPacket, txLength);
    dynamixelSetRx();

    if (txPacket[DYNAMIXEL_ID] != DYNAMIXEL_BROADCAST_ID) {
        if (txPacket[DYNAMIXEL_INSTRUCTION] == DYNAMIXEL_READ) {
            rxLength = txPacket[DYNAMIXEL_PARAMETER + 1] + 6;
        } else {
            rxLength = 6;
        }

        return dynamixelReadPacket(rxPacket, rxLength);
    }

    dynamixelSetTx();

    return DYNAMIXEL_SUCCESS;
}

u08 dynamixelPing(u08 id)
{
    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 2;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_PING;

    return dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);
}

u08 dynamixelReadByte(u08 id, u08 address, u08* value)
{
    u08 result;

    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 4;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_READ;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) address;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) 1;

    result = dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);

    if(result == DYNAMIXEL_SUCCESS) {
        *value = dynamixelRxPacket[DYNAMIXEL_PARAMETER];
    }

    return result;
}

u08 dynamixelReadWord(u08 id, u08 address, u16* value)
{
    u08 result;

    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 4;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_READ;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) address;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) 2;

    result = dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);

    if (result == DYNAMIXEL_SUCCESS) {
        *value = (u16) makeWord(dynamixelRxPacket[DYNAMIXEL_PARAMETER], dynamixelRxPacket[DYNAMIXEL_PARAMETER+1]);
    }

    return result;
}

u08 dynamixelReadTable(u08 id, u08 startAddress, u08 endAddress, u08* table)
{
    u08 result;
    u08 length = endAddress - startAddress + 1;

    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 4;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_READ;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) startAddress;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) length;

    result = dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);

    if (result == DYNAMIXEL_SUCCESS) {
        for (u08 i = 0; i < length; i++) {
            table[startAddress + i] = dynamixelRxPacket[DYNAMIXEL_PARAMETER + i];
        }
    }

    return result;
}

u08 dynamixelWriteByte(u08 id, u08 address, u08 value)
{
    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 4;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_WRITE;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) address;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) value;

    return dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);
}

u08 dynamixel_writeword(u08 id, u08 address, u16 value)
{
    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 5;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_WRITE;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) address;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) getLowByte(value);
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+2] = (u08) getHighByte(value);

    return dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);
}

u08 dynamixelSyncWrite(u08 address, u08 length, u08 number, u08* param)
{
    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) DYNAMIXEL_BROADCAST_ID;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_SYNC_WRITE;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER]   = (u08) address;
    dynamixelTxPacket[DYNAMIXEL_PARAMETER+1] = (u08) length;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) ((length + 1) * number + 4);

    for(u08 i = 0; i < ((length + 1) * number); i++) {
        dynamixelTxPacket[DYNAMIXEL_PARAMETER + 2 + i] = (u08) param[i];
    }

    return dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);
}

u08 dynamixelReset(u08 id)
{
    dynamixelTxPacket[DYNAMIXEL_ID]          = (u08) id;
    dynamixelTxPacket[DYNAMIXEL_LENGTH]      = (u08) 2;
    dynamixelTxPacket[DYNAMIXEL_INSTRUCTION] = (u08) DYNAMIXEL_RESET;

    return dynamixelTxRx(dynamixelTxPacket, dynamixelRxPacket);
}

u16 makeWord(u08 lowbyte, u08 highbyte)
{
    return ((highbyte << 8) + lowbyte);
}

u08 getLowByte(u16 word)
{
    return (word & 0xff);
}

u08 getHighByte(u16 word)
{
    return ((word & 0xff00) >> 8);
}
