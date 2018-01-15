#include "hwglobal.h"

#include "../twi.h"
#include "../rprintf.h"
#include "../a2d.h"
#include "../utils.h"

#include "uart2.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "motors.h"
#include "channels.h"
#include "hwutils.h"
#include "arm.h"
#include "servo8t.h"
#include "compass.h"

void initRobot(void);
void initPorts(void);
void signalStart();
void shortDemo();

void wait(u16 miliseconds) {
	for (u16 i = 0; i < miliseconds; i++) _delay_ms(1);
}

int main(void)
{
	_delay_ms(3000);
	initRobot();
	
	//testMotors();
	
	shortDemo();

	//u08 sendDataLength = 4;
	//u08 sendData[sendDataLength];
	//sendData[0] = 'S';
	//sendData[1] = 'P';	i2cMasterSend(0, 0, sendData);

	//printf("myvar = " + myvar);
	//wait(myvar);
	//go(FORWARD, 123);

	while(1);

	return 0;
}

void shortDemo() {
	raiseArm();
	twist(LEFT, 255, 20);
	rotateArmToPos(88, FALSE); // Right
	halt();
	_delay_ms(500);
	twist(RIGHT, 255, 40);
	rotateArmToPos(168, FALSE); // Left
	halt();
	_delay_ms(500);
	twist(LEFT, 255, 20);
	rotateArmToPos(128, FALSE);
	halt();
	_delay_ms(500);
	closeGripper();
	_delay_ms(500);
	openGripper();
	_delay_ms(500);
	rotateArmToPos(BIN_LEFT, FALSE);
	closeGripper();
	_delay_ms(500);
	openGripper();
	_delay_ms(500);
	rotateArmToPos(BIN_RIGHT, FALSE);
	closeGripper();
	_delay_ms(500);
	openGripper();
	_delay_ms(500);
	rotateArmToPos(ROTATE_HOME, FALSE);
}

void initRobot(void)
{
	initPorts();

	signalStart();

	armOff();

	// UARTs and printf
	uartInit();
	setRemoteComm(USB);
	rprintfCRLF();
	rprintfProgStrM("Starting...\n\r");
	rprintfCRLF();

	// ADCs
	a2dInit();
	a2dSetPrescaler(ADC_PRESCALE_DIV32);
	a2dSetReference(ADC_REFERENCE_AVCC);
	
	initHead();
	
	// Two-Wire Interface Devices
	//i2cInit();
	//initMotors();
	
	/*
	initCompass();  // Calls i2cInit
	testCompass();
	*/
	
	// Arm
	//initServo8t();
	//initArm();
	//armOn();
}

static void dutyCycleLED(u08 dutyCycle, u08 pulseWidth, u16 *currentTime) {
		debugLEDoff();
		for (u16 tick = 0; tick < (pulseWidth - dutyCycle); tick++) {
			_delay_ms(1);
			(*currentTime)++;
		}
		debugLEDon();
		for (u16 i = 0; i < dutyCycle; i++) {
			_delay_ms(1);
			(*currentTime)++;
		}
}

void signalStart() {
	static const int NUMBER_OF_WINKS = 3;
	for (u08 wink = 0; wink < NUMBER_OF_WINKS; wink++) {
		u16 currentTime = 0;
		const u16 endTime = 1000;
		const u08 pulseWidth = 20;
		while (currentTime < endTime)
		{
			u08 dutyCycle = currentTime/100;
			dutyCycleLED(dutyCycle, pulseWidth, &currentTime);
		}
		_delay_ms(500);
	}
	debugLEDoff();
}

void initPorts(void)
{
	DDRA = 0b00000000;  //configure all A ports for input				0x00
	PORTA = 0b00000000; //make sure pull-up resistors are turned off	0x00

	// ANALOG PORTS
	DDRA = 0b11111111;
	//       ||||||||
	//       |||||||\___0: 
	//       ||||||\____1: 
	//       |||||\_____2: 
	//       ||||\______3: 
	//       |||\_______4: 
	//       ||\________5: 
	//       |\_________6: 
	//       \__________7:

	// DIGITAL PORTSC:\Robot\Zeus2014\Zeus2014.hex
	DDRB = 0b11111111;
	//       ||||||||
	//       |||||||\___0: Debug LED, OUTPUT, Pin 1
	//       ||||||\____1: Clock Out, Pin 2
	//       |||||\_____2: Left Motor FORWARD, OUTPUT, Pin 3 / Interrupt In
	//       ||||\______3: Left Motor PWM (OC0A), OUTPUT, Pin 4
	//       |||\_______4: Right Motor PWM (OC0B), OUTPUT, Pin 5
	//       ||\________5: Left Motor BACKWARD, OUTPUT, Pin 6
	//       |\_________6: Right Motor FORWARD, OUTPUT, Pin 7
	//       \__________7: Right Motor BACKWARD, OUTPUT, Pin 8

	DDRC = 0b11111011;
	//       ||||||||
	//       |||||||\___0: I2C SCL, I/O, 
	//       ||||||\____1: I2C SDA, I/O,
	//       |||||\_____2: Interrupter, Pin 24
	//       ||||\______3: Battery Monitor, Pin 25
	//       |||\_______4: 
	//       ||\________5: 
	//       |\_________6: 
	//       \__________7: Arm Power, Pin 29

	//cbi(DDRB, PC2); // No pull-up on Interrupter
	DDRD = 0b11111010;
	//       ||||||||
	//       |||||||\___0: UART 0 Rx (RXD0), INPUT, Pin 14
	//       ||||||\____1: UART 0 Tx (TXD0), OUTPUT, Pin 15
	//       |||||\_____2: UART 1 Rx (RXD1), INPUT, Pin 16
	//       ||||\______3: UART 1 Tx (TXD1), OUTPUT, Pin 17
	//       |||\_______4: UART Select, OUTPUT, Pin 18
	//       ||\________5: UART Select, OUTPUT, Pin 19
	//       |\_________6: UART Select, OUTPUT, Pin 20
	//       \__________7: UART Select, OUTPUT, Pin 21
}

