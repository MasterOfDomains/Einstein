#ifndef ARM_H
#define ARM_H

#include "hwglobal.h"
#include "hwutils.h"
#include "servo8t.h"
#include "armpos.h"

typedef enum
{
	SHOULDER_ROTATE = 1,
	SHOULDER = 2,
	ELBOW = 3,
	WRIST = 4,
	GRIPPER_ROTATE = 5,
	GRIPPER = 6
} armServo;

typedef enum
{
	INIT,
	HOME,
	CROUCH,
	LEAN,
	SIT
} armPositionName;

typedef enum
{
	GRIPPER_LEVEL,
	GRIPPER_VERTICAL_LEFT,
	GRIPPER_VERTIAL_RIGHT
} gripperRotationPositionName;

typedef enum
{ // 110 is rotated to either side
	ROTATE_HOME = 127,
	BIN_RIGHT = 72,
	BIN_LEFT = 177,
	SIDE_RIGHT = 12,
	SIDE_LEFT = 238
} armRotatePosition;

typedef enum
{
	GRIPPER_CLOSED = 82,
	GRIPPER_OPEN = 136
} gripperPosition;

void raiseArm(void);

void initArm(void);
void setHomePosition(armServo servo, int position);
void armOn(void);
void armOff(void);
void returnArm(void);
void moveArmToPos(armPositionName posName);
void moveArmServo(armServo servo, u08 dest);

// Rotates one tick if in range. Returns FALSE if not
// Leaves arm bend in current position. Does not ensure "Clearence"
BOOL rotateArm(side rotateSide);

// Pass numeric position. Pass 0 to return to initial (level) position.
void grip(gripperPosition pos);
// Pass Enum value only
void rotateGripper(gripperRotationPositionName positionName);

#endif

