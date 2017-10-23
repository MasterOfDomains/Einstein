#ifndef MOTORS_H
#define MOTORS_H

#include "../global.h"
#include "lutils.h"

#define INTERRUPTER_PORT PORTB
#define INTERRUPTER_BIT PB0

#define LEFT_STRAIGHT_SPEED 150
#define RIGHT_STRAIGHT_SPEED 150

void testMotors(void);
void testEncoders();

void initMotors(void);

// Stop both sides
void halt(void);

// One side independent
void setSpeed(side wheel, direction motorDir, u08 speed);

// Both sides same direction
void go(direction dir, u08 speed);
void move(direction dir, u08 speed, float distance, BOOL stop);

// Both sides opposite directions
void spin(side spinSide, u08 speed);
void twist(side spinSide, u08 speed, float amount);

#endif
