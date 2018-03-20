#ifndef VISION_H_
#define VISION_H_

#include "avrcam.h"

// Adjusts for White Balance and Autogain
void initVision(void);

void setMinBlobHeight(u16 height);

blob *getBestBlob(color *blobColor);

u08 getTallestBlob(struct blobArray blobs, color blobColor);
BOOL samePoint(struct point point1, struct point point2);

// Returns True if blob object is centered on screen in supplied
// orientation (horizontal or vertical) and within supplied error
// margin
BOOL centered(blob *object, orientation dir, u08 error);

BOOL blobsTouching(blob *blob1, blob *blob2);

//void getBlobArray(struct blobArray *returnArray, color blobColor, u08 maxHeight, u08 maxWidth);

// Makes a newly declared blob "blank"
void emptyBlob(blob *blobStruct);

u08 getBlobHeight(blob *pBlob);
u08 getBlobWidth(blob *pBlob);
struct point getBlobMiddle(blob *pBlob);

// Displays blob array graphically
void displayBlobArray(struct blobArray *blobs);

// Displays blob info
void displayBlob(blob *trackedBlob);

void displayPoint(struct point *p);

#endif