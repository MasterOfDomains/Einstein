#include "utils.h"

direction oppositeDir(direction dir)
{
    direction returnVal;
    if (dir == FORWARD) {
        returnVal = BACKWARD;
    } else {
        returnVal = FORWARD;
    }
    return returnVal;
}

side oppositeSide(side pSide)
{
    side returnVal;
    if (pSide == LEFT) {
        returnVal = RIGHT;
    } else {
        returnVal = LEFT;
    }
    return returnVal;
}
