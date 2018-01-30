// Notes
// X pixels go from 1 to 176 Right to Left
// center = 93 (176/2 + 7)
// Y pixels go from 0 to 143 (8F)
// A block from 3ft (mid course) is 11 pixels wide

#ifndef CAMERA_H
#define CAMERA_H

#include "hwutils.h"

#define PICTURE_WIDTH 176
#define PICTURE_HEIGHT 143

#define MAX_COLORS 8

typedef enum
{
	ORANGE = 0,
	YELLOW = 1,
	RED = 2,
	BLUE = 3,
	GREEN = 4,
	BROWN = 5,
	WHITE = 6,
	BLACK = 7
} color;

struct colorStruct
{
	color name;
	u08 r_lower;
	u08 r_upper;
	u08 g_lower;
	u08 g_upper;
	u08 b_lower;
	u08 b_upper;
};

#define COLOR_MAP_LENGTH 48
char colorMap[COLOR_MAP_LENGTH];

struct blob
{
	struct point cornerUL;
	struct point cornerBR;
	color blobColor;
};

struct blobArray
{
	struct blob contents[8];
	u08 length;
};

void clearTrackingColors(void);
void addTrackingColor(color colorName);
void createColorMap(void);
BOOL setColorMap(void);
void displayColors(void);
void displayColor(struct colorStruct displayColor);

struct blobArray getBlobs(void);
void enableTracking(void);
void disableTracking(void);
u08 getPictureWidth(void);
u08 getPictureHeight(void);
u08 getPictureCenter(orientation way);
void initCamera(u08 avrUart);
void printColorName(color cName, BOOL crlf);
struct colorStruct getColor(color colorName);
struct colorStruct * getColorPointer(color structColor);
u08 validateColorLimit(u08 limit);
#endif
