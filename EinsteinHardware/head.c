#include "head.h"

#include <avr/io.h>
#include <util/delay.h>

#include "../a2d.h"

#include "hwutils.h"

#define INFRARED PA0

#define HEADLIGHTS_PORT PORTB
#define HEADLIGHTS_ENABLE PB6
#define SONAR_ENABLE_PORT PORTB
#define SONAR_ENABLE PB7
#define LEFT_SONAR PA6
#define RIGHT_SONAR PA7
#define SONAR_PORT PINA
#define SONAR_ACTIVATION_PULSE_TIME 100
#define SONAR_CYCLES_PER_INCH 41
#define MAX_SONAR_CYCLES 4140

void headLights(BOOL on)
{
    if (on) {
        PORT_ON(HEADLIGHTS_PORT, HEADLIGHTS_ENABLE);
    } else {
        PORT_OFF(HEADLIGHTS_PORT, HEADLIGHTS_ENABLE);
    }
    _delay_ms(1);
}

float getSonarDistance(side sonarSide)
{
    s08 bit = -1;
    if (sonarSide == LEFT) {
        bit = LEFT_SONAR;
    } else if (sonarSide == RIGHT) {
        bit = RIGHT_SONAR;
    } else {
        signalFatalError(INVALID_SENSOR);
    }
    u32 sonarCycles = 0;
    while (PORT_IS_OFF(SONAR_PORT, bit)); // Wait for pulse
    while (PORT_IS_ON(SONAR_PORT, bit)) {
        sonarCycles += 1;
        if (sonarCycles == MAX_SONAR_CYCLES) {
            break;
        }
    }
    return (float)sonarCycles/SONAR_CYCLES_PER_INCH;
}

struct headSonarReadings readHeadSonars()
{
    struct headSonarReadings readings;
    PORT_ON(SONAR_ENABLE_PORT, SONAR_ENABLE);
    _delay_us(SONAR_ACTIVATION_PULSE_TIME);
    PORT_OFF(SONAR_ENABLE_PORT, SONAR_ENABLE);
    readings.left = getSonarDistance(LEFT);
    readings.right= getSonarDistance(RIGHT);
    return readings;
}

u08 getIR()
{
    return a2dConvert8bit(INFRARED);
}

void initHead()
{
    rprintfProgStrM("Initializing Head");
    rprintfCRLF();
    PORT_OFF(SONAR_ENABLE_PORT, SONAR_ENABLE); // Disable sonars
    PORT_OFF(HEADLIGHTS_PORT, HEADLIGHTS_ENABLE); // Turn off headlights
    initVision();
    rprintfProgStrM("Head Initialized");
    rprintfCRLF();
}