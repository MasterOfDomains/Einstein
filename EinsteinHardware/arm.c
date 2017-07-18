#include "arm.h"

/*------- Servo Positions And Directions of Movement --------------------------------
'
'     Servo             Number     Home       Down             Up
'
'     SHOULDER_ROTATE   1          167        Clockwise        Counter-Clockwise
'     SHOULDER_EXTEND   2          156        Forward          Backward
'     ELBOW             3          125        Forward          Backward
'     WRIST_EXTEND      4          147        Backward         Forward
'     WRIST_ROTATE      5          145        Clockwise        Counter-Clockwise
'     GRIPPER           6          125        Close            Open
'
'                     /   \
'                     \   /
'                      (o) Wrist Extend
'                      | |
'                      | |
'                      (o) Elbow
'                      | |
'                      | |
'                      (o) Shoulder Extend
'                    -------
'                    |     |
'
'              <-- Front Of Robot: Forward
'
-----------------------------------------------------------------------------------*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

//#include "rprintf.h"

#define ARM_ON_PORT PORTC
#define ARM_ON PC7
#define SERVO_DELAY 20 // ms

struct armPositionStruct
{
	u08 shoulderRotate;
	u08 shoulder;
	u08 elbow;
	u08 wrist;
	u08 wristRotate;
	u08 gripper;
};

//struct armBendR

struct armPositionStruct armInitPositions; // At power-up
struct armPositionStruct armHomePositions; // Default or not in use
struct armPositionStruct armPositions; // Current

struct armServoMovement
{
	// Copied
	armServo servo;
	u08 initPos;
	u08 startPos;
	s16 relation;
	// Calculated
	u08 destPos;
	s16 difference;
	u08 currPos;
};

struct armBendRelations armBends[23];

void moveArmServo(armServo servo, u08 dest)
{
	u08 startPos, currPos;
	s08 inc = 1;
	switch (servo)
	{
		case SHOULDER:
			startPos = armPositions.shoulder;
			break;
		case ELBOW:
			startPos = armPositions.elbow;
			break;
		case WRIST:
			startPos = armPositions.wrist;
			break;
		default:
			rprintf("Servo not supported");
	}
	rprintf("startPos=%d, dest=%d\n\r", startPos, dest);
	currPos = startPos;
	if (startPos > dest)
		inc *= -1;
	rprintf("inc=");
	rprintfNum(10, 3, TRUE, ' ', inc);
	rprintfCRLF();
	while (dest != currPos)
	{
		currPos = (s16)currPos + inc;
		moveServo(servo, currPos);			
		_delay_ms(SERVO_DELAY);
	}
}

void raiseArm(void)
{
	moveArmServo(SHOULDER, armHomePositions.shoulder);
	moveArmServo(ELBOW, armHomePositions.elbow);
	moveArmServo(WRIST, armHomePositions.wrist);
}

armRotatePosition getArmRotateSide(side rotateSide)
{
	armRotatePosition pos;
	if (rotateSide == LEFT)
		pos = SIDE_LEFT;
	else //(rotateSide == RIGHT)
		pos = SIDE_RIGHT;
	return pos;
}

struct armBendRelations armPos(armBendRelationsName pos)
{
	struct armBendRelations returnStruct;
	if (pos >= (sizeof(armBends)/sizeof(struct armBendRelations)))
		signalFatalError(INVALID_ARM_POSITION);
	else
		returnStruct = armBends[pos];
	return returnStruct;
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
	rotateArmToPos(ROTATE_HOME, TRUE);
	grip(GRIPPER_HOME);
	bendArmToPos(ARM_BEND_INIT);
}

void closeGripper(void)
{
	moveServo(GRIPPER, 79);
	_delay_ms(500);
}

void openGripper(void)
{
	moveServo(GRIPPER, armHomePositions.gripper);
	_delay_ms(500);
}

void grip(gripperPosition pos)
{
	moveServo(GRIPPER, pos);
	_delay_ms(500);
}

BOOL rotateArm(side rotateSide)
{
	BOOL withinRange = TRUE;
	u08 destPos;
	if (rotateSide == LEFT)
	{
		destPos = armPositions.shoulderRotate + 1;
		if (destPos > SIDE_LEFT + OVER_ROTATE_ALLOWANCE)
			withinRange = FALSE;
		else
			armPositions.shoulderRotate = destPos;
	}
	else if (rotateSide == RIGHT)
	{
		destPos = armPositions.shoulderRotate - 1;
		if (destPos < SIDE_RIGHT - OVER_ROTATE_ALLOWANCE)
			withinRange = FALSE;
		else
			armPositions.shoulderRotate = destPos;
	}
	if (withinRange)
	{
		rprintf(" Rotate %d ", armPositions.shoulderRotate);
		rprintfCRLF();
		moveServo(SHOULDER_ROTATE, armPositions.shoulderRotate);
		_delay_ms(SERVO_DELAY);
	}
	return withinRange;
}

void rotateArmToPos(armRotatePosition destPos, BOOL clear)
{
	if (clear)
		bendArmToPos(ARM_BEND_ROTATE);
	side moveSide;
	if (destPos < armPositions.shoulderRotate)
		moveSide = RIGHT;
	else
		moveSide = LEFT;
	while (armPositions.shoulderRotate != destPos)
	{
		if (moveSide == LEFT)
			armPositions.shoulderRotate += 1;
		else if (moveSide == RIGHT)
			armPositions.shoulderRotate -= 1;
		moveServo(SHOULDER_ROTATE, armPositions.shoulderRotate);
		_delay_ms(SERVO_DELAY);
	}
}

void bendArmToPos(armBendRelationsName posName)
{
	struct armBendRelations pos = armPos(posName);
	struct armServoMovement shoulderMovement, elbowMovement;
	shoulderMovement.servo = SHOULDER;
	shoulderMovement.initPos = armInitPositions.shoulder;
	shoulderMovement.startPos = armPositions.shoulder;
	elbowMovement.servo = ELBOW;
	elbowMovement.initPos = armInitPositions.elbow;
	elbowMovement.startPos = armPositions.elbow;

	shoulderMovement.relation = pos.shoulder;
	elbowMovement.relation = pos.elbow;

	shoulderMovement.destPos = (s16)shoulderMovement.initPos + shoulderMovement.relation;
	elbowMovement.destPos = (s16)elbowMovement.initPos + elbowMovement.relation;
	shoulderMovement.difference = (s16)shoulderMovement.startPos - 
		(s16)shoulderMovement.destPos;
	elbowMovement.difference = (s16)elbowMovement.startPos - (s16)elbowMovement.destPos;
	u08 *largestMovementPos, *smallestMovementPos;
	struct armServoMovement largestMovement, smallestMovement;
	if (abs(shoulderMovement.difference) > abs(elbowMovement.difference))
	{
		largestMovement = shoulderMovement;
		smallestMovement = elbowMovement;
		largestMovementPos = &armPositions.shoulder;
		smallestMovementPos = &armPositions.elbow;
	}
	else
	{
		largestMovement = elbowMovement;
		smallestMovement = shoulderMovement;
		largestMovementPos = &armPositions.elbow;
		smallestMovementPos = &armPositions.shoulder;
	}

	u08 largestMoveInc = abs(largestMovement.difference)/abs(smallestMovement.difference);
	largestMovement.currPos = largestMovement.startPos;
	smallestMovement.currPos = smallestMovement.startPos;
	while (smallestMovement.currPos != smallestMovement.destPos)
	{
		if (smallestMovement.currPos < smallestMovement.destPos)
			smallestMovement.currPos += 1;
		else
			smallestMovement.currPos -= 1;
		*smallestMovementPos = smallestMovement.currPos;
		moveServo(smallestMovement.servo, smallestMovement.currPos);
		for (u08 largeTicks = 0; largeTicks < largestMoveInc; largeTicks++)
		{
			if (largestMovement.currPos != largestMovement.destPos)
			{
				if (largestMovement.currPos < largestMovement.destPos)
					largestMovement.currPos += 1;
				else
					largestMovement.currPos -= 1;
				*largestMovementPos = largestMovement.currPos;
				moveServo(largestMovement.servo, largestMovement.currPos);
				_delay_ms(SERVO_DELAY);
			}
			else
				break;
		}
	}
	while (largestMovement.currPos != largestMovement.destPos)
	{
		if (largestMovement.currPos < largestMovement.destPos)
			largestMovement.currPos += 1;
		else
			largestMovement.currPos -= 1;
		*largestMovementPos = largestMovement.currPos;
		moveServo(largestMovement.servo, largestMovement.currPos);
		_delay_ms(SERVO_DELAY);
	}
}

void initArm(void)
{
	// "Initializing Arm\n\r"
	
	//static const int mystring[] PROGMEM = "Hello";
	//rprintfProgStrM(mystring); // wide character array initialized from non-wide string
	
	// #define rprintfProgStrM(string)			(rprintfProgStr(PSTR(string)))
	
	//rprintfProgStrM("Hello"); // incompatible pointer type [-Wincompatible-pointer-types]
	
	//const char error_code1[7] PROGMEM = {"error1"};
	//rprintfProgStrM(error_code1);
	
	//rprintfProgStr(PSTR("")); // incompatible pointer type [-Wincompatible-pointer-types]
	
	//char *mycharPtr = 'A';
	//rprintfProgStr(mycharPtr); // incompatible pointer type [-Wincompatible-pointer-types]

	//rprintfProgStr('A'); // makes pointer from integer without a cast [-Wint-conversion]
	
	//char *mychar;
	//rprintfProgStr(&mychar); // incompatible pointer type [-Wincompatible-pointer-types]
	//rprintfProgStrM(mychar); // incompatible pointer type [-Wincompatible-pointer-types]
	//rprintfProgStrM(*mychar); // makes pointer from integer without a cast [-Wint-conversion]
	
	//char mychar;
	//rprintfProgStr(&mychar); // incompatible pointer type [-Wincompatible-pointer-types]
	//rprintfProgStrM(mychar); // makes pointer from integer without a cast [-Wint-conversion]
	//rprintfProgStrM(*mychar); // invalid type argument of unary '*' (have 'int')

	
	//rprintfProgStr(1); // makes pointer from integer without a cast [-Wint-conversion]

	//rprintf("Hello");

	/*
	struct armBendRelations armBendFloorGrab1 = { -151, -92 };
	struct armBendRelations armBendFloorGrab2 = { -152, -97 };
	struct armBendRelations armBendFloorGrab3 = { -154, -104 };
	struct armBendRelations armBendFloorGrab4 = { -156, -112 };
	struct armBendRelations armBendFloorGrab5 = { -159, -120 };
	struct armBendRelations armBendCargoFeel1 = { -143, -74 };
	struct armBendRelations armBendCargoFeel2 = { -144, -78 };
	struct armBendRelations armBendCargoFeel3 = { -146, -84 };
	struct armBendRelations armBendCargoFeel4 = { -149, -93 };
	struct armBendRelations armBendCargoFeel5 = { -152, -102 };
	struct armBendRelations armBendInit = { 0, 0 };
	struct armBendRelations armBendRotate = { -60, -68 };
	struct armBendRelations armBendFloorLook = { -50, 2 };
	struct armBendRelations armBendFloorDropA = { -113, -74 };
	struct armBendRelations armBendFloorDropB = { -159, -120 };
	struct armBendRelations armBendFloorDrop = { -156, -112 };
	struct armBendRelations armBendBinDrop = { -39, -14 };
	struct armBendRelations armBendBinClear = { -50, -5 };
	struct armBendRelations armBendBinGrab = { -54, -5 };
	struct armBendRelations armBendBinMeasure_Rail = { -25, -12 };
	struct armBendRelations armBendBinMeasure_Sea = { -29, 0 };
	struct armBendRelations armBendBinMeasure_Air = { -42, 0 };
	struct armBendRelations armBendPlatformLook = { -130, -81 };
	armBends[ARM_BEND_FLOOR_GRAB_1] = armBendFloorGrab1;
	armBends[ARM_BEND_FLOOR_GRAB_2] = armBendFloorGrab2;
	armBends[ARM_BEND_FLOOR_GRAB_3] = armBendFloorGrab3;
	armBends[ARM_BEND_FLOOR_GRAB_4] = armBendFloorGrab4;
	armBends[ARM_BEND_FLOOR_GRAB_5] = armBendFloorGrab5;
	armBends[ARM_BEND_CARGO_FEEL_1] = armBendCargoFeel1;
	armBends[ARM_BEND_CARGO_FEEL_2] = armBendCargoFeel2;
	armBends[ARM_BEND_CARGO_FEEL_3] = armBendCargoFeel3;
	armBends[ARM_BEND_CARGO_FEEL_4] = armBendCargoFeel4;
	armBends[ARM_BEND_CARGO_FEEL_5] = armBendCargoFeel5;
	armBends[ARM_BEND_INIT] = armBendInit;
	armBends[ARM_BEND_ROTATE] = armBendRotate;
	armBends[ARM_BEND_FLOOR_LOOK] = armBendFloorLook;
	armBends[ARM_BEND_FLOOR_DROP_A] = armBendFloorDropA;
	armBends[ARM_BEND_FLOOR_DROP_B] = armBendFloorDropB;
	armBends[ARM_BEND_FLOOR_DROP] = armBendFloorDrop;
	armBends[ARM_BEND_BIN_DROP] = armBendBinDrop;
	armBends[ARM_BEND_BIN_CLEAR] = armBendBinClear;
	armBends[ARM_BEND_BIN_GRAB] = armBendBinGrab;
	armBends[ARM_BEND_BIN_MEASURE_RAIL] = armBendBinMeasure_Rail;
	armBends[ARM_BEND_BIN_MEASURE_SEA] = armBendBinMeasure_Sea;
	armBends[ARM_BEND_BIN_MEASURE_AIR] = armBendBinMeasure_Air;
	armBends[ARM_BEND_PLATFORM_LOOK] = armBendPlatformLook;
	*/

	armInitPositions.shoulderRotate = getServoPos(SHOULDER_ROTATE);
	armInitPositions.shoulder = getServoPos(SHOULDER);
	armInitPositions.elbow = getServoPos(ELBOW);
	armInitPositions.wrist = getServoPos(WRIST);
	armInitPositions.wristRotate = getServoPos(WRIST_ROTATE);
	armInitPositions.gripper = getServoPos(GRIPPER);

	armPositions.shoulderRotate = armInitPositions.shoulderRotate;
	armPositions.shoulder = armInitPositions.shoulder;
	armPositions.elbow = armInitPositions.elbow;
	armPositions.wrist = armInitPositions.wrist;
	armPositions.wristRotate = armInitPositions.wristRotate;
	armPositions.gripper = armInitPositions.gripper;
	
	armHomePositions.shoulderRotate = armInitPositions.shoulderRotate;
	armHomePositions.shoulder = 122;
	armHomePositions.elbow = 90;
	armHomePositions.wrist = 68;
	armHomePositions.wristRotate = armInitPositions.wristRotate;
	armHomePositions.gripper = armPositions.gripper;
	
	//grip(GRIPPER_OPEN_GRAB);
	//powerDownArm();
}

void displayArmPositions(void)
{
	//rprintf("Rotate: %d\n\r", armPositions.rotate);
	rprintf("Shoulder: %d\n\r", armPositions.shoulder);
	rprintf("Elbow: %d\n\r", armPositions.elbow);
	//rprintf("Gripper: %d\n\r", armPositions.gripper);
}

