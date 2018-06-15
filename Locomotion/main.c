#include "../global.h"

#include <avr/io.h>
#include <util/delay.h>

#include "../twi.h"
#include "../rprintf.h"

#include "uart.h"
#include "encoder.h"
#include "lutils.h"
#include "interface.h"
#include "motors.h"

void init(void);
void initPorts(void);

int main (void)
{
    _delay_ms(3000);
    init();
    //testMotors();
    while (1) {
        struct commandStruct command = waitForCommand();
        rprintf("Dir = %d", command.commandDir);
        rprintfCRLF();
        rprintf("Side = %d", command.commandSide);
        rprintfCRLF();
        rprintf("Amount = %d", command.distance);
        rprintfCRLF();
        rprintf("Speed = %d", command.speed);
        rprintfCRLF();

        switch (command.name) {
        case HARD_STOP:
            halt();
            break;
        case SOFT_STOP:
            halt();
            break;
        case GO:
            go(command.commandDir, command.speed);
            break;
        case MOVE:
            move(command.commandDir, command.speed, command.distance, TRUE);
            break;
        case SPIN:
            spin(command.commandSide, command.speed);
            break;
        case TWIST:
            twist(command.commandSide, command.speed, command.distance);
            break;
        }
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
    initMotors();
    initEncoders();
    initInterface();
}

void initPorts(void)
{
    DDRB = 0b00000110;
    //       ||||||||
    //       |||||||\___0: Interrupter, Pin 14
    //       ||||||\____1: LED , Pin 15
    //       |||||\_____2: Interrupt Out (SS'), Pin 16
    //       ||||\______3: \
    //       |||\_______4:  > Programming (SPI)
    //       ||\________5: /
    //       |\_________6: Clock In, Pin 9
    //       \__________7: (Leave floating), Pin 10

    //cbi(DDRB, PB0); // No pull-up on Interrupter
    sbi(DDRB, PB1); // LED off
    cbi(DDRB, PB2); // Interrupt off
    cbi(DDRB, PB6); // No pull-up on clock input
    cbi(DDRB, PB7); // No pull-up on floating clock input

    DDRC = 0b10111111;
    //       ||||||||
    //       |||||||\___0: Dir Left, Pin 23
    //       ||||||\____1: Dir Left, Pin 24
    //       |||||\_____2: Dir Right, Pin 25
    //       ||||\______3: Dir Right, Pin 26
    //       |||\_______4: I2C (SDA), Pin 27
    //       ||\________5: I2C (SLC), Pin 28
    //       |\_________6: Reset, Pin 1
    //       \__________7: (Not Available)

    //cbi(DDRC, PC6); // No pull-up for Reset input

    DDRD = 0b01100010;
    //       ||||||||
    //       |||||||\___0: UART Rx (RXD), INPUT, Pin 2
    //       ||||||\____1: UART Tx (TXD), OUTPUT, Pin 3
    //       |||||\_____2: Encoder A (Right), Pin 4
    //       ||||\______3: Encoder A (Left), Pin 5
    //       |||\_______4: Encoder B (Right), Pin 6
    //       ||\________5: PWM Right (OC0B), Pin 11
    //       |\_________6: PWM Left (OC0A), Pin 12
    //       \__________7: Encoder B (Left), Pin 13

    /*
    cbi(DDRD, PD0); // No pull-up on UART Rx
    cbi(DDRD, PD2); // No pull-up for encoder input
    cbi(DDRD, PD3); // No pull-up for encoder input
    cbi(DDRD, PD4); // No pull-up for encoder input
    cbi(DDRD, PD7); // No pull-up for encoder input
    */
}
