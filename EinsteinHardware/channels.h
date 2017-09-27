#ifndef CHANNELS_H
#define CHANNELS_H

#include "hwglobal.h"

struct uartChannel
{
	uartName name;
	u32 baudRate;
	u08 avrUart;
	u08 muxChip;
	u08 muxChannel;
	u08 muxAddressA;
	u08 muxAddressB;
};

struct uartChannel selectUartChannel(uartName name);

#endif
