#include "arm.h"

/*------- Servo Positions And Directions of Movement --------------------------------

Servo				Number	Init	Home	Down		Up

SHOULDER_ROTATE		1		128		128		Clockwise	Counter-Clockwise
SHOULDER_EXTEND		2		85		122		Backward	Forward
ELBOW				3		221		88		Backward	Forward
WRIST				4		39		63		Forward		Backward
GRIPPER_ROTATE		5		122		120		Clockwise	Counter-Clockwise
GRIPPER				6		127		127		Close		Open

                     /   \
                     \   /
                      (o) Wrist
                      | |
                      | |
                      (o) Elbow
                      | |
                      | |
                      (o) Shoulder Extend
                    -------
                    |     |

              <-- Front Of Robot: Forward

-----------------------------------------------------------------------------------*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "hwutils.h"

#define ARM_ON_PORT PORTC
#define ARM_ON PC7
#define SERVO_DELAY_MS 10

#define NUM_SERVOS 6
#define MAX_SHOULDER_BEND -159 // Most extended
#define MIN_SHOULDER_BEND -50 // Most retracted
#define MAX_ELBOW_BEND -120 // Most extended
#define MIN_ELBOW_BEND 2 // Most folded
#define MAX_ROTATE_LEFT 238
#define MAX_ROTATE_RIGHT 12
#define GRIPPER_VERTICAL_DIST 96
#define OVER_ROTATE_ALLOWANCE 0

typedef struct armServoMovement
{
	armServo servo;
	u08 startPos;
	u08 *currPos;
	u08 destPos;
	s16 difference;
} armServoMovement;

struct armPositionStruct
{
	armPositionName name;
	s08 index;
	u08 shoulderRotate;
	u08 shoulder;
	u08 elbow;
	u08 wrist;
	u08 gripperRotate;
	u08 gripper;
};

struct armPositionsStruct
{
	struct armPositionStruct positions[20];
	u08 length;
};

struct armPositionsStruct armPositions;
struct armPositionStruct currentArmPosition;

struct armPositionStruct getArmPosition(armPositionName name);
struct armPositionStruct getNewArmPosition(armPositionName name);
struct armServoMovement getArmServoMovement(armServo servo, u08 destPos);
u08 *getCurrentArmServoPosition(armServo servo);
void incrementTowardPosition(armServoMovement movement);
void moveArmServo(armServo servo, u08 dest);
void populateArmPositions();
void printMovement(armServoMovement movement);

struct armPositionStruct getArmPosition(armPositionName name)
{
	struct armPositionStruct position;
	
	for (u08 i = 0; i < armPositions.length; i++) {
		if (armPositions.positions[i].name == name) {
			position = armPositions.positions[name];
			break;
		}
	}
	return position;
}

u08 *getCurrentArmServoPosition(armServo servo)
{
	u08 *returnVal;
	switch (servo)
	{
		case SHOULDER_ROTATE:
			returnVal = &currentArmPosition.shoulderRotate;
			break;
		case SHOULDER:
			returnVal = &currentArmPosition.shoulder;
			break;
		case ELBOW:
			returnVal = &currentArmPosition.elbow;
			break;
		case WRIST:
			returnVal = &currentArmPosition.wrist;
			break;
		case GRIPPER_ROTATE:
			returnVal = &currentArmPosition.gripperRotate;
			break;
		case GRIPPER:
			returnVal = &currentArmPosition.gripper;
			break;
		default:
			signalFatalError(INVALID_SERVO);
			returnVal = NULL;
	}
	return returnVal;
}

void raiseArm(void)
{
	moveArmServo(SHOULDER, getArmPosition(HOME).shoulder);
	moveArmServo(ELBOW, getArmPosition(HOME).elbow);
	moveArmServo(WRIST, getArmPosition(HOME).wrist);
}

void armOn()
{
	PORT_ON(ARM_ON_PORT, ARM_ON);
	_delay_ms(2000); // Relay movement and servo initialization
}

void armOff()
{
	_delay_ms(10); // Let last servo movement finish
	PORT_OFF(ARM_ON_PORT, ARM_ON);
}

void returnArm()
{
	//rotateArmToPos(ROTATE_HOME);
	grip(0);
	moveArmToPos(HOME);
}

void grip(gripperPosition pos)
{
	if (pos != 0)
		moveArmServo(GRIPPER, pos);
	else
		moveArmServo(GRIPPER, getArmPosition(INIT).gripper);
}

BOOL rotateArm(side rotateSide)
{
	BOOL withinRange = TRUE;
	u08 destPos = 0;
	u08 *position = getCurrentArmServoPosition(SHOULDER_ROTATE);
	if (rotateSide == LEFT)
	{
		destPos = *position + 1;
		if (destPos > SIDE_LEFT + OVER_ROTATE_ALLOWANCE)
			withinRange = FALSE;
	}
	else if (rotateSide == RIGHT)
	{
		destPos = *position - 1;
		if (destPos < SIDE_RIGHT - OVER_ROTATE_ALLOWANCE)
			withinRange = FALSE;
	} 
	else
	{
		destPos = getArmPosition(INIT).shoulderRotate;
	}
	if (withinRange)
		moveArmServo(SHOULDER_ROTATE, destPos);
	return withinRange;
}

void rotateGripper(gripperRotationPositionName positionName)
{
	u08 homePosition = getArmPosition(INIT).gripperRotate;
	rprintf("rotateGripper - homePosition: %d, GRIPPER_VERTICAL_DIST: %d\n\r", homePosition, GRIPPER_VERTICAL_DIST);
	u08 position = 0;
	switch (positionName)
	{
		case GRIPPER_LEVEL:
			rprintf("Going Home\n\r");
			position = homePosition;
			break;
		case GRIPPER_VERTIAL_RIGHT:
			rprintf("Turning RIGHT\n\r");
			position = homePosition - GRIPPER_VERTICAL_DIST;
			break;
		case GRIPPER_VERTICAL_LEFT:
			rprintf("Turning LEFT\n\r");
			position = homePosition + GRIPPER_VERTICAL_DIST;
			break;
	}
	moveArmServo(GRIPPER_ROTATE, position);
}


void moveArmServo(armServo servo, u08 dest)
{
	if (dest != 0)
	{
		s08 inc = 1;
		u08 *position = getCurrentArmServoPosition(servo);
		if (*position > dest)
			inc *= -1;
		while (dest != *position)
		{
			*position = (s16)*position + inc;
			moveServo(servo, *position);			
			_delay_ms(SERVO_DELAY_MS);
		}
	}
}

struct armServoMovement getArmServoMovement(armServo servo, u08 destPos)
{
	struct armServoMovement movement;
	movement.servo = servo;
	movement.currPos = getCurrentArmServoPosition(servo);
	movement.startPos = *movement.currPos;
	movement.destPos = destPos;
	if (destPos != 0)
		movement.difference = (s16)movement.destPos - (s16)movement.startPos;
	else
		movement.difference = 0;
	return movement;
}

void incrementTowardPosition(armServoMovement movement)
{
	s08 inc = 1;
	if (movement.destPos < *movement.currPos)
		inc = -1;
	moveArmServo(movement.servo, *movement.currPos + inc);
}

void moveArmToPos(armPositionName posName)
{
	armServoMovement movements[NUM_SERVOS];
	movements[SHOULDER_ROTATE - 1] = getArmServoMovement(SHOULDER_ROTATE, getArmPosition(posName).shoulderRotate);
	movements[SHOULDER - 1] = getArmServoMovement(SHOULDER, getArmPosition(posName).shoulder);
	movements[ELBOW - 1] = getArmServoMovement(ELBOW, getArmPosition(posName).elbow);
	movements[WRIST - 1] = getArmServoMovement(WRIST, getArmPosition(posName).wrist);
	movements[GRIPPER_ROTATE - 1] = getArmServoMovement(GRIPPER_ROTATE, getArmPosition(posName).gripperRotate);
	movements[GRIPPER - 1] = getArmServoMovement(GRIPPER, getArmPosition(posName).gripper);
	s16 largestMovement = 0;
	for (u08 servo = 0; servo < NUM_SERVOS; servo++)
	{
		if (movements[servo].difference != 0 && abs(movements[servo].difference) > abs(largestMovement))
			largestMovement = movements[servo].difference;
	}
	u08 percentComplete = 0;
	for (u08 moveIncrement = 0; moveIncrement < abs(largestMovement); moveIncrement++)
	{
		for (u08 servo = 0; servo < NUM_SERVOS; servo++)
		{
			armServoMovement movement = movements[servo];
			if (movement.difference != 0)
			{
				u08 amountMoved = abs(movement.startPos - *movement.currPos);
				if (movement.difference == largestMovement)
				{
					incrementTowardPosition(movement);
					percentComplete = ((u16)amountMoved * 100) / abs(movement.difference);
				}
				else
				{
					if ((((u16)amountMoved * 100) / movement.difference) < percentComplete)
						incrementTowardPosition(movement);
				}
			}
		}
	}
}

void setHomePosition(armServo servo, int position)
{
	moveServo(GRIPPER_ROTATE, position);
	setHome(servo);
}

struct armPositionStruct getNewArmPosition(armPositionName name)
{
	struct armPositionStruct returnStruct;
	returnStruct.name = name;
	returnStruct.index = armPositions.length;
	returnStruct.shoulderRotate = 0;
	returnStruct.shoulder = 0;
	returnStruct.elbow = 0;
	returnStruct.wrist = 0;
	returnStruct.gripperRotate = 0;
	returnStruct.gripper = 0;
	return returnStruct;
}

void populateArmPositions()
{
	armPositions.length = 0;
	
	struct armPositionStruct init = getNewArmPosition(INIT);
	init.shoulderRotate = getServoPos(SHOULDER_ROTATE);
	init.shoulder = getServoPos(SHOULDER);
	init.elbow = getServoPos(ELBOW);
	init.wrist = getServoPos(WRIST);
	init.gripperRotate = getServoPos(GRIPPER_ROTATE);
	init.gripper = getServoPos(GRIPPER);
	armPositions.positions[armPositions.length++] = init;
	
	currentArmPosition = init;

	struct armPositionStruct home = getNewArmPosition(HOME);
	home.shoulder = HOME_SHOULDER;
	home.elbow = HOME_ELBOW;
	home.wrist = HOME_WRIST;
	armPositions.positions[armPositions.length++] = home;
	
	struct armPositionStruct crouch = getNewArmPosition(CROUCH);
	crouch.shoulder = CROUCH_SHOULDER;
	crouch.elbow = CROUCH_ELBOW;
	crouch.wrist = CROUCH_WRIST;
	armPositions.positions[armPositions.length++] = crouch;
	
	struct armPositionStruct sit = getNewArmPosition(SIT);
	sit.shoulder = SIT_SHOULDER;
	sit.elbow = SIT_ELBOW;
	sit.wrist = SIT_WRIST;
	armPositions.positions[armPositions.length++] = sit;
}

void initArm()
{
	populateArmPositions();
	//grip(GRIPPER_OPEN_GRAB);
	//powerDownArm();
}
