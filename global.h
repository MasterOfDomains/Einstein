#ifndef EINSTEIN_H
#define EINSTEIN_H

// global defines
#include "einsteindefs.h"
// global types definitions
#include "einsteintypes.h"

// project/system dependent defines

// CPU clock speed
//#define F_CPU        16000000               		// 16MHz processor
//#define F_CPU        14745000               		// 14.745MHz processor
//#define F_CPU        14745600
//#define F_CPU        8000000               		// 8MHz processor
//#define F_CPU        7372800               		// 7.37MHz processor
//#define F_CPU        4000000               		// 4MHz processor
//#define F_CPU        16000000               		// 16MHz processor
#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond
//#define RPRINTF_COMPLEX

#endif