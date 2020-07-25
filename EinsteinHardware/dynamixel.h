
#ifndef _DYNAMIXEL_H_
#define _DYNAMIXEL_H_

#include <avr/io.h>

#include "hwglobal.h"

#define DYNAMIXEL_BAUDRATE 1000000

#define DYNAMIXEL_OK      1
#define DYNAMIXEL_TIMEOUT 2

#define DYNAMIXEL_ID           2
#define DYNAMIXEL_LENGTH       3
#define DYNAMIXEL_INSTRUCTION  4
#define DYNAMIXEL_ERROR        4
#define DYNAMIXEL_PARAMETER    5

#define DYNAMIXEL_BROADCAST_ID  254
#define DYNAMIXEL_PACKET_SIZE   128

#define DYNAMIXEL_PING        1
#define DYNAMIXEL_READ        2
#define DYNAMIXEL_WRITE       3
#define DYNAMIXEL_REG_WRITE   4
#define DYNAMIXEL_ACTION      5
#define DYNAMIXEL_RESET       6
#define DYNAMIXEL_SYNC_WRITE  131

#define DYNAMIXEL_SUCCESS     1
#define DYNAMIXEL_RX_CORRUPT  2
#define DYNAMIXEL_RX_TIMEOUT  3
#define DYNAMIXEL_TX_FAIL     4
#define DYNAMIXEL_TX_TIMEOUT  5

void initDynamixel(u08 avrUart);
void dynamixelSetTx(void);
void dynamixelSetRx(void);
void dynamixelWrite(u08 c);

u08 dynamixelCalculateChecksum(volatile u08* packet);
u08 dynamixelWritePacket(volatile u08* packet, u08 length);
u08 dynamixelReadPacket(volatile u08* packet, u08 length);
u08 dynamixelTxRx(volatile u08* txpacket, volatile u08* rxpacket);

u08 dynamixelPing(u08 id);
u08 dynamixelReadByte(u08 id, u08 address, u08* value);
u08 dynamixelReadWord(u08 id, u08 address, u16* value);
u08 dynamixelReadTable(u08 id, u08 start_address, u08 end_address, u08* table);
u08 dynamixelWriteByte(u08 id, u08 address, u08 value);
u08 dynamixel_writeword(u08 id, u08 address, u16 value);
u08 dynamixelSyncWrite(u08 address, u08 length, u08 number, u08* param);
u08 dynamixelReset(u08 id);

u16 makeWord(u08 lowbyte, u08 highbyte);
u08 getLowByte(u16 word);
u08 getHighByte(u16 word);

#endif
