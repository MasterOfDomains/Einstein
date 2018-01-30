#ifndef _VISION_H_
#define _VISION_H_


#include "avrcam.h"
#include "hwutils.h"

#define MAX_ROW_BLOBS 5 // blobs that be detected in one sweep

typedef enum
{
	R, G, B
} rgb;

struct colorProfile
{
	char name[10];
	struct colorStruct colorInfo[MAX_COLORS];
};

u08 getTallestBlob(struct blobArray blobs, color blobColor);
BOOL samePoint(struct point point1, struct point point2);

// Methods for using vision while bot POV is moving
// moveSide - direction bot is moving
// currPoint - latest known point of target
// targX/targY - if currPoint has pass this point funcion returns TRUE
BOOL pointReachedX(side moveSide, struct point currPoint, u08 targX);
BOOL pointReachedY(direction moveDir, struct point currPoint, u08 targY);

// Adjusts for White Balance and Autogain
void initVision(void);

// Vision Profiles
void colorProfileMenu(void);

void testVision(void);
void getTestBitmap(void);

// For tweaking color structures
void adjustColors(void);

// Returns True if blob object is centered on screen in supplied 
// orientation (horizontal or vertical) and within supplied error
// margin
BOOL testCentered(struct blob *object, orientation dir, u08 error);

// Displays entire image over debug UART to test console
//void displayImage(struct bitmapStruct *bitmap);

// Displays blob array graphically
void displayBlobArray(struct blobArray *blobs);

// Displays blob info
void displayBlob(struct blob *trackedBlob);

void displayPoint(struct point *p);

// Returns True if pixel was return as '1'
//BOOL testPixel(struct bitmapStruct *bitmap, u08 row, u08 col);

// Returns True if pixel is valid (within borders of bitmap)
BOOL pixelInBounds(u08 x, u08 y);

BOOL blobsTouching(struct blob *blob1, struct blob *blob2);

//void getBlobArray(struct blobArray *returnArray, color blobColor, u08 maxHeight, u08 maxWidth);

// Makes a newly declared blob "blank"
void emptyBlob(struct blob *blobStruct);

u08 getBlobHeight(struct blob *pBlob);
u08 getBlobWidth(struct blob *pBlob);
struct point getBlobMiddle(struct blob *pBlob);
void adjustColorProfiles(void);

#endif
