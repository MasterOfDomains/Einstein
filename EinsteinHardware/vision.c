#include "vision.h"

#include <stddef.h>
#include <stdlib.h>

#define MIN_BLOB_SIZE 30

BOOL getBlobCluster(color *blobColor, struct blobArray *blobs, blob **returnBlob);
void sortBlobArray(struct blobArray *blobs);
struct blobArray getTestBlobs(void);

void initVision()
{
    initCamera(CAMERA_UART);
}

blob *getBestBlob(color *blobColor)
{
    blob *returnBlob = malloc(sizeof(struct blob));
#ifndef SIMULATOR
    struct blobArray blobs = getBlobs();
#else
    struct blobArray blobs = getTestBlobs();
#endif
    displayBlobArray(&blobs);
    u08 tallestHeight = 0;
    s08 indexBest = -1;
    for (u08 i = 0; i < blobs.length; i++) {
        u08 blobHeight = getBlobHeight(&blobs.contents[i]);
        if (blobHeight >= MIN_BLOB_SIZE && blobHeight > tallestHeight) {
            tallestHeight = blobHeight;
            indexBest = i;
        }
    }
    if (indexBest != -1) {
        returnBlob = &blobs.contents[indexBest];
    } else {
        if (blobs.length > 0) {
            sortBlobArray(&blobs);
            rprintfCRLF();
            rprintfProgStrM("Sorted\n\r");
            displayBlobArray(&blobs);
            rprintfCRLF();

            if (blobColor == ((void *)0)) {
                blobColor = &blobs.contents[0].blobColor;
            }
            getBlobCluster(blobColor, &blobs, &returnBlob);
        }

    }
    return returnBlob;
}

void sortBlobArray(struct blobArray *blobs)
{
    blob blobHolder;
    for (u08 i = 0; i < blobs->length; ++i) {
        for (u08 j = i + 1; j < blobs->length; ++j) { // For each after
            if (getBlobHeight(&blobs->contents[j]) > getBlobHeight(&blobs->contents[i])) {
                blobHolder =  blobs->contents[i];
                blobs->contents[i] = blobs->contents[j];
                blobs->contents[j] = blobHolder;
            }
        }
    }
}

BOOL getBlobCluster(color *blobColor, struct blobArray *sortedBlobArray, blob **returnBlob)
{
    BOOL success = TRUE;
    struct blobArray relevantBlobs; // Largest blob in sorted array may not be desired color
    relevantBlobs.length = 0;
    for (u08 i = 0; i < sortedBlobArray->length; i++) {
        if (sortedBlobArray->contents[i].blobColor == *blobColor) {
            relevantBlobs.contents[relevantBlobs.length++] = sortedBlobArray->contents[i];
        }
    }
    rprintfCRLF();
    rprintfProgStrM("Relevant:\n\r");
    displayBlobArray(&relevantBlobs);
    *returnBlob = &relevantBlobs.contents[0]; // Begins as largest blob
    for (u08 i = 1; i < relevantBlobs.length; i++) { // Loop through the rest by size
        if (relevantBlobs.contents[i].cornerBR.x > (*returnBlob)->cornerBR.x) {
            (*returnBlob)->cornerBR.x = relevantBlobs.contents[i].cornerBR.x;
        }
        if (relevantBlobs.contents[i].cornerBR.y > (*returnBlob)->cornerBR.y) {
            (*returnBlob)->cornerBR.y = relevantBlobs.contents[i].cornerBR.y;
        }
        if (relevantBlobs.contents[i].cornerUL.x < (*returnBlob)->cornerUL.x) {
            (*returnBlob)->cornerUL.x = relevantBlobs.contents[i].cornerUL.x;
        }
        if (relevantBlobs.contents[i].cornerUL.y < (*returnBlob)->cornerUL.y) {
            (*returnBlob)->cornerUL.y = relevantBlobs.contents[i].cornerUL.y;
        }
    }
    if (getBlobWidth(*returnBlob) < MIN_BLOB_SIZE || getBlobHeight(*returnBlob) < MIN_BLOB_SIZE) {
        returnBlob = NULL;
        success = FALSE;
    }
    return success;
}

u08 getTallestBlob(struct blobArray blobs, color blobColor)
{
    u08 tallestIndex = blobs.length;
    u08 tallestHeight = 0;
    for (u08 blobIter = 0; blobIter < blobs.length; blobIter++) {
        u08 currHeight = getBlobHeight(&blobs.contents[blobIter]);
        if (currHeight > tallestHeight) {
            tallestIndex = blobIter;
            tallestHeight = currHeight;
        }
    }
    return tallestIndex;
}

BOOL samePoint(struct point point1, struct point point2)
{
    BOOL same = FALSE;
    if ((point1.x == point2.x) && (point1.y == point2.y)) {
        same = TRUE;
    }
    return same;
}

BOOL centered(blob *object, orientation dir, u08 error)
{
    BOOL returnVal = FALSE;
    u08 middleMass, pictureCenter, pictureCenterLow, pictureCenterHigh;
    struct point middle = getBlobMiddle(object);
    if (dir == HORIZONTAL) {
        middleMass = middle.x;
        pictureCenter = getPictureCenter(HORIZONTAL);
    } else {
        middleMass = middle.y;
        pictureCenter = getPictureCenter(VERTICAL);
    }
    pictureCenterLow = pictureCenter - error;
    pictureCenterHigh = pictureCenter + error;

    if (middleMass >= pictureCenterLow && middleMass <= pictureCenterHigh) {
        returnVal = TRUE;
    }
    return returnVal;
}

BOOL blobsTouching(blob *blob1, blob *blob2)
{
    BOOL returnVal = FALSE;
    // Determine if there is horizontal overlap
    if (((blob1->cornerUL.x >= blob2->cornerUL.x && blob1->cornerUL.x < blob2->cornerBR.x) ||
            (blob1->cornerBR.x <= blob2->cornerBR.x && blob1->cornerBR.x > blob2->cornerUL.x))
            &&
            ((blob1->cornerUL.y >= blob2->cornerUL.y && blob1->cornerUL.y <= blob2->cornerBR.y) ||
             (blob1->cornerBR.y <= blob2->cornerBR.y && blob1->cornerBR.y >= blob2->cornerUL.y))) {
        returnVal = TRUE;
    }
    return returnVal;
}

u08 getBlobHeight(blob *pBlob)
{
    u08 height = pBlob->cornerBR.y - pBlob->cornerUL.y + 1;
    return height;
}

u08 getBlobWidth(blob *pBlob)
{
    u08 width = pBlob->cornerBR.x - pBlob->cornerUL.x + 1;
    return width;
}

struct point getBlobMiddle(blob *pBlob)
{
    struct point blobMiddle;
    u08 width = getBlobWidth(pBlob);
    u08 height = getBlobHeight(pBlob);
    blobMiddle.x = pBlob->cornerUL.x + (width/2);
    blobMiddle.y = pBlob->cornerUL.y + (height/2);
    return blobMiddle;
}

void displayBlobArray(struct blobArray *blobs)
{
    rprintf("Blob Array Length: %d\n\r", blobs->length);
    for (u08 blob = 0; blob < blobs->length; blob++) {
        rprintf("Blob %d:\n\r", blob);
        displayBlob(&blobs->contents[blob]);
    }
}

void displayBlob(blob *trackedBlob)
{
    rprintfProgStrM("Color: ");
    printColorName(trackedBlob->blobColor, FALSE);
    rprintf("  Width: %d  Height: %d\n\r", getBlobWidth(trackedBlob), getBlobHeight(trackedBlob));
    rprintf("X1: %d", trackedBlob->cornerUL.x);
    rprintf("  Y1: %d", trackedBlob->cornerUL.y);
    rprintfCRLF();
    rprintf("X2: %d", trackedBlob->cornerBR.x);
    rprintf("  Y2: %d", trackedBlob->cornerBR.y);
    rprintfCRLF();
}

void displayPoint(struct point *p)
{
    rprintf("Point: X: %d, Y: %d\n\r", p->x, p->y);
    rprintfCRLF();
}

struct blob getTestBlob(color blobColor, u08 X, u08 Y, s08 sizer)
{
    struct blob testBlob;
    testBlob.blobColor = blobColor;
    testBlob.cornerUL = (struct point) {X, Y};
    u08 blobSize = MIN_BLOB_SIZE + sizer;
    testBlob.cornerBR = (struct point) {X + blobSize, Y + blobSize};
    return testBlob;
}

struct blobArray getTestBlobs(void)
{
    struct blobArray blobs;
    blobs.contents[0] = getTestBlob(RED, 20, 40, -5);
    blobs.contents[1] = getTestBlob(GREEN, 80, 30, -10);
    blobs.contents[2] = getTestBlob(BLUE, 100, 10, -15);
    blobs.contents[3] = getTestBlob(ORANGE, 140, 40, -5);
    blobs.contents[4] = getTestBlob(RED, 55, 45, -2);
    blobs.contents[5] = getTestBlob(BLUE, 140, 100, -8);
    blobs.contents[6] = getTestBlob(RED, 30, 100, -20);
    blobs.contents[7] = getTestBlob(YELLOW, 20, 120, -15);
    blobs.length = 8;
    return blobs;
}