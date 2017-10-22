#ifndef MOTORS_H
#define MOTORS_H

#include "hwglobal.h"

#include "hwutils.h"

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

#ifdef EXTERNAL_MOTOR_CONTROL
void externalGo(direction dir, u08 speed);
void externalMove(side moveSide, direction dir, u08 speed, float amount);
void externalSpin(side spinSide, u08 speed);
void externalTwist(side spinSide, u08 speed, float amount);
#endif

#endif
