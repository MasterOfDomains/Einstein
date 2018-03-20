#ifndef HEAD_H
#define HEAD_H

#include "../utils.h"

#include "hwglobal.h"
#include "vision.h"

struct headSonarReadings
{
	u08 left;
	u08 right;
};

void initHead();
struct headSonarReadings readHeadSonars();
void headLights(BOOL on);
u08 getIR();

#endif