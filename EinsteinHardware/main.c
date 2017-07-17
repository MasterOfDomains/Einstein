#include <avr/io.h>

#include "../twi.h"
#include "../rprintf.h"
#include "../uart2.h"

#include "core.h"
#include "utils.h"
//#include "servo8t.h"

int main(void)
{
	i2cSendStart();
	rprintf("s");
	uartInit();
	say("Hello");
	debugLEDoff();
	//initServo8t();

	
    /* Replace with your application code */
    while (1) 
    {
    }
}

