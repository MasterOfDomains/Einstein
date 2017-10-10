#ifndef UTILS_H
#define UTILS_H

typedef enum
{
	LEFT,
	RIGHT,
	CENTER // i.e. neither Side
} side;

typedef enum
{
	FORWARD,
	BACKWARD,
	MIDDLE
} direction;

typedef enum
{
	VERTICAL,
	HORIZONTAL
} orientation;

// Flip direction or side
direction oppositeDir(direction dir);
side oppositeSide(side pSide);

#endif