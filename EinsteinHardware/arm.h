#ifndef ARM_H
#define ARM_H

#include "hwglobal.h"
#include "hwutils.h"
#include "servo8t.h"

#define MAX_SHOULDER_BEND -159 // Most extended
#define MIN_SHOULDER_BEND -50 // Most retracted
#define MAX_ELBOW_BEND -120 // Most extended
#define MIN_ELBOW_BEND 2 // Most folded
#define MAX_ROTATE_LEFT 238
#define MAX_ROTATE_RIGHT 12




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
{ // Relative Positions
	BEND_HOME,
	FLOOR_GRAB,
	FLOOR_LOOK,
	BIN_DROP
} armBendPosition;

// Absolute Positions ---------------------------------------------------------

typedef enum
{
	ROTATE_HOME = 127,
	BIN_RIGHT = 72,
	BIN_LEFT = 177,
	SIDE_RIGHT = 12,
	SIDE_LEFT = 238
} armRotatePosition;

/*
typedef enum
{
	VERTICAL = ,
	HORIZONTAL = 
} wristRotatePosition;
*/

typedef enum
{
	GRIPPER_CLOSED = 20, // was 85
	GRIPPER_HOME = 50,
	GRIPPER_OPEN_GRAB = 63,
	GRIPPER_OPEN_BIN = 58
} gripperPosition;

// -----------------------------------------------------------------------------

typedef enum
{
	ARM_BEND_FLOOR_GRAB_1 = 0,
	ARM_BEND_FLOOR_GRAB_2 = 1,
	ARM_BEND_FLOOR_GRAB_3 = 2,
	ARM_BEND_FLOOR_GRAB_4 = 3,
	ARM_BEND_FLOOR_GRAB_5 = 4,
	ARM_BEND_CARGO_FEEL_1 = 5,
	ARM_BEND_CARGO_FEEL_2 = 6,
	ARM_BEND_CARGO_FEEL_3 = 7,
	ARM_BEND_CARGO_FEEL_4 = 8,
	ARM_BEND_CARGO_FEEL_5 = 9,
	ARM_BEND_INIT = 10,
	ARM_BEND_ROTATE = 11,
	ARM_BEND_FLOOR_LOOK = 12,
	ARM_BEND_FLOOR_DROP = 13,
	ARM_BEND_FLOOR_DROP_A = 14,
	ARM_BEND_FLOOR_DROP_B = 15,
	ARM_BEND_BIN_DROP = 16,
	ARM_BEND_BIN_CLEAR = 17,
	ARM_BEND_BIN_GRAB = 18,
	ARM_BEND_BIN_MEASURE_RAIL = 19,
	ARM_BEND_BIN_MEASURE_SEA = 20,
	ARM_BEND_BIN_MEASURE_AIR = 21,
	ARM_BEND_PLATFORM_LOOK = 22
} armBendRelationsName;

#define OVER_ROTATE_ALLOWANCE 0

struct armBendRelations
{
	// These are added to "Init" positions to determine
	// the actual positions
	s16 shoulder;
	s16 elbow;
};

//struct armBendRelations armBendHome = { -60, -68 };

void raiseArm(void);

void initArm(void);
void armOn(void);
void armOff(void);
void returnArm(void);
void bendArmToPos(armBendRelationsName posName);
void displayArmPositions(void);

// Rotates one tick if in range. Returns FALSE if not
// Leaves arm bend in current position. Does not ensure "Clearence"
BOOL rotateArm(side rotateSide);
// Rotates to predetermined position. Set "clear" to ensures clearence
void rotateArmToPos(armRotatePosition pos, BOOL clear);
void grip(gripperPosition pos);
void closeGripper(void);
void openGripper(void);
armRotatePosition getArmRotateSide(side rotateSide);

#endif

