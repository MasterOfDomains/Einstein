#ifndef HWUTILS_H
#define HWUTILS_H

#include "hwglobal.h"

#include <avr/io.h>		    // I/O definitions (port names, pin names, etc

#include "../utils.h"


#define ASCII_ESCAPE 27

#define PORT_ON( port_letter, number )			port_letter |= (1<<number)
#define PORT_OFF( port_letter, number )			port_letter &= ~(1<<number)
#define FLIP_PORT( port_letter, number )		port_letter ^= (1<<number)
#define PORT_IS_ON( port_letter, number )		( port_letter & (1<<number) )
#define PORT_IS_OFF( port_letter, number )		!( port_letter & (1<<number) )

struct point
{
	u08 x;
	u08 y;
};

typedef enum
{
	UART_ERROR = 0,
	INVALID_SERVO = 1,
	SERVO8T_COMM_ERROR = 2,
	AVRCAM_COMM_ERROR = 3,
	INVALID_SPEED = 4,
	OUT_OF_SENSOR_RANGE = 5,
	I2C_COMM_ERROR = 6,
	INVALID_ARM_POSITION = 7,
	INVALID_SENSOR = 8
} errorCode;

typedef enum
{
	IR_LEFT,
	IR_FRONT,
	IR_RIGHT,
	IR_ARM,
	SONAR_LEFT,
	SONAR_RIGHT
} distSensor;

typedef enum
{
	IR,
	SONAR
} distSensorType;

// Retrieves a positive integer from UART terminal emulator
u32 getNumber(u08 maxDigits);

// Retrieves a string from UART terminal emulator
/* Example
	char *str;
	str = getString(10);
*/
// CAUTION: Caller must FREE the memory
char *getString(u08 maxLength);

// Waits for a byte for limited amount of time
BOOL uartReceiveByte_TL(u08 uart, u08 *byte, u08 seconds);

// Returns element from distSensor enumeration
distSensor getSideDistSensor(side sensorSide, distSensorType sensorType);

// Unrecoverable Error has happened
void signalFatalError(errorCode error);

// For very accurate delays
void delay_cycles(unsigned long int cycles);

// Trig Functions
signed int cos_SoR(long signed int degrees);
signed int sin_SoR(long signed int degrees);
signed int tan_SoR(long signed int degrees);

// Doesn't work?
void flipSign(s32 *number);

// Debug LED
void debugLEDoff(void);
void debugLEDon(void);
void debugLEDtoggle(void);

void switchBluetoothMode(void);

// Returns true if physical input button is pressed
u16 button_pressed(void);

// Waits for physical input button or ASCII character from UART
// terminal Emulator. Returns TRUE if the button pressed was the
// 'Escape' key
BOOL waitForButton(void);

// Gets Y/N from terminal Emulator
BOOL menu_getConfirm(void);

// Manual Control Functions
char getKeyTap(BOOL crlf);
side menu_chooseSide(void);
distSensor menu_chooseDistSensor(void);

// Read sensors
u16 getAccel(orientation way);
float getDistance(distSensor sensor);

// Tests sensors in endless loop
void testDistSensor(distSensor sensor);
void testAccel(void);

void printSide(side printSide, BOOL crlf);

#endif
