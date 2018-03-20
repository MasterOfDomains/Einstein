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

#define MAX_TRACKED_BLOBS 8
#define MAX_COLORS 8

typedef enum
{
	RED = 0,
	ORANGE = 1,
	GREEN = 2,
	BLUE = 3,
	YELLOW = 4,
	WHITE = 5
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

typedef struct blob
{
	struct point cornerUL;
	struct point cornerBR;
	color blobColor;
} blob;

struct blobArray
{
	blob contents[MAX_TRACKED_BLOBS];
	u08 length;
};

struct blobArray getBlobs(void);
void enableTracking(void);
void disableTracking(void);
u08 getPictureWidth(void);
u08 getPictureHeight(void);
u08 getPictureCenter(orientation way);
void initCamera(u08 avrUart);
void printColorName(color cName, BOOL crlf);

#endif
