#ifndef MOTORS_H
#define MOTORS_H

#include "../global.h"

#include "utils.h"

#define LEFT_STRAIGHT_SPEED 150
#define RIGHT_STRAIGHT_SPEED 150

void testMotors(void);
void initMotors(void);

// Stop both sides
void halt(void);

// One side independent
void setSpeed(side wheel, direction motorDir, u08 speed);

// Both sides same direction
void go(direction dir, u08 speed);
void move(direction dir, u08 speed, u32 amount);

// Both sides opposite directions
void spin(side spinSide, u08 speed);
void twist(side spinSide, u08 speed, u32 amount);

#endif
