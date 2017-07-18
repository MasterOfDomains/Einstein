#ifndef SERVO8T_H
#define SERVO8T_H

#include "utils.h"

void initServo8t(void);
void moveServo(u08 servo, u08 pos);
u08 getServoPos(u08 servo);
u16 getTorque(u08 servo);
void testTorque(void);
void setHome(u08 servo);

#endif
