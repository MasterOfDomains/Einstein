#ifndef COMPASS_H
#define COMPASS_H

#include "hwutils.h"

#define MAX_HEADING 359

// Used to determine if robot has yet turned to desired heading
BOOL headingReached(u16 startHeading, u16 currHeading, u16 destHeading, side dir);

// Used for 'manual' calibrated - robot must be spun by external
// force
void calibrateCompass(void);

// Displays reading in infinite loop
void testCompass(void);

// Returns current Heading in degrees
u16 readCompass(void);

// Populates startHeading variable
void initCompass(void);

// Calculates the new heading based on desired turn amount
u16 calcHeading(s16 startHeading, side robotSide, s16 amount);

// Returns heading from initial starting position
u16 getHomeHeading(void);

// Returns heading from initial starting position
u16 setHomeHeading(void);

// Returns the side fewest degrees away from a heading or CENTER
// if dead-on
side getHeadingSide(u16 heading);

// Returns the distance to a heading moving toward a given side
u16 getHeadingDist(u16 heading, side headingSide);

#endif
