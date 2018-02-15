#include "arm.h"

/*------- Servo Positions And Directions of Movement --------------------------------

Servo             Number	Init	Home	Down		Up

SHOULDER_ROTATE   1			128		128		Clockwise	Counter-Clockwise
SHOULDER_EXTEND   2			85		122		Forward		Backward
ELBOW             3			221		90		Forward		Backward
WRIST_EXTEND      4			39		68		Backward	Forward
WRIST_ROTATE      5			124		145		Clockwise	Counter-Clockwise
GRIPPER           6			127		127		Close		Open

                     /   \
                     \   /
                      (o) Wrist Extend
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
//#include "rprintf.h"

#define ARM_ON_PORT PORTC
#define ARM_ON PC7
#define SERVO_DELAY_MS 20

#define NUM_SERVOS 6
#define MAX_SHOULDER_BEND -159 // Most extended
#define MIN_SHOULDER_BEND -50 // Most retracted
#define MAX_ELBOW_BEND -120 // Most extended
#define MIN_ELBOW_BEND 2 // Most folded
#define MAX_ROTATE_LEFT 238
#define MAX_ROTATE_RIGHT 12
#define GRIPPER_VERTICAL_DIST 100
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
	u08 wristRotate;
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

void printBadArmPosition(u08 servo, u08 pos, u08 actualPos)
{
	rprintf("Bad Servo Position [%d]. Mem: %d, Actual: %d", servo, pos, actualPos);
	rprintfCRLF();
}

BOOL checkArmPositions() {
	BOOL positionsCorrect = TRUE;
	u08 shoulderRotatePos = *getCurrentArmServoPosition(SHOULDER_ROTATE);
	u08 shoulderPos = *getCurrentArmServoPosition(SHOULDER);
	u08 elbowPos = *getCurrentArmServoPosition(ELBOW);
	u08 wristPos = *getCurrentArmServoPosition(WRIST);
	u08 wristRotatePos = *getCurrentArmServoPosition(WRIST_ROTATE);
	u08 gripperPos = *getCurrentArmServoPosition(GRIPPER);
		
	u08 actualShoulderRotatePos = getServoPos(SHOULDER_ROTATE);
	u08 actualShoulderPos = getServoPos(SHOULDER);
	u08 actualElbowPos = getServoPos(ELBOW);
	u08 actualWristPos = getServoPos(WRIST);
	u08 actualWristRotatePos = getServoPos(WRIST_ROTATE);
	u08 actualGripperPos = getServoPos(GRIPPER);
	
	if (shoulderRotatePos != actualShoulderRotatePos) {
		printBadArmPosition(SHOULDER_ROTATE, shoulderRotatePos, actualShoulderRotatePos);
		positionsCorrect = FALSE;
	}
	if (shoulderPos != actualShoulderPos) {
		printBadArmPosition(SHOULDER, shoulderPos, actualShoulderPos);
		positionsCorrect = FALSE;
	}
	if (elbowPos != actualElbowPos) {
		printBadArmPosition(ELBOW, elbowPos, actualElbowPos);
		positionsCorrect = FALSE;
	}
	if (wristPos != actualWristPos) {
		printBadArmPosition(WRIST, wristPos, actualWristPos);
		positionsCorrect = FALSE;
	}
	if (wristRotatePos != actualWristRotatePos) {
		printBadArmPosition(WRIST_ROTATE, wristRotatePos, actualWristRotatePos);
		positionsCorrect = FALSE;
	}
	if (gripperPos != actualGripperPos) {
		printBadArmPosition(GRIPPER, gripperPos, actualGripperPos);
		positionsCorrect = FALSE;
	}

	return positionsCorrect;
}

struct armPositionStruct getArmPosition(armPositionName name) {
	struct armPositionStruct position;
	
	for (u08 i = 0; i < armPositions.length; i++) {
		if (armPositions.positions[i].name == name) {
			position = armPositions.positions[name];
			break;
		}
	}
	return position;
}

u08 *getCurrentArmServoPosition(armServo servo) {
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
		case WRIST_ROTATE:
			returnVal = &currentArmPosition.wristRotate;
			break;
		case WRIST:
			returnVal = &currentArmPosition.wrist;
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
	grip(GRIPPER_HOME);
	moveArmToPos(HOME);
}

void closeGripper(void)
{
	moveArmServo(GRIPPER, 79);
	_delay_ms(500);
}

void openGripper(void)
{
	moveArmServo(GRIPPER, GRIPPER_OPEN_GRAB);
	_delay_ms(500);
}

void grip(gripperPosition pos)
{
	moveArmServo(GRIPPER, pos);
	_delay_ms(500);
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
	if (withinRange && destPos)
	{
		rprintf(" Rotate to %d ", destPos);
		rprintfCRLF();
		moveArmServo(SHOULDER_ROTATE, destPos);
	}
	return withinRange;
}

void rotateWrist(wirstRotationPositionName positionName)
{
	u08 homePosition = getArmPosition(HOME).wristRotate;
	u08 position = 0;
	switch (positionName) {
		case GRIPPER_LEVEL:
			position = homePosition;
			break;
		case GRIPPER_VERTIAL_RIGHT:
			position = homePosition - GRIPPER_VERTICAL_DIST;
			break;
		case GRIPPER_VERTICAL_LEFT:
			position = homePosition + GRIPPER_VERTICAL_DIST;
			break;
	}
	rprintf(" W-Rotate to %d ", position);
	rprintfCRLF();
	moveArmServo(SHOULDER_ROTATE, position);
}


void moveArmServo(armServo servo, u08 dest)
{
	if (dest != 0) {
		s08 inc = 1;
		u08 *position = getCurrentArmServoPosition(servo);
		rprintf("position=%d, dest=%d - ", *position, dest);
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

struct armServoMovement getArmServoMovement(armServo servo, u08 destPos) {
	//getArmServoMovement(GRIPPER, getArmPosition(posName).gripper);
	struct armServoMovement movement;
	movement.servo = servo;
	movement.currPos = getCurrentArmServoPosition(servo);
	movement.startPos = *movement.currPos;
	movement.destPos = destPos;
	if (destPos != 0)
		movement.difference = (s16)movement.destPos - (s16)movement.startPos;
	else
		movement.difference = 0;
	printMovement(movement);
	return movement;
}

void incrementTowardPosition(armServoMovement movement)
{
	s08 inc = 1;
	rprintfProgStrM("-> ");
	if (movement.destPos < *movement.currPos) {
		inc = -1;
	}
	moveArmServo(movement.servo, *movement.currPos + inc);
}

void printMovement(armServoMovement movement) {
	rprintfCRLF();
	rprintf("Servo %d Movement\n\r", movement.servo);
	rprintf("startPos: %d\n\r", movement.startPos);
	rprintf("destPos: %d\n\r", movement.destPos);
	rprintf("difference: %d\n\r", movement.difference);
	rprintf("currPos: %d\n\r", *movement.currPos);
	rprintfCRLF();
}

void moveArmToPos(armPositionName posName)
{
	armServoMovement movements[NUM_SERVOS];
	movements[SHOULDER_ROTATE - 1] = getArmServoMovement(SHOULDER_ROTATE, getArmPosition(posName).shoulderRotate);
	movements[SHOULDER - 1] = getArmServoMovement(SHOULDER, getArmPosition(posName).shoulder);
	movements[ELBOW - 1] = getArmServoMovement(ELBOW, getArmPosition(posName).elbow);
	movements[WRIST - 1] = getArmServoMovement(WRIST, getArmPosition(posName).wrist);
	movements[WRIST_ROTATE - 1] = getArmServoMovement(WRIST_ROTATE, getArmPosition(posName).wristRotate);
	movements[GRIPPER - 1] = getArmServoMovement(GRIPPER, getArmPosition(posName).gripper);
	s16 largestMovement = 0;
	for (u08 servo = 0; servo < NUM_SERVOS; servo++) {
		if (movements[servo].difference != 0 && abs(movements[servo].difference) > abs(largestMovement))
			largestMovement = movements[servo].difference;
	}
	rprintf("largestMovement %d\n\r", largestMovement);
	u08 percentComplete = 0;
	for (u08 moveIncrement = 0; moveIncrement < abs(largestMovement); moveIncrement++) {
		rprintfCRLF();
		rprintf("Begin Increment %d\n\r", moveIncrement);
		for (u08 servo = 0; servo < NUM_SERVOS; servo++) {
			armServoMovement movement = movements[servo];
			if (movement.difference != 0) {
				u08 amountMoved = abs(movement.startPos - *movement.currPos);
				if (movement.difference == largestMovement) {
					rprintfProgStrM("* ");
					rprintf("Servo %d - ", movements[servo].servo);
					incrementTowardPosition(movement);
					percentComplete = ((u16)amountMoved * 100) / abs(movement.difference);
					rprintf("Amt Moved: %d, Pct Complete: %d\n\r", amountMoved, percentComplete);
				} else {
					if ((((u16)amountMoved * 100) / movement.difference) < percentComplete) {
						rprintf("Servo %d - ", movements[servo].servo);
						rprintfProgStrM(". ");
						incrementTowardPosition(movement);
						rprintfCRLF();
					}
				}
			}
		}
		rprintf("End Increment %d", moveIncrement);
		rprintfCRLF();
	}
}

void setHomePosition(armServo servo, int position) {
	moveServo(WRIST_ROTATE, position);
	setHome(servo);
}

struct armPositionStruct getNewArmPosition(armPositionName name) {
	struct armPositionStruct returnStruct;
	returnStruct.name = name;
	returnStruct.index = armPositions.length;
	returnStruct.shoulderRotate = 0;
	returnStruct.shoulder = 0;
	returnStruct.elbow = 0;
	returnStruct.wristRotate = 0;
	returnStruct.wrist = 0;
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
	init.wristRotate = getServoPos(WRIST_ROTATE);
	init.gripper = getServoPos(GRIPPER);
	armPositions.positions[armPositions.length++] = init;
	
	currentArmPosition = init;

	struct armPositionStruct home = getNewArmPosition(HOME);
	home.shoulder = 122;
	home.elbow = 90;
	home.wrist = 145;
	armPositions.positions[armPositions.length++] = home;
	
	struct armPositionStruct crouch = getNewArmPosition(CROUCH);
	crouch.shoulder = 130;
	crouch.elbow = 80;
	crouch.wrist = 58;
	armPositions.positions[armPositions.length++] = crouch;
}

void initArm()
{
	populateArmPositions();
	//grip(GRIPPER_OPEN_GRAB);
	//powerDownArm();
}

void displayArmPositions(struct armPositionStruct position)
{
	switch (position.name) {
		case CROUCH:
			rprintfProgStrM("Crouch ");
			break;
		case HOME:
			rprintfProgStrM("Home ");
			break;
		case INIT:
			rprintfProgStrM("Init ");
			break;
		case LEAN:
			rprintfProgStrM("Lean ");
			break;
		case SIT:
			rprintfProgStrM("Sit ");
			break;
		default:
			rprintfProgStrM("Not Found ");

	}
	rprintfNum(10, 2, FALSE, ' ', position.index);
	rprintfCRLF();
	rprintf("S-Rotate: %d\n\r", position.shoulderRotate);
	rprintf("Shoulder: %d\n\r", position.shoulder);
	rprintf("Elbow: %d\n\r", position.elbow);
	rprintf("W-Rotate: %d\n\r", position.wristRotate);
	rprintf("Wrist: %d\n\r", position.wrist);
	rprintf("Gripper: %d\n\r", position.gripper);
}