
//*****************************************************************************
//
// File Name	: 'global.h'
// Title		: AVR project global include 
// Author		: Pascal Stang
// Created		: 7/12/2001
// Revised		: 9/30/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
//	Description : This include file is designed to contain items useful to all
//					code files and projects.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef GLOBAL_H
#define GLOBAL_H

#include "../global.h"

//#define RPRINTF_COMPLEX

#ifndef _AVR_
	#define _LOG_
	//#define F_CPU 8000000
#endif

#define TIME_FACTOR ((F_CPU+250000)/500000)

#ifdef _AVR_

	// Uncomment the following line to LOG output
	//#define _LOG_

	// PWM on Port B2 - comment out this line to stop it
	//#define _AVR_PWM_

	// Binary output on SOUND_PORT, SOUND_BIT - comment out this line to stop it
	//#define _AVR_BINARY_

	// 4 port output on QUAD_PORT using pins QUAD_PIN, QUAD_PIN + 1, QUAD_PIN + 2 and QUAD_PIN + 3
	#define _AVR_QUAD_

	#include <avr/io.h>
	#include <avr/pgmspace.h>
	//#include "avrlibdefs.h"
	//#include "avrlibtypes.h"
	#include "uart2.h"
	#include "rprintf.h"
	#include <avr/sleep.h>

	// Specify the output port pin to use for binary sound control
	#ifdef _AVR_BINARY_
		#define SOUND_PORT PORTB
		#define SOUND_DDR DDRB
		#define SOUND_BIT BV(PB4)
	#endif

	// Specify the output port pin to use for quad sound control
	#ifdef _AVR_QUAD_
		#define QUAD_PORT PORTB
		#define QUAD_DDR  DDRB
		#define QUAD_BIT  PB0
		#define QUAD_MASK (BV(QUAD_BIT) | BV(QUAD_BIT+1) | BV(QUAD_BIT+2) | BV(QUAD_BIT+3)) 
	#endif


#else
	// Compiling for windows
	#ifdef _LOG_
		#include <stdio.h>
		#define loggerP(s) logger(s)
	#endif
	#define PROGMEM
	#include <memory.h>
	#define pgm_read_byte(addr) *addr
	#define pgm_read_word(addr) *addr
	#define memcpy_P(a,b,c) memcpy(a,b,c);
	typedef unsigned char uint8_t;
	typedef signed char int8_t;
	typedef int int16_t;
	#define PSTR(s) s
	void setLogFile(FILE * file);
#endif


#ifndef TRUE
#define FALSE 0
#define TRUE -1
#endif 
typedef int8_t boolean;



typedef struct Vocab {
	const char* txt;
	const char* phoneme;
} VOCAB;

typedef struct Phoneme {
	const char* txt;
	const char* phoneme;
	uint8_t attenuate;
} PHONEME;

typedef struct strSoundIndex{
	uint8_t SoundNumber;
	int8_t byte1;
	uint8_t byte2;
} SOUND_INDEX;

#include "core.h"
#include "speech.h"

#ifdef _AVR_
	// Phase and frequency correct - no prescale
	#define PWM_FLAGS ((1 << WGM13) | (1 << CS10))

	// 20kHz PWM
	#if F_CPU == 7372800
	#define PWM_TOP (600/2)

	#elif F_CPU == 14745600
	#define PWM_TOP (1200/2)

	#elif F_CPU == 8000000
	#define PWM_TOP (600/2)

	#elif F_CPU== 16000000
	#define PWM_TOP (1200/2)

	#elif F_CPU== 20000000
	#define PWM_TOP (1500/2)

	#elif F_CPU== 1000000
	#define PWM_TOP (600/(2*8))
	#endif

#endif

void init(void);

#define min(a,b) (a < b) ? a : b

#define RPRINTF_FLOAT

#define UARTS_MULTIPLEXED

typedef enum
{
	VOID, // No uart has been selected
	COMPUTER,
	CAMERA,
	SERVOS,
	USB,
	BLUETOOTH,
	ASSISTANT
} uartName;

#define EXTERNAL_MOTOR_CONTROL

#define SERVOS_UART 0
#define SERVOS_UART_BAUDRATE 19200

#define CAMERA_UART 0
#define CAMERA_UART_BAUDRATE 115200

#define COMPUTER_UART 0
#define COMPUTER_UART_BAUDRATE 115200

#define BLUETOOTH_UART 1
#define BLUETOOTH_UART_BAUDRATE 115200

#define USB_UART 1
#define USB_UART_BAUDRATE 115200

#define DEBUG_LED

#ifdef DEBUG_LED
#define DEBUG_LED_PORT PORTB
#define DEBUG_LED_BIT PB0
#endif

//define INPUT_BUTTON
#ifdef INPUT_BUTTON
//define INPUT_BUTTON_NC // Normally-Closed
#define INPUT_BUTTON_NO // Normally-Open
#endif

#ifdef INPUT_BUTTON
#define INPUT_BUTTON_PORT PORTX
#define INPUT_BUTTON_BIT PXX
#endif

void setRemoteComm(uartName newRemoteComm);
// returns UART number of AVR (0,1,2, etc)
u08 getRemoteComm(void);
uartName getRemoteCommName(void);

#endif
