#include <avr/io.h>

#include "../twi.h"
#include "../rprintf.h"
#include "../uart2.h"

#include "core.h"
#include "utils.h"
#include "servo8t.h"
#include "motors.h"
#include "arm.h"

int main(void)
{
	i2cSendStart();
	rprintf("s");
	uartInit();
	say("Hello");
	debugLEDoff();
	initServo8t();
	initMotors();
	initArm();
	
    /* Replace with your application code */
    while (1) 
    {
    }
}

