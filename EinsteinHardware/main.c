#include <avr/io.h>

#include "../twi.h"
#include "../rprintf.h"
#include "../uart2.h"

#include "core.h"

int main(void)
{
	i2cSendStart();
	rprintf("s");
	uartInit();
	say("Hello");
	
    /* Replace with your application code */
    while (1) 
    {
    }
}

