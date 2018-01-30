#include "channels.h"

#include <avr/io.h>

#include "uart2.h"

#include "hwutils.h"

uartName currentUart0Channel = VOID;
uartName currentUart1Channel = VOID;

struct uartChannel getUartChannel(uartName name);

struct uartChannel selectUartChannel(uartName name)
{
	struct uartChannel channel;

	channel = getUartChannel(name);
	if ((channel.avrUart == 0 && currentUart0Channel != channel.name) ||
	    (channel.avrUart == 1 && currentUart1Channel != channel.name))
	{
		if (channel.avrUart == 0)
			currentUart0Channel = channel.name;
		else if (channel.avrUart == 1)
			currentUart1Channel = channel.name;

		// Switching a UART's channel requires resetting baud rate
		uartSetBaudRate(channel.avrUart, channel.baudRate);

		switch (channel.muxChannel)
		{
			case (0):
				PORT_OFF(PORTD, channel.muxAddressA);
				PORT_OFF(PORTD, channel.muxAddressB);
				break;
			case (1):
				PORT_ON(PORTD, channel.muxAddressA);
				PORT_OFF(PORTD, channel.muxAddressB);
				break;
			case (2):
				PORT_OFF(PORTD, channel.muxAddressA);
				PORT_ON(PORTD, channel.muxAddressB);
				break;
			case (3):
				PORT_ON(PORTD, channel.muxAddressA);
				PORT_ON(PORTD, channel.muxAddressB);
				break;
		}
	}
	return channel;
}

struct uartChannel getUartChannel(uartName name)
{
	struct uartChannel channel;
	channel.name = name;
	switch (name)
	{
		case (COMPUTER):
			channel.avrUart = COMPUTER_UART;
			channel.baudRate = COMPUTER_UART_BAUDRATE;
			channel.muxChip = 2;
			channel.muxChannel = 1;
			channel.muxAddressA = PD6;
			channel.muxAddressB = PD7;
			break;
		case (CAMERA):
			channel.avrUart = CAMERA_UART;
			channel.baudRate = CAMERA_UART_BAUDRATE;
			channel.muxChip = 2;
			channel.muxChannel = 2;
			channel.muxAddressA = PD6;
			channel.muxAddressB = PD7;
			break;
		case (SERVOS):
			channel.avrUart = SERVOS_UART;
			channel.baudRate = SERVOS_UART_BAUDRATE;
			channel.muxChip = 2;
			channel.muxChannel = 0;
			channel.muxAddressA = PD6;
			channel.muxAddressB = PD7;
			break;
		case (BLUETOOTH):
			channel.avrUart = BLUETOOTH_UART;
			channel.baudRate = BLUETOOTH_UART_BAUDRATE;
			channel.muxChip = 1;
			channel.muxChannel = 1;
			channel.muxAddressA = PD4;
			channel.muxAddressB = PD5;
			break;
		default: // USB
			channel.avrUart = USB_UART;
			channel.baudRate = USB_UART_BAUDRATE;
			channel.muxChip = 1;
			channel.muxChannel = 2;
			channel.muxAddressA = PD4;
			channel.muxAddressB = PD5;
	}
	return channel;
}
