#ifndef HEAD_H
#define HEAD_H

#include "../utils.h"

#include "hwglobal.h"

struct headSonarReadings
{
	u08 left;
	u08 right;
};

void initHead();
struct headSonarReadings readHeadSonars();
u08 getIR();

#endif