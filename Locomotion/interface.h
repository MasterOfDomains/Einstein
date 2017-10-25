#ifndef INTERFACE_H
#define INTERFACE_H

#include "../global.h"
#include "../utils.h"

#include "lutils.h"

struct commandStruct
{
	locomotionCommandName name;
	direction commandDir;
	side commandSide;
	float distance;
	u08 speed;
};

void initInterface(void);
struct commandStruct waitForCommand(void);

#endif
