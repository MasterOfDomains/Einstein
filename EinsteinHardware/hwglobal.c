#include "hwglobal.h"

#include <util/delay.h>

#include "uart2.h"
#include "../rprintf.h"

struct remoteCommStruct
{
	uartName name;
	u08 avrUart;
} currentRemoteComm;

BOOL setRprintfUart(u08 uart);

BOOL setRprintfUart(u08 uart)
{
	BOOL success = TRUE;
	switch (uart)
	{
		case (0):
			rprintfInit(uart0SendByte);
			break;
		case (1):
			rprintfInit(uart1SendByte);
			break;
		default:
			success = FALSE;
	}
	return success;
}

void setRemoteComm(uartName newRemoteComm)
{
	if (newRemoteComm != currentRemoteComm.name)
	{
		BOOL success = TRUE;
		struct remoteCommStruct newRemoteCommStruct;
		newRemoteCommStruct.name = newRemoteComm;
		switch (newRemoteComm)
		{
			case (USB):
				newRemoteCommStruct.avrUart = USB_UART;
				break;
			case (BLUETOOTH):
				newRemoteCommStruct.avrUart = BLUETOOTH_UART;
				break;
			default:
				success = FALSE;
		}
		success = setRprintfUart(newRemoteCommStruct.avrUart);
		if (success)
			currentRemoteComm = newRemoteCommStruct;
		else
		{
			while (1)
			{
				rprintfProgStrM("Bad Remote Comm");
				rprintfCRLF();
				DEBUG_LED_PORT ^= (1 << DEBUG_LED_BIT);
				_delay_ms(250);
			}
		}
	}
}

uartName getRemoteCommName(void)
{
	return currentRemoteComm.name;
}

u08 getRemoteComm(void)
{
	return currentRemoteComm.avrUart;
}
