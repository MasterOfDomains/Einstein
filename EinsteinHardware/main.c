#include <avr/io.h>

#include "../twi.h"
#include "../rprintf.h"
#include "../uart2.h"

int main(void)
{
	i2cSendStart();
	rprintf("s");
	uartInit();
	
    /* Replace with your application code */
    while (1) 
    {
    }
}

