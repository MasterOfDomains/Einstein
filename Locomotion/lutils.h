#ifndef LUTILS_H
#define LUTILS_H

#include <avr/io.h>		    // I/O definitions (port names, pin names, etc)

#include "../global.h"
#include "../utils.h"

#define ASCII_ESCAPE 27

#define PORT_ON( port_letter, number )			port_letter |= (1<<number)
#define PORT_OFF( port_letter, number )			port_letter &= ~(1<<number)
#define FLIP_PORT( port_letter, number )		port_letter ^= (1<<number)
#define PORT_IS_ON( port_letter, number )		( port_letter & (1<<number) )
#define PORT_IS_OFF( port_letter, number )		!( port_letter & (1<<number) )

typedef enum {
    INVALID_SPEED = 0,
    I2C_COMM_ERROR = 1,
    I2C_RECEIVED = 2
} errorCode;

// Unrecoverable Error has happened
void signalFatalError(errorCode error);

// Debug LED
void LED_off(void);
void LED_on(void);
void debugLEDtoggle(void);

// Waits for ASCII character from UART
// terminal Emulator. Returns TRUE if the button pressed was the
// 'Escape' key
BOOL waitForButton(void);

// Gets Y/N from terminal Emulator
BOOL menu_getConfirm(void);

// Manual Control Functions
char getKeyTap(BOOL crlf);
side menu_chooseSide(void);

void printSide(side printSide, BOOL crlf);

#endif
