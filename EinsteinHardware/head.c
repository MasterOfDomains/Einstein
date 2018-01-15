#include "head.h"

#include <avr/io.h>
#include <util/delay.h>

#include "../a2d.h"

#include "hwutils.h"

#define INFRARED PA0

#define SONAR_ENABLE_PORT PORTB
#define SONAR_ENABLE PB7
#define LEFT_SONAR PA6
#define RIGHT_SONAR PA7
#define SONAR_POWERUP_TIME 100

void clearADCs() {
	a2dConvert8bit(LEFT_SONAR);
	a2dConvert8bit(RIGHT_SONAR);
	a2dConvert8bit(LEFT_SONAR);
	a2dConvert8bit(RIGHT_SONAR);
}

struct headSonarReadings readHeadSonars() {
	clearADCs();
	struct headSonarReadings readings;
	PORT_ON(SONAR_ENABLE_PORT, SONAR_ENABLE);
	
	_delay_us(SONAR_POWERUP_TIME);
	readings.left = a2dConvert8bit(LEFT_SONAR);
	
	PORT_OFF(SONAR_ENABLE_PORT, SONAR_ENABLE);
	
	_delay_us(SONAR_POWERUP_TIME);
	readings.right= a2dConvert8bit(RIGHT_SONAR);

	return readings;
}

u08 getIR() {
	return a2dConvert8bit(INFRARED);
}

void initHead() {
	PORT_OFF(PORTB, PB7); // Disable
}