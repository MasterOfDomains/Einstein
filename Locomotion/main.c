#include "../global.h"

#include <avr/io.h>
#include <util/delay.h>

#include "../twi.h"
#include "../rprintf.h"

#include "uart.h"
#include "utils.h"
#include "interface.h"

void init(void);
void initPorts(void);

int main (void)
{
	_delay_ms(3000);
	init();
	while (1)
	{
		struct commandStruct command = waitForCommand();
		rprintf("Dir = %d", command.commandDir);
		rprintfCRLF();
		rprintf("Side = %d", command.commandSide);
		rprintfCRLF();
		rprintf("Amount = %d", command.distance);
		rprintfCRLF();
		rprintf("Speed = %d", command.speed);
		rprintfCRLF();
	}
	return 1;
}

void init(void)
{
	initPorts();
	LED_off();
	uartInit();
	uartSetBaudRate(115200);
	rprintfInit(uartSendByte);
	rprintfProgStrM("Starting...");
	rprintfCRLF();
	initInterface();
}

void initPorts(void)
{
	DDRB = 0b00000111;
	//       ||||||||
	//       |||||||\___0: Dir Right, Pin 14
	//       ||||||\____1: LED , Pin 15
	//       |||||\_____2: Interrupt Out (SS'), Pin 16
	//       ||||\______3: \
	//       |||\_______4:  > Programming (SPI)
	//       ||\________5: /
	//       |\_________6: Clock In, Pin 9
	//       \__________7: (Leave floating), Pin 10

	sbi(DDRB, PB1); // LED off
	cbi(DDRB, PB2); // Interrupt off
	cbi(DDRB, PB6); // No pull-up on clock input
	cbi(DDRB, PB7); // No pull-up on floating clock input
	
	DDRC = 0b10110000;
	//       ||||||||
	//       |||||||\___0: Encoder B (Left), Pin 23
	//       ||||||\____1: Encoder A (Left), Pin 24
	//       |||||\_____2: Encoder B (Right), Pin 25
	//       ||||\______3: Encoder A (Right), Pin 26
	//       |||\_______4: I2C (SDA), Pin 27
	//       ||\________5: I2C (SLC), Pin 28
	//       |\_________6: Reset, Pin 1
	//       \__________7: (Not Available)

	cbi(DDRC, PC0); // No pull-up for encoder input
	cbi(DDRC, PC1); // No pull-up for encoder input
	cbi(DDRC, PC2); // No pull-up for encoder input
	cbi(DDRC, PC3); // No pull-up for encoder input

	DDRD = 0b11111010;
	//       ||||||||
	//       |||||||\___0: UART Rx (RXD), INPUT, Pin 2
	//       ||||||\____1: UART Tx (TXD), OUTPUT, Pin 3
	//       |||||\_____2: Interrupt In (INT0), Pin 4
	//       ||||\______3: Dir Left, Pin 5
	//       |||\_______4: Dir Left, Pin 6
	//       ||\________5: PWM Left (OC0B), Pin 11
	//       |\_________6: PWM Right (OC0A), Pin 12
	//       \__________7: Dir Right, Pin 13

	cbi(DDRD, PD0); // No pull-up on UART Rx
	cbi(DDRD, PD2); // No pull-up on Interrupt
	cbi(DDRD, PD5); // PWM off
	cbi(DDRD, PD6); // PWM off
}
