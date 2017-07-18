#ifndef INTERFACE_H
#define INTERFACE_H

#include "../global.h"

#include "utils.h"

typedef enum
{
	GO,
	MOVE,
	SPIN,
	TWIST,
	HARD_STOP,
	SOFT_STOP
} commandName;

struct commandStruct
{
	commandName name;
	direction commandDir;
	side commandSide;
	float distance; // inches?
	u08 speed;
};

void initInterface(void);
struct commandStruct waitForCommand(void);

#endif
