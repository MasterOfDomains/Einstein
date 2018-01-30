#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <avr/eeprom.h>
#include <string.h>

#include "../rprintf.h"

#include "vision.h"

#define MAX_COLOR_PROFILES 30

// EEPROM MEMORY LOCATIONS
#define CP_COUNT_ADDR 1
#define CURRENT_CP_ADDR 2
#define CP_AVAILABLE_MAP_ADDR 4

color menu_chooseColor(void);
char menu_adjustColor_Main(void);
char menu_adjustColors_Main(void);
rgb menu_chooseRGB(void);
char menu_chooseLimit(void);
char menu_chooseIncrement(void);
void restoreColorDefault(struct colorStruct *alteredColor);
void adjustColor(color trackedColor);

u16 getCPAddress(u08 number);
void displayColorProfile(struct colorProfile *profile);
BOOL getCPAvailability(u08 profileNumber);
void setCPAvailability(u08 profileNumber, BOOL available);
u08 getAvailableProfileNumber(void);
u08 writeNewColorProfileToEEPROM(struct colorProfile *profile);
struct colorProfile readColorProfileFromEEPROM(u08 profileNumber);
void deleteColorProfile(u08 profileNumber);
void copyStringToCPname(char str[], struct colorProfile *profile);
void copyCPfromRAMtoEEPROM(u08 profileNum, char name[]);
void copyCPfromEEPROMtoRAM(u08 profileNum);

u08 getTallestBlob(struct blobArray blobs, color blobColor)
{
	u08 tallestIndex;
	u08 tallestHeight = 0;
	for (u08 blobIter = 0; blobIter < blobs.length; blobIter++)
	{
		u08 currHeight = getBlobHeight(&blobs.contents[blobIter]);
		if (currHeight > tallestHeight)
		{
			tallestIndex = blobIter;
			tallestHeight = currHeight;
		}
	}
	return tallestIndex;
}

void adjustColorProfiles(void)
{
	BOOL adjustmentMade = FALSE;
	for (color colorIter = 0; colorIter < MAX_COLORS; colorIter++)
	{
		rprintfProgStrM("Place ");
		printColorName(colorIter, TRUE);
		waitForButton();
		BOOL acceptable = TRUE;
		while (!acceptable)
		{
			clearTrackingColors();
			addTrackingColor(colorIter);
			createColorMap();
			setColorMap();
			struct blobArray blobs = getBlobs();
			u08 tallestHeight = 0;
			u08 tallestIndex;
			for (u08 blobIter = 0; blobIter < blobs.length; blobIter++)
			{ // Select tallest
				u08 height = getBlobHeight(&blobs.contents[blobIter]);
				if (height > tallestHeight)
					tallestIndex = blobIter;
			}
			u08 tallestWidth = getBlobWidth(&blobs.contents[tallestIndex]);
			rprintfProgStrM("Tallest Blob: ");
			rprintf("H: %d W: %d\n\r", tallestHeight, tallestWidth);
			displayBlob(&blobs.contents[tallestIndex]);
			rprintfProgStrM("Accept?\n\r");
			acceptable = menu_getConfirm();
			if (!acceptable)
			{
				adjustmentMade = TRUE;
				adjustColor(colorIter);
			}
		}
	}
	if (adjustmentMade)
	{
		// Get current profile number
		u08 profileNum = eeprom_read_byte((u08*)CURRENT_CP_ADDR);
		// Get EEPROM stored version of current profile
		struct colorProfile profile;
		u16 address = getCPAddress(profileNum);
		eeprom_read_block((void*)&profile, (const void*)address,
			sizeof(struct colorProfile));
		// Replace colors with those in RAM
		for (color colorIter = 0; colorIter < MAX_COLORS; colorIter++)
		{
			profile.colorInfo[colorIter] = getColor(colorIter);
		}
		eeprom_update_block((const void*)&profile, (void*)address, 
		sizeof(struct colorProfile));

	}
}

void deleteColorProfile(u08 profileNumber)
{
	setCPAvailability(profileNumber, TRUE);
	u08 count = eeprom_read_byte((const u08*)CP_COUNT_ADDR);
	eeprom_write_byte((u08*)CP_COUNT_ADDR, --count);
}

void printCPAvailability(u32 map)
{
	rprintfProgStrM("Map=");
	rprintfNum(10, 11, FALSE, ' ',   map);
	rprintfCRLF();
}

BOOL getCPAvailability(u08 profileNumber)
{
	BOOL available = FALSE;
	u32 map = eeprom_read_dword((const u32*)CP_AVAILABLE_MAP_ADDR);
	u32 mapMask = 1;
	u32 available32 = map & (mapMask << profileNumber);
	if (available32)
		available = TRUE;
	return available;
}

void setCPAvailability(u08 profileNumber, BOOL available)
{
	u32 map = eeprom_read_dword((const u32*)CP_AVAILABLE_MAP_ADDR);
	u32 mapMask = 1;
	mapMask = (mapMask << profileNumber);
	if (available)
		map |= mapMask;
	else
		map  &= ~mapMask;
	eeprom_update_dword((u32*)CP_AVAILABLE_MAP_ADDR, map);
}

u08 getAvailableColorProfileNumber(void)
{
	u08 number = 0;
	for (u08 profileIter = 0; profileIter < MAX_COLOR_PROFILES; profileIter++)
	{
		if (getCPAvailability(profileIter))
		{
			number = profileIter;
			break;
		}
	}
	return number;
}

u16 getCPAddress(u08 number)
{
	u16 returnAddress;
	#define CPBeginAdress 10
	returnAddress = CPBeginAdress + (number * sizeof(struct colorProfile));
	return returnAddress;
}

void copyCPfromEEPROMtoRAM(u08 profileNum)
{
	struct colorProfile profile = readColorProfileFromEEPROM(profileNum);
	for (u08 colorIter = 0; colorIter < MAX_COLORS; colorIter++)
	{ // Load into RAM
		color name = profile.colorInfo[colorIter].name;
		struct colorStruct *currentColorStruct = getColorPointer(name);
		*currentColorStruct = profile.colorInfo[colorIter];
	}
	rprintfProgStrM("Profile In RAM\n\r");
}

void copyCPfromRAMtoEEPROM(u08 profileNum, char name[])
{ // Error Here?
	struct colorProfile profile;
	copyStringToCPname(name, &profile);
	rprintf("New name: ");
	rprintfStr(profile.name);
	rprintfCRLF();
	for (color colorIter = 0; colorIter < MAX_COLORS; colorIter++)
	{
		struct colorStruct *currColor = getColorPointer(colorIter);
		profile.colorInfo[colorIter].name = currColor->name;
		profile.colorInfo[colorIter].r_lower = currColor->r_lower;
		profile.colorInfo[colorIter].r_upper = currColor->r_upper;
		profile.colorInfo[colorIter].g_lower = currColor->g_lower;
		profile.colorInfo[colorIter].g_upper = currColor->g_upper;
		profile.colorInfo[colorIter].b_lower = currColor->b_lower;
		profile.colorInfo[colorIter].b_upper = currColor->b_upper;
	}
	u16 address = getCPAddress(profileNum);
	rprintfProgStrM("Updating EEPROM\n\r");
	displayColorProfile(&profile);
	waitForButton();
	eeprom_update_block((const void*)&profile, (void*)address, 
		sizeof(struct colorProfile));
}

u08 writeNewColorProfileToEEPROM(struct colorProfile *profile)
{
	u08 profileNumber = getAvailableColorProfileNumber();
	u16 address = getCPAddress(profileNumber);
	eeprom_write_block((const void*)profile, (void*)address, 
		sizeof(struct colorProfile));
	u08 count = eeprom_read_byte((u08*)CP_COUNT_ADDR);
	eeprom_update_byte((u08*)CP_COUNT_ADDR, ++count);
	rprintf("Profile written to %d. Total: %d\n\r", profileNumber, count);
	setCPAvailability(profileNumber, FALSE);
	return profileNumber;
}

struct colorProfile readColorProfileFromEEPROM(u08 profileNumber)
{
	struct colorProfile profile;
	u16 address = getCPAddress(profileNumber);
	eeprom_read_block((void*)&profile, (const void*)address,
		sizeof(struct colorProfile));
	return profile;
}

void copyStringToCPname(char str[], struct colorProfile *profile)
{
	u08 nameIter = 0;
	while (*str)
	{
		profile->name[nameIter++] = *str;
		str++;
	}
	profile->name[nameIter] = 0;
}

void renameColorProfile(u08 profileNum)
{
	struct colorProfile profile = readColorProfileFromEEPROM(profileNum);
	char *namePtr = getString(sizeof(profile.name));
	copyStringToCPname(namePtr, &profile);
	free((void*)namePtr);
	u16 address = getCPAddress(profileNum);
	eeprom_write_block((const void*)&profile, (void*)address, 
		sizeof(struct colorProfile));
	profile = readColorProfileFromEEPROM(profileNum);
}

void colorProfileMenu(void)
{
	while (TRUE)
	{
		rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("COLOR PROFILE MAIN MENU");		rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("N - Create New");				rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("S - Select Existing");			rprintfCRLF();	rprintfCRLF();
		rprintfProgStrM("Esc - Exit");					rprintfCRLF();	rprintfCRLF();
		char keyTap;
GetMainAction:
		keyTap = getKeyTap(TRUE);
		if (keyTap == ASCII_ESCAPE)
			break;
		else if (keyTap == 'N')
		{
			struct colorProfile profile;
			char *namePtr = getString(sizeof(profile.name));
			copyStringToCPname(namePtr, &profile);
			free((void*)namePtr);
			for (color profileColor = 0; profileColor < MAX_COLORS; profileColor++)
			{
				rprintf("Color %d: ", profileColor);
				printColorName(profileColor, TRUE);
				profile.colorInfo[profileColor].name = profileColor;
				rprintfProgStrM("R Lower\n");
				profile.colorInfo[profileColor].r_lower = validateColorLimit(getNumber(3));
				rprintfProgStrM("R Upper\n");
				profile.colorInfo[profileColor].r_upper = validateColorLimit(getNumber(3));
				rprintfProgStrM("G Lower\n");
				profile.colorInfo[profileColor].g_lower = validateColorLimit(getNumber(3));
				rprintfProgStrM("G Upper\n");
				profile.colorInfo[profileColor].g_upper = validateColorLimit(getNumber(3));
				rprintfProgStrM("B Lower\n");
				profile.colorInfo[profileColor].b_lower = validateColorLimit(getNumber(3));
				rprintfProgStrM("B Upper\n");
				profile.colorInfo[profileColor].b_upper = validateColorLimit(getNumber(3));
			}
			writeNewColorProfileToEEPROM(&profile);
		}
		else if (keyTap == 'S')
		{
			u08 numProfiles = eeprom_read_byte((const u08*)CP_COUNT_ADDR);
SelectProfileMenu:
			rprintfCRLF();
			rprintf("%d Profiles.\n\r", numProfiles);
			if (numProfiles > 0)
			{
				for (u08 profileIter = 0; profileIter < MAX_COLOR_PROFILES; profileIter++)
				{
					BOOL available = getCPAvailability(profileIter);
					if (!available) // Profile Number is occupied
					{
						rprintf("\t%d. ", profileIter);
						struct colorProfile profile = readColorProfileFromEEPROM(profileIter);
						rprintfStr(profile.name);
						rprintfCRLF();
					}
				}
				rprintfCRLF();
				rprintfProgStrM("Select Profile\n\r");
				u08 profileNum;
GetProfileNumber:
				rprintfCRLF();
				profileNum = getNumber(2);
				if (profileNum < 0 || profileNum >= MAX_COLOR_PROFILES)
					goto GetProfileNumber;
				if (getCPAvailability(profileNum))
				{
					rprintfProgStrM("Try Again\n\r");
					goto GetProfileNumber;
				}
				rprintf("Selected %d\n\r", profileNum);
				copyCPfromEEPROMtoRAM(profileNum);
				struct colorProfile profile = readColorProfileFromEEPROM(profileNum);
				displayColorProfile(&profile);
				rprintfCRLF();
				rprintfProgStrM("E - Edit");			rprintfCRLF();	rprintfCRLF();
				rprintfProgStrM("R - Rename");			rprintfCRLF();	rprintfCRLF();
				rprintfProgStrM("D - Delete");			rprintfCRLF();	rprintfCRLF();
				rprintfProgStrM("B - Back to List");	rprintfCRLF();	rprintfCRLF();
				rprintfProgStrM("C - Continue");		rprintfCRLF();	rprintfCRLF();
GetProfileAction:
				keyTap = getKeyTap(TRUE);
				if (keyTap == 'E')
				{
					adjustColor(menu_chooseColor());
					copyCPfromRAMtoEEPROM(profileNum, profile.name);
				}
				else if (keyTap == 'R')
					renameColorProfile(profileNum);
				else if (keyTap == 'D')
				{
					rprintf("Del\n\r");
					setCPAvailability(profileNum, TRUE);
				}
				else if (keyTap == 'B')
					goto SelectProfileMenu;
				else if (keyTap == 'C')
				{
					// Set as default
					eeprom_write_byte((u08*)CURRENT_CP_ADDR, profileNum);
					break;
				}
				else
					goto GetProfileAction;
			}
		}
		else
			goto GetMainAction;
	} // end while
}

void initVision()
{
	u08 currentDefault = eeprom_read_byte((u08*)CURRENT_CP_ADDR);
	if (currentDefault == 255)
	{
		rprintfProgStrM("Program EEPROM");
		while (TRUE);
	}
	rprintf("Colors: %d\n\r", currentDefault);
	copyCPfromEEPROMtoRAM(currentDefault);
	displayColors();
}

void displayColorProfile(struct colorProfile *profile)
{
	rprintfProgStrM("Profile: ");
	rprintfStr(profile->name);
	rprintfCRLF();
	for (u08 colorIter = 0; colorIter < MAX_COLORS; colorIter++)
		displayColor(profile->colorInfo[colorIter]);
}

void adjustColor(color trackedColor)
{
	struct colorStruct *colorAdjust = getColorPointer(trackedColor);
	displayColor(*colorAdjust);
	struct colorStruct colorRestore = *colorAdjust;
	BOOL keepAdjusting = TRUE;
	while (keepAdjusting)
	{
		u08 action = toupper(menu_adjustColor_Main());
		switch (action)
		{
			case 'T': // View Sample
				//getBitmap(trackedColor);
				//displayImage(&bitmap);
				clearTrackingColors();
				addTrackingColor(trackedColor);
				createColorMap();
				setColorMap();
				struct blobArray blobs = getBlobs();
				displayBlobArray(&blobs);
	  			break;
			case 'R':
				*colorAdjust = colorRestore;
				displayColor(*colorAdjust);
				break;
			case 'A':
			{
				displayColor(*colorAdjust);
				rgb trackedPrimary = menu_chooseRGB();
				char limit = menu_chooseLimit();
				u08 *limitQty;
				if (trackedPrimary == R)
				{
					if (toupper(limit) == 'U')
						limitQty = &colorAdjust->r_upper;
					else if (toupper(limit) == 'L')
						limitQty = &colorAdjust->r_lower;
				}
				else if (trackedPrimary == G)
				{
					if (toupper(limit) == 'U')
						limitQty = &colorAdjust->g_upper;
					else if (toupper(limit) == 'L')
						limitQty = &colorAdjust->g_lower;
				} 
				else if (trackedPrimary == B)
				{
					if (toupper(limit) == 'U')
						limitQty = &colorAdjust->b_upper;
					else if (toupper(limit) == 'L')
						limitQty = &colorAdjust->b_lower;
				}
				while (TRUE)
				{
					char inc = menu_chooseIncrement();
					if (inc == ASCII_ESCAPE)
						break;
					if (toupper(inc) == 'I')
						*limitQty = *limitQty + 16;
					else if (toupper(inc) == 'D')
						*limitQty = *limitQty - 16;
					else if (toupper(inc) == ASCII_ESCAPE)
						break;
					rprintf("%d\n\r", *limitQty);
				}
				displayColor(*colorAdjust);
	  			break;
	  		}
	  		case ASCII_ESCAPE:
	  			keepAdjusting = FALSE;
		}
	}
}

char menu_adjustColor_Main()
{
	rprintfCRLF();
	rprintfProgStrM("ADJUST COLORS");				rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("T - Test");        			rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("A - Adjust RGB");				rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("R - Restore Prev RGB");		rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("Esc - Exit");					rprintfCRLF();	rprintfCRLF();
	char keyTap = getKeyTap(TRUE);
	return keyTap;
}

char menu_chooseIncrement(void)
{
	rprintfCRLF();
	rprintfProgStrM("CHOOSE INCREMENT");	rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("I - Increase");   		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("D - Decrease");  		rprintfCRLF();	rprintfCRLF();
	char keyTap = getKeyTap(TRUE);
	return keyTap;  
}

rgb menu_chooseRGB(void)
{
	rprintfCRLF();
	rprintfProgStrM("CHOOSE: R - G - B");
	rprintfCRLF();
	rgb selection;
	char keyTap = getKeyTap(TRUE);
	if (toupper(keyTap) == 'R')
		selection = R;
	else if (toupper(keyTap) == 'G')
		selection = G;
	else if (toupper(keyTap) == 'B')
		selection = B;
	return selection;
}

char menu_chooseLimit()
{
	rprintfCRLF();
	rprintfProgStrM("CHOOSE LIMIT");		rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("U - Adjust Upper");	rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("L - Adjust Lower");	rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("Esc - Exit");			rprintfCRLF();	rprintfCRLF();
	char keyTap = getKeyTap(TRUE);
	return keyTap;	
}

color menu_chooseColor(void)
{
	color returnColor;
	rprintfCRLF();
	rprintfProgStrM("CHOOSE COLOR");	rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("Y - Yellow");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("O - Orange");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("R - Red");			rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("B - Blue");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("G - Green");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("N - Brown");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("W - White");		rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("K - Black");		rprintfCRLF();	rprintfCRLF();
	char keyTap;
	getKeyTap:
	keyTap = getKeyTap(TRUE);
	switch (keyTap)
	{
		case 'Y':
			returnColor = YELLOW;
			break;
		case 'O':
			returnColor = ORANGE;
			break;
		case 'R':
			returnColor = RED;
			break;
		case 'B':
			returnColor = BLUE;
			break;
		case 'G':
			returnColor = GREEN;
			break;
		case 'N':
			returnColor = BROWN;
			break;
		case 'W':
			returnColor = WHITE;
			break;
		case 'K':
			returnColor = BLACK;
			break;
		default:
			goto getKeyTap;
	}
	return returnColor;	
}

char menu_adjustColors_Main(void)
{
	rprintfCRLF();
	rprintfProgStrM("ADJUST COLORS");		rprintfCRLF();	rprintfCRLF();
	rprintfProgStrM("S - Select Color");	rprintfCRLF();	rprintfCRLF();	
	rprintfProgStrM("Esc - Exit");			rprintfCRLF();	rprintfCRLF();
	char keyTap = getKeyTap(TRUE);
	return keyTap;
}

void adjustColors(void)
{
	while (TRUE)
	{
		char mainAction = menu_adjustColors_Main();
		if (mainAction == ASCII_ESCAPE)
			break;
		else if (toupper(mainAction) == 'S')
		{
			color trackedColor = menu_chooseColor();
			adjustColor(trackedColor);
		}
	}
}

u08 getBlobHeight(struct blob *pBlob)
{
	u08 height = pBlob->cornerBR.y - pBlob->cornerUL.y + 1;
	return height;
}

u08 getBlobWidth(struct blob *pBlob)
{
	u08 width = pBlob->cornerBR.x - pBlob->cornerUL.x + 1;
	return width;
}

struct point getBlobMiddle(struct blob *pBlob)
{
	struct point blobMiddle;
	u08 width = getBlobWidth(pBlob);
	u08 height = getBlobHeight(pBlob);
	blobMiddle.x = pBlob->cornerUL.x + (width/2);
	blobMiddle.y = pBlob->cornerUL.y + (height/2);
	return blobMiddle;
}

BOOL samePoint(struct point point1, struct point point2)
{
	BOOL same = FALSE;
	if ((point1.x == point2.x) && (point1.y == point2.y))
		same = TRUE;
	return same;
}

BOOL pointReachedX(side moveSide, struct point currPoint, u08 targX)
{
	BOOL reached = FALSE;
	if (moveSide == LEFT)
	{
		if (currPoint.x >= targX)
			reached = TRUE;
	}
	else // RIGHT
	{
		if (currPoint.x <= targX)
			reached = TRUE;
	}
	return reached;
}

BOOL pointReachedY(direction moveDir, struct point currPoint, u08 targY)
{
	BOOL reached = FALSE;
	if (moveDir == FORWARD)
	{
		if (currPoint.y >= targY)
			reached = TRUE;
	}
	else // BACKWARD
	{
		if (currPoint.y <= targY)
			reached = TRUE;
	}
	return reached;
}

BOOL blobsTouching(struct blob *blob1, struct blob *blob2)
{
	BOOL returnVal = FALSE;
	// Determine if there is horizontal overlap
	if (((blob1->cornerUL.x >= blob2->cornerUL.x && blob1->cornerUL.x < blob2->cornerBR.x) ||
	    (blob1->cornerBR.x <= blob2->cornerBR.x && blob1->cornerBR.x > blob2->cornerUL.x))
		&&
	    ((blob1->cornerUL.y >= blob2->cornerUL.y && blob1->cornerUL.y <= blob2->cornerBR.y) ||
	    (blob1->cornerBR.y <= blob2->cornerBR.y && blob1->cornerBR.y >= blob2->cornerUL.y)))
		returnVal = TRUE;
	return returnVal;
}

BOOL dataContigious(u08 prevRowX1, u08 prevRowX2,
                   u08 currRowX1, u08 currRowX2)
{
	BOOL returnVal = FALSE;
	//if (!(currRowX2 <= prevRowX1) && !(currRowX1 >= prevRowX2))
	if (!(currRowX2 < prevRowX1) && !(currRowX1 > prevRowX2))
		returnVal = TRUE;
	return returnVal;
}

BOOL testCentered(struct blob *object, orientation dir, u08 error)
{
	BOOL returnVal = FALSE;
	u08 middleMass, pictureCenter, pictureCenterLow, pictureCenterHigh;
	struct point middle = getBlobMiddle(object);
	if (dir == HORIZONTAL)
	{
		middleMass = middle.x;
		pictureCenter = getPictureCenter(HORIZONTAL);
	}
	else
	{
		middleMass = middle.y;
		pictureCenter = getPictureCenter(VERTICAL);
	}
	pictureCenterLow = pictureCenter - error;
	pictureCenterHigh = pictureCenter + error;

	if (middleMass >= pictureCenterLow && middleMass <= pictureCenterHigh)
		returnVal = TRUE;
	return returnVal;
}

void displayBlobArray(struct blobArray *blobs)
{
	rprintf("Blob Array Length: %d\n\r", blobs->length);
	for (u08 blob = 0; blob < blobs->length; blob++)
	{
		rprintf("Blob %d:\n\r", blob);
		displayBlob(&blobs->contents[blob]);
	}
}

void displayBlob(struct blob *trackedBlob)
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

void emptyBlob(struct blob *blobStruct)
{
	blobStruct->cornerUL.x = 0;
	blobStruct->cornerUL.y = 0;
	blobStruct->cornerBR.x = 0;
	blobStruct->cornerBR.y = 0;
}
