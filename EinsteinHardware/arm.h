#ifndef ARM_H
#define ARM_H

#include "hwglobal.h"
#include "hwutils.h"
#include "servo8t.h"

typedef enum
{
	SHOULDER_ROTATE = 1,
	SHOULDER = 2,
	ELBOW = 3,
	WRIST = 4,
	WRIST_ROTATE = 5,
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
	LEVEL,
	VERTICAL_LEFT,
	VERTIAL_RIGHT
} wirstRotationPositionName;

typedef enum
{
	ROTATE_HOME = 127,
	BIN_RIGHT = 72,
	BIN_LEFT = 177,
	SIDE_RIGHT = 12,
	SIDE_LEFT = 238
} armRotatePosition;

typedef enum
{
	GRIPPER_CLOSED = 20, // was 85
	GRIPPER_HOME = 50,
	GRIPPER_OPEN_GRAB = 63,
	GRIPPER_OPEN_BIN = 58
} gripperPosition;

void raiseArm(void);

void initArm(void);
void setHomePosition(armServo servo, int position);
void armOn(void);
void armOff(void);
void returnArm(void);
void moveArmToPos(armPositionName posName);

// Rotates one tick if in range. Returns FALSE if not
// Leaves arm bend in current position. Does not ensure "Clearence"
BOOL rotateArm(side rotateSide);

void rotateWristToPos(int destPos);
void grip(gripperPosition pos);
void closeGripper(void);
void openGripper(void);
armRotatePosition getArmRotateSide(side rotateSide);

#endif

