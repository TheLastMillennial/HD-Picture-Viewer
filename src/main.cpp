/*HD Picture Viewer
* By TheLastMillennial
* https://github.com/TheLastMillennial/HD-Picture-Viewer
* Build With:    ./make debug --directory="path/to/HD-Picture-Viewer"
*/

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <fileioc.h>
#include <debug.h>
#include <compression.h>

#include "main.h"
#include "loadingBarHandler.h"
#include "globals.h"
#include "guiUtils.h"
#include "gfx/errorgfx.h"


int main(void)
{
	gfx_Begin();
	gfx_SetTextTransparentColor(254);
	drawSplashScreen();

	LoadingBar& loadingBar = LoadingBar::getInstance();

	loadingBar.resetLoadingBar(2);

	//checks if the database exists and is ready 0 failure; 1 created; 2 exists
	if (isDatabaseReady() == 0)
	{
		// Display error then quit.
		dbg_sprintf(dbgout, "\nDatabase not ready");
		PrintCentered("Database failure.");
		while (!os_GetCSC());
		gfx_End();
		return 0;
	}

	loadingBar.increment();

	// rebuildDB() returns how many images were found.
	uint24_t picsDetected {rebuildDB()};
	if (picsDetected == 0)
	{
		drawNoImagesFound();
		while (!os_GetCSC());
		gfx_End();
		return 0;
	}
	else
	{
		loadingBar.increment();
		//display the list of images
		drawHomeScreen(picsDetected);
		//quit
		gfx_End();
		return 0;
	}
}

// Display UI to select an image
void drawHomeScreen(uint24_t picsCount) {
	char* picNames{ static_cast<char*>(malloc(picsCount * BYTES_PER_IMAGE_NAME)) };
	ti_var_t database{ ti_Open("HDPICDB","r") };
	uint24_t startName{ 0 },
		maxAllowedWidthInPxl{ MAX_THUMBNAIL_WIDTH }, maxAllowedHeightInPxl{ MAX_THUMBNAIL_HEIGHT };
	int24_t xOffset{ 0 }, yOffset{ 0 };

	bool menuEnter, menuQuit,
		menuUp, menuDown, menuHelp,
		prev, next,
		deletePic, resetPic, redrawPic,
		zoomIn, zoomOut, zoomMax,
		panUp, panDown, panLeft, panRight;

	//seeks to the first image name
	ti_Seek(8, SEEK_SET, database);
	//loops through every picture that was detected and store the image name to picNames
	for (uint24_t i{ 0 };i <= picsCount;i++) {
		ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME], 8, 1, database);
		picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
		ti_Seek(8, SEEK_CUR, database);
	}

	//set up variable that checks if drawImage failed
	uint8_t imageErr{ 0 };
	
	/* main menu */
	gfx_FillScreen(PALETTE_BLACK);
	drawMenu(startName, picNames, picsCount);
	//thumbnail
	drawImage(startName, 180, 120, 0, 0, false);

	/* UI */
	bool quitProgram{ false };
	uint8_t errorID = 0;
	do {
		static bool fullScreenImage{ false };	

		//scans the keys for keypress
		kb_Scan();

		menuHelp  = kb_Data[1] & kb_Mode;
		menuEnter = kb_Data[6] & kb_Enter;
		menuQuit  = kb_Data[6] & kb_Clear;
		menuUp    = kb_Data[7] & kb_Up;
		menuDown  = kb_Data[7] & kb_Down;

		next      = kb_Data[1] & kb_Graph;
		prev      = kb_Data[1] & kb_Yequ;
		resetPic  = kb_Data[1] & kb_Window;
		deletePic = kb_Data[1] & kb_Del;
		zoomMax   = kb_Data[1] & kb_Zoom;
		zoomIn    = kb_Data[6] & kb_Add;
		zoomOut   = kb_Data[6] & kb_Sub;
		redrawPic = kb_Data[6] & kb_Enter;
		panUp     = kb_Data[7] & kb_Up;
		panDown   = kb_Data[7] & kb_Down;
		panLeft   = kb_Data[7] & kb_Left;
		panRight  = kb_Data[7] & kb_Right;

		//if any key was pressed
		if (kb_AnyKey()) {

			// clear. Go back.
			if (menuQuit){
				//If we're viewing an image, exit to menu. If we're already on menu, quit program.
				if (fullScreenImage){
					fullScreenImage = false;
					resetPic=true;
					redrawPic=true;
					errorID = 1;
					gfx_FillScreen(PALETTE_BLACK);
					while(kb_AnyKey()!=0); //wait for key lift
				}else{
					quitProgram = true;
					break; 
				}
			}
			
			// enter. Fullscreen image
			if (menuEnter){
				if(!fullScreenImage)
					resetPic=true;
				fullScreenImage = true;
				redrawPic = true;
				errorID = 2;
			}
			
			// mode. Show help.
			if (menuHelp){
				drawHelp();
				while(kb_AnyKey()!=0); //wait for key lift
				while(!os_GetCSC()); //wait for key press
				gfx_FillScreen(PALETTE_BLACK);
				resetPic=true;
				redrawPic=true;
			}
			
			//left, right, up, down. Image panning.
			if (fullScreenImage){
				if (panLeft) {
					xOffset++;
					redrawPic=true;
					errorID = 3;
				}
				if (panRight) {
					xOffset--;
					redrawPic=true;	
					errorID = 4;
				}
				if (panUp) {
					yOffset--;
					redrawPic=true;	
					errorID = 5;
				}
				if (panDown) {
					yOffset++;
					redrawPic=true;	
					errorID = 6;
				}
			}

			//Zoom key. Zoom in as far as possible while maintaining full quality
			if(zoomMax && fullScreenImage)
			{
				//pull image full dimensions from database
				char picDimensions[6];
				ti_var_t database{ ti_Open("HDPICDB","r") };
				ti_Seek(18 + (16 * startName), SEEK_CUR, database);
				ti_Read(picDimensions, 6, 1, database);
				ti_Close(database);

				//convert string to int
				maxAllowedWidthInPxl= (((static_cast<uint24_t>(picDimensions[0]) - '0') * 100 + (static_cast<uint24_t>(picDimensions[1]) - '0') * 10 + static_cast<uint24_t>(picDimensions[2]) - '0') + 1) * SUBIMG_WIDTH_AND_HEIGHT;
				maxAllowedHeightInPxl=(((static_cast<uint24_t>(picDimensions[3]) - '0') * 100 + (static_cast<uint24_t>(picDimensions[4]) - '0') * 10 + static_cast<uint24_t>(picDimensions[5]) - '0') + 1) * SUBIMG_WIDTH_AND_HEIGHT;

				imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					maxAllowedWidthInPxl = maxAllowedWidthInPxl / ZOOM_SCALE;
					maxAllowedHeightInPxl = maxAllowedHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);
					imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
					//if zooming back out didn't fix it, abort.
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Cant zoom in!!");

						PrintCenteredX("Error zooming in.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
			}
			
			//Plus key. Zoom in by double
			if (zoomIn && fullScreenImage) {
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomIn error"); }
				//doubles zoom
				maxAllowedWidthInPxl = maxAllowedWidthInPxl * ZOOM_SCALE;
				maxAllowedHeightInPxl = maxAllowedHeightInPxl * ZOOM_SCALE;
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom In\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);
				imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					maxAllowedWidthInPxl = maxAllowedWidthInPxl / ZOOM_SCALE;
					maxAllowedHeightInPxl = maxAllowedHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);
					imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
					//if zooming back out didn't fix it, abort.
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Cant zoom in!!");

						PrintCenteredX("Error zooming in.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
			}

			//subtract key. Zoom out by double.
			if (zoomOut && fullScreenImage) {
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom Out");
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomOut error"); }

				//ensure we can zoom out without maxAllowedWidthInPxl or maxAllowedHeightInPxl becoming 0
				if (maxAllowedWidthInPxl / ZOOM_SCALE != 0 && maxAllowedHeightInPxl / ZOOM_SCALE != 0) {
					//apply the zoom out to the width and height
					maxAllowedWidthInPxl = maxAllowedWidthInPxl / ZOOM_SCALE;
					maxAllowedHeightInPxl = maxAllowedHeightInPxl / ZOOM_SCALE;

					//dbg_sprintf(dbgout, "\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);
					imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
					//this means we can't zoom out any more. Zoom back in.
					if (imageErr != 0) {
						//dbg_sprintf(dbgout, "\nCant zoom out trying zooming in...");
						maxAllowedWidthInPxl = maxAllowedWidthInPxl * ZOOM_SCALE;
						maxAllowedHeightInPxl = maxAllowedHeightInPxl * ZOOM_SCALE;
						//dbg_sprintf(dbgout, "\n Zoomed in\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);

						imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
						//if zooming back in didn't fix it, abort.
						if (imageErr != 0) {
							dbg_sprintf(dbgout, "\nERR: Cant zoom out!!");

							PrintCenteredX("Error zooming out.", 130);
							PrintCenteredX("Press any key to quit.", 140);
							while (!os_GetCSC());
							ti_Close(database);
							free(picNames);
							gfx_End();
							return;
						}
					}
				}
				else
				{
					//dbg_sprintf(dbgout, "\nmaxWidth or maxAllowedHeightInPxl too small. \n Zoom out aborted.");
					//dbg_sprintf(dbgout, "\n maxAllowedWidthInPxl: %d\n maxAllowedHeightInPxl: %d ", maxAllowedWidthInPxl, maxAllowedHeightInPxl);
					//redraw the image. If it fails, I dunno why. It should be the exact same image as was previously displayed
					imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, true);
					//err handler
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Issue displaying same image??");

						PrintCenteredX("Error with zoom.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
			}

			//Delete. delete all appvars related to current image
			if (deletePic) {
				//the current palette is about to be deleted. Set the default palette
				gfx_SetDefaultPalette(gfx_8bpp);
				//we don't want the user seeing the horrors of their image with the wrong palette
				gfx_FillScreen(PALETTE_BLACK);
				gfx_SetTextFGColor(XLIBC_GREY);
				gfx_SetTextBGColor(PALETTE_BLACK);
				gfx_SetTextScale(1,1);
				PrintCenteredX("Deleting Picture...", 120);

				//delete the palette and all subimages
				deleteImage(startName);
				PrintCenteredX("Picture deleted.", 130);
				PrintCenteredX("Press any key.", 140);
				while (!os_GetCSC());

				//picture names will change. Delete what we currently have
				free(picNames);
				picNames = nullptr;
				startName = 0;
				
				//set color for splash screen
				gfx_SetTextFGColor(XLIBC_GREY);
				gfx_SetTextBGColor(PALETTE_BLACK);
				
				//rebuild the database to account for the deleted image. 
				ti_Close(database);
				picsCount = rebuildDB();
				database = ti_Open("HDPICDB", "r");

				//check if all images were deleted. If so, just quit.
				if (picsCount == 0) {
					drawNoImagesFound();
					while (!os_GetCSC());
					gfx_End();
					return;
				}

				//ensure text is readable
				gfx_SetTextFGColor(XLIBC_GREY);
				gfx_SetTextBGColor(PALETTE_BLACK);
				//re-allocate memory for the picture names
				picNames = static_cast<char*>(malloc(picsCount * BYTES_PER_IMAGE_NAME));

				//seeks to the first image name
				ti_Seek(8, SEEK_SET, database);
				//loops through every picture that was detected and store the image name to picNames
				for (uint24_t i{ 0 };i <= picsCount;i++) {
					ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME], 8, 1, database);
					picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
					ti_Seek(8, SEEK_CUR, database);
				}

				//re construct the GUI
				gfx_FillScreen(PALETTE_BLACK);
				resetPic=true;
				redrawPic = true;
			}

			/* Graph or down. Increases the name to start on and redraws the text */
			if (next || (menuDown &&  !fullScreenImage)) {
				startName++;
				//make sure user can't scroll down too far
				if (startName > picsCount-1)
				{
					startName = picsCount-1;
				}

				resetPic = fullScreenImage;
				redrawPic = true;
				errorID = 8; //if an error is thrown, then we've scrolled past the safety barrier somehow.
			}

			/* Y= or up. Decreases the name to start on and redraws the text */
			if (prev || (menuUp && !fullScreenImage)) {
				startName--;
				// Checks if startName underflowed. startName shouldn't be more than the max number of images possible.
				if (startName > MAX_IMAGES)
				{
					startName = 0;
				}
				resetPic = fullScreenImage;
				redrawPic = true;
				errorID = 9; //if an error is thrown, then we've scrolled past the safety barrier somehow.
			}
			
			//Window. Reset zoom and pan
			if (resetPic) {
				if (fullScreenImage)
				{
					xOffset = 0;
					yOffset = 0;
					maxAllowedWidthInPxl = LCD_WIDTH;
					maxAllowedHeightInPxl = LCD_HEIGHT;
				}else{
					xOffset = 0;
					yOffset = 0;
					maxAllowedWidthInPxl = MAX_THUMBNAIL_WIDTH;
					maxAllowedHeightInPxl = MAX_THUMBNAIL_HEIGHT;
				}
				redrawPic = true;
			}
			
			// If necessary, draw the image with new settings.
			if (redrawPic) {
				if(!fullScreenImage)
				{
					drawMenu(startName, picNames, picsCount);
				}
				while(kb_AnyKey()!=0); //wait for key lift
				imageErr = drawImage(startName, maxAllowedWidthInPxl, maxAllowedHeightInPxl, xOffset, yOffset, fullScreenImage);
				if (imageErr != 0) {
					PrintCenteredX("Error: ", 150);
					gfx_PrintUInt(errorID,3);
					PrintCenteredX("Press any key to quit.", 160);
					while (!os_GetCSC());
					ti_Close(database);
					free(picNames);
					gfx_End();
					return;
				}
			}
			if(!fullScreenImage)
				drawWatermark();
		}
	} while (!quitProgram);

	ti_Close(database);
	free(picNames);
}


void deleteImage(uint24_t picName) {
	//open the database to figure out what image we're about to delete
	ti_var_t database{ ti_Open("HDPICDB","r") };
	char imgWH[6], imgID[2], picAppvarToFind[9], palName[9];

	//seeks past header (8bytes), imgName, and unselected images
	ti_Seek(16 + (16 * picName), SEEK_CUR, database);
	//reads the image letter ID (2 bytes)
	ti_Read(imgID, 2, 1, database);
	//reads the image width/height (6 bytes)
	ti_Read(imgWH, 6, 1, database);
	//closes database
	ti_Close(database);

	/*converts the char numbers from the header appvar into uint numbers
	The header has 6 numbers so the below ? will go from 0-5
	(uint24_t)imgWH[?]-'0')*100 covers the 100's place
	(uint24_t)imgWH[?]-'0')*10 covers the 10's place
	(uint24_t)imgWH[?]-'0' covers the 1's place
	+1 accounts for 0 being the starting number
	*/
	uint24_t picWidthInSubimages{ ((static_cast<uint24_t>(imgWH[0]) - '0') * 100 + (static_cast<uint24_t>(imgWH[1]) - '0') * 10 + static_cast<uint24_t>(imgWH[2]) - '0') + 1 };
	uint24_t picHeightInSubimages{ ((static_cast<uint24_t>(imgWH[3]) - '0') * 100 + (static_cast<uint24_t>(imgWH[4]) - '0') * 10 + static_cast<uint24_t>(imgWH[5]) - '0') + 1 };


	//deletes palette
	sprintf(palName, "HP%.2s0000", imgID);
	int delSuccess{ ti_Delete(palName) };
	if (delSuccess == 0) {
		PrintCenteredX(palName, 120);
		dbg_sprintf(dbgout, "\nERR: Issue deleting palette");
	}
	//sets up loading bar finish line
	gfx_SetColor(PALETTE_WHITE);
	gfx_VertLine_NoClip(260,153,7);

	LoadingBar& loadingBar = LoadingBar::getInstance();
	loadingBar.resetLoadingBar(picWidthInSubimages * picHeightInSubimages);

	//delete every subimage
	for (uint24_t xSubimage = (picWidthInSubimages - 1);xSubimage < MAX_UINT; xSubimage--) {
		for (uint24_t ySubimage = (picHeightInSubimages - 1);ySubimage < MAX_UINT; ySubimage--) {

			//combines the separate parts into one name to search for
			sprintf(picAppvarToFind, "%.2s%03u%03u", imgID, xSubimage, ySubimage);

			/*This opens the variable with the name that was just assembled.
			* It then gets the pointer to that and stores it in a graphics variable
			*/
			delSuccess = ti_Delete(picAppvarToFind);
			//checks if the subimage does not exist
			if (delSuccess == 0) {
				//subimage does not exist
				dbg_sprintf(dbgout, "\nERR: Issue deleting subimage");
				dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgID, xSubimage, ySubimage);
			}
			loadingBar.increment();
		}
	}
}

/* Draws the image stored in database at position startName.
* Draws the image at location x,y starting at top left corner.
* If x=-1 then make image horizontally centered in the screen.
* If y=-1 then make image vertically centered on the screen.
* Image will automatically be resized to same aspect ratio so you just set the max width and height (4,3 will fit the screen normally)
* If successful, returns 0. Otherwise returns 1
*/
uint8_t drawImage(uint24_t picName, uint24_t maxAllowedWidthInPxl, uint24_t maxAllowedHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen) {
	dbg_sprintf(dbgout, "\n\n--IMAGE CHANGE--");
	ti_var_t database{ ti_Open("HDPICDB","r") };

	char imgWH[6], picID[2], picAppvarToFind[9], palName[9];
	int24_t scaleNum{ 1 }, scaleDen{ 1 }, newSubimgWidthHeight{ 0 };
	if (refreshWholeScreen)
	{
		gfx_FillScreen(PALETTE_BLACK);
	}
	else
	{
		//used for thumbnails
		gfx_SetColor(PALETTE_BLACK);
		gfx_FillRectangle_NoClip(150, 0, 170, 240);
	}
	//seeks past header (8bytes), imgName, and unselected images
	ti_Seek(16 + (16 * picName), SEEK_CUR, database);
	//reads the image letter ID (2 bytes)
	ti_Read(picID, 2, 1, database);
	//reads the image width/height (6 bytes)
	ti_Read(imgWH, 6, 1, database);
	//closes database
	ti_Close(database);

	//Converts the width/height from a char array into two integers by converting char into decimal value
	//then subtracting 48 to get the actual number.
	dbg_sprintf(dbgout, "\nimgHeader: %s \n", imgWH);

	/*converts the char numbers from the header appvar into uint numbers
	(uint24_t)imgWH[?]-'0')*100 covers the 100's place
	(uint24_t)imgWH[?]-'0')*10 covers the 10's place
	(uint24_t)imgWH[?]-'0' covers the 1's place
	+1 accounts for 0 being the starting number
	*/
	int24_t picWidthInSubimages{ ((static_cast<int24_t>(imgWH[0]) - '0') * 100 + (static_cast<int24_t>(imgWH[1]) - '0') * 10 + static_cast<int24_t>(imgWH[2]) - '0') + 1 };
	int24_t picHeightInSubimages{ ((static_cast<int24_t>(imgWH[3]) - '0') * 100 + (static_cast<int24_t>(imgWH[4]) - '0') * 10 + static_cast<int24_t>(imgWH[5]) - '0') + 1 };
	uint24_t maxAllowedWidthInSubimages{ (maxAllowedWidthInPxl / SUBIMG_WIDTH_AND_HEIGHT) }; 
	uint24_t maxAllowedHeightInSubimages{ (maxAllowedHeightInPxl / SUBIMG_WIDTH_AND_HEIGHT) };
	dbg_sprintf(dbgout, "\n maxWS: %d\n widthS: %d\n maxHS: %d\n heightS: %d\n", maxAllowedWidthInSubimages, picWidthInSubimages, maxAllowedHeightInSubimages, picHeightInSubimages);

	//checks if it should scale an image horizontally or vertically.
	if((picWidthInSubimages * 80)/320 >= (picHeightInSubimages * 80)/240)
	{
		scaleNum = maxAllowedWidthInSubimages;
		scaleDen = picWidthInSubimages;
		//dbg_sprintf(dbgout, "\nWidth too wide. %d , %d", (picWidthInSubimages * 80)/320, (picHeightInSubimages * 80)/240);
	}
	else
	{
		scaleNum = maxAllowedHeightInSubimages;
		scaleDen = picHeightInSubimages;
		//dbg_sprintf(dbgout, "\nHeight too tall. ");
	}
	
	
	if (scaleNum == 0 || scaleDen == 0) {
		dbg_sprintf(dbgout, "\nERR: Cant zoom out\n scaleNum:%d\n scaleDen:%d", scaleNum, scaleDen);
		return 1;
	}
	newSubimgWidthHeight = SUBIMG_WIDTH_AND_HEIGHT * scaleNum;

	/*
	[jacobly] so now whenever we want to compute
	`x * scale`
	we instead want to compute
	`x * (scaleNum / scaleDen)`
	which can now be reordered to use strictly integer math as
	`(x * scaleNum) / scaleDen`
	
	[jacobly] if you don't know:
	floorDiv(x, y) := x / y;
	roundDiv(x, y) := (x + (y / 2)) / y;
	ceilDiv(x, y) := (x + y - 1) / y;
	[MateoC] huh I didn't know about roundDiv
	*/

	dbg_sprintf(dbgout, "\n newWH: %d \n ScaleNum: %d \n scaleDen: %d \n xOffset: %d \n yOffset %d", newSubimgWidthHeight, scaleNum, scaleDen, xOffset, yOffset);

	//pointer to memory where each unsized subimage will be stored
	gfx_sprite_t* srcImg{ gfx_MallocSprite(SUBIMG_WIDTH_AND_HEIGHT, SUBIMG_WIDTH_AND_HEIGHT) };
	if (!srcImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate src memory!");
		//PrintCenteredX("ERR: Failed to allocate src memory!", 130);
		return 1;
	}
	//scales the subimage width and height to the final output dimensions
	int24_t newSubimgDim{ newSubimgWidthHeight / scaleDen };
	//ensure the resized subimage will fit within the dimensions of the screen.
	if (newSubimgDim > LCD_HEIGHT) {
		dbg_sprintf(dbgout, "\nERR: Subimage will be too large: %d", newSubimgDim);
		//PrintCenteredX("ERR: Output picture too large!", 130);
		free(srcImg);
		return 1;
	}
	//allocates memory for resized image
	gfx_sprite_t* outputImg{ gfx_MallocSprite(newSubimgDim,newSubimgDim) };
	if (!outputImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate output memory!");
		//PrintCenteredX("ERR: Failed to allocate output memory!", 130);
		free(srcImg);
		return 1;
	}

	//sets correct palettes
	sprintf(palName, "HP%.2s0000", picID);
	ti_var_t palSlot{ ti_Open(palName,"r") };
	if (!palSlot) {
		PrintCenteredX(palName, 110);
		PrintCenteredX("ERR: Palette does not exist!", 120);
		PrintCenteredX("Image may have recently been deleted.", 130);
		PrintCenteredX("Try restarting the program.", 140);
		while (!os_GetCSC());
		free(srcImg);
		free(outputImg);
		return 1;
	}
	ti_Seek(24, SEEK_SET, palSlot);
	gfx_SetPalette(ti_GetDataPtr(palSlot), 512, 0);
	ti_Close(palSlot);

	dbg_sprintf(dbgout, "\n-------------------------");

	gfx_SetTextFGColor(PALETTE_WHITE);
	/*
	if (refreshWholeScreen)
		PrintCenteredX("Rendering...",110);
	*/
	
	//Displays all the images
	dbg_sprintf(dbgout, "\nwS: %d\nxO: %d", picWidthInSubimages, xOffset);
	dbg_sprintf(dbgout, "\nhS: %d\nyO: %d", picHeightInSubimages, yOffset);


	//This calculates the number of subimages you can fit in the screen horizontally
	//we know the horizontal resolution of the screen is 320px. 
	//We can get the width of each subimage by doing newSubimgWidthHeight/scaleDen
	//ceilDiv since we don't want missing subimages.
	int24_t rightMostSubimg{ ceilDiv(static_cast<int24_t>( LCD_WIDTH) , (newSubimgWidthHeight / scaleDen)) };
	//leftmost and topmost always starts at 0
	int24_t leftMostSubimg{ 0 };
	int24_t topMostSubimg { 0 };
	//This calculates the number of subimages you can fit in the screen vertically
	//we know the vertical resolution of the screen is 240px. 
	//We can get the width of each subimages by doing newSubimgWidthHeight/scaleDen
	//ceilDiv since we don't want missing subimages. (Overflow is compensated for, if necessary, below)
	int24_t bottomMostSubimg{ ceilDiv(static_cast<int24_t> (LCD_HEIGHT) , (newSubimgWidthHeight / scaleDen)) };


	/* Apply pan offsets */
	//if we're panning horizontally, shift the rightmost and leftmost subimages (xOffset is negative in this case)
	rightMostSubimg -= xOffset;
	leftMostSubimg -= xOffset;
	//if we're panning vertically, shift the topmost and bottommost subimages (yOffset is negative in this case)
	bottomMostSubimg += yOffset;
	topMostSubimg += yOffset;


	/* Ensure we don't try to display more subimages than exist */
	if (rightMostSubimg > picWidthInSubimages)
		rightMostSubimg = picWidthInSubimages;
	if (leftMostSubimg < 0)
		leftMostSubimg = 0;
	if (bottomMostSubimg > picHeightInSubimages)
		bottomMostSubimg = picHeightInSubimages;
	if (topMostSubimg < 0)
		topMostSubimg = 0;

	dbg_sprintf(dbgout, "\nrightMost %d\nLeftMost %d", rightMostSubimg, leftMostSubimg);
	dbg_sprintf(dbgout, "\ntopMost %d\nbottomMost %d", topMostSubimg, bottomMostSubimg);

	/* Display final image */

	//the -1 is to account for both the
	//the +1 is to prevent underflow which would cause an infinite loop
	//this for loop outputs pic right to left, top to bottom
	const int24_t xFirstID{ (leftMostSubimg) - 1 }, xLastID{(rightMostSubimg) - 1 };
	const int24_t yFirstID{ (topMostSubimg) - 1 }, yLastID{ (bottomMostSubimg) - 1 };

	const uint8_t thumbnailOffsetX = refreshWholeScreen ? 0 : 150;
	const uint24_t thumbnailOffsetY = refreshWholeScreen ? 0 : ((240-(newSubimgDim*bottomMostSubimg))/2);

	dbg_sprintf(dbgout, "\nxFirstID %d \nxLastID %d", xFirstID, xLastID);


	// Loop through all subimages to create full image
	for (int24_t xSubimgID{ xLastID };xSubimgID > xFirstID;--xSubimgID) {
		const uint24_t subimgPxlPosX{ static_cast<uint24_t>(thumbnailOffsetX) + static_cast<uint24_t>((xSubimgID + xOffset) * (newSubimgWidthHeight / scaleDen)) };

		//this for loop outputs pic bottom to top
		for (int24_t ySubimgID{ yLastID };ySubimgID > yFirstID;--ySubimgID) {
			const uint24_t subimgPxlPosY{ thumbnailOffsetY + static_cast<uint24_t>((ySubimgID - yOffset) * (newSubimgWidthHeight / scaleDen)) };

			//a key interrupted output. Quit immediately

			if(os_GetCSC())
			{
					
				if(refreshWholeScreen)
				{
					PrintCenteredX("Rendering Halted.",130);
					PrintCenteredX("Press [enter] to restart.",140);
				}
				//free up source and output memory
				free(srcImg);
				free(outputImg);
				return 0;
			}
			
			//combines the separate parts into one name to search for
			sprintf(picAppvarToFind, "%.2s%03u%03u", picID, xSubimgID, ySubimgID);
			dbg_sprintf(dbgout, "\n%.2s%03u%03u", picID, xSubimgID, ySubimgID);
			/*
			* This opens the variable with the name that was just assembled.
			* It then gets the pointer to that and stores it in a graphics variable
			*/
			const ti_var_t subimgSlot{ ti_Open(picAppvarToFind,"r") };
			//checks if the subimage exists
			if (subimgSlot) {
				//subimage exists, load it
				//seeks past header
				ti_Seek(16, SEEK_CUR, subimgSlot);
				//store the original image into srcImg
				//srcImg = (gfx_sprite_t*)ti_GetDataPtr(subimgSlot);

				zx0_Decompress(srcImg, ti_GetDataPtr(subimgSlot));
				//resizes it to outputImg size
				gfx_ScaleSprite(srcImg, outputImg);

				//displays subimage
				//if we are displaying an edge image, clip the subimage. Otherwise don't clip for extra speed.
				if (xSubimgID == xLastID || ySubimgID == yLastID) {
					gfx_Sprite(outputImg, subimgPxlPosX, subimgPxlPosY );
				}
				else {
					gfx_Sprite_NoClip(outputImg, subimgPxlPosX, subimgPxlPosY);
				}

			}
			else {
				//subimage does not exist, display error image
				dbg_sprintf(dbgout, "\nERR: Subimage doesn't exist!");
				dbg_sprintf(dbgout, "\n %s", picAppvarToFind);
				//dbg_sprintf(dbgout,"\nERR: \nxsubimage: %d \newSubimgWidthHeight: %d \nscaleDen: %d",xSubimage,newSubimgWidthHeight,scaleDen);
				zx7_Decompress(srcImg, errorTriangle_compressed);
				//resizes it to outputImg size
				gfx_ScaleSprite(srcImg, outputImg);
				//displays the output image
				//dbg_sprintf(dbgout,"\nxsubimage: %d \newSubimgWidthHeight: %d \nscaleDen: %d\n",xSubimage,newSubimgWidthHeight,scaleDen);
				gfx_Sprite(outputImg, subimgPxlPosX, subimgPxlPosY);
				//while(!os_GetCSC());
				continue;
			}

			//cleans up
			ti_Close(subimgSlot);

		}
	}
	//free up source and output memory
	free(srcImg);
	free(outputImg);
	return 0;
}


/* Rebuilds the database of images on the calculator */
uint24_t rebuildDB() {
	char* var_name, * imgInfo[16];// nameBuffer[10];
	void* search_pos = NULL;
	uint24_t imagesFound{ 0 };
	ti_var_t database = ti_Open("HDPICDB", "w"), palette;
	ti_Write("HDDATV10", 8, 1, database);//Rewrites the header because w overwrites everything

	//resets splash screen for new loading bar
	drawSplashScreen();

	LoadingBar& loadingBar = LoadingBar::getInstance();
	loadingBar.resetLoadingBar(MAX_IMAGES);

	/*
	* Searches for palettes. This is a lot easier than searching for every single
	* subimage because there is guaranteed to only be one palette per image.
	* The palette contains all the useful information such as the image size and
	* the two letter ID for each appvar. This makes it easy to find every subimage via a loop.
	*/
	while ((var_name = ti_DetectVar(&search_pos, "HDPALV10", OS_TYPE_APPVAR)) != NULL) {

		//sets progress of how many images were found
		imagesFound++;
		loadingBar.increment();
		//finds the name, letter ID, and size of entire image this palette belongs to.
		palette = ti_Open(var_name, "r");
		//seeks past useless info
		ti_Seek(8, SEEK_CUR, palette);
		ti_Seek(16, SEEK_CUR, database);
		//reads the important info (16 bytes)
		ti_Read(&imgInfo, 16, 1, palette);
		//Writes the info to the database
		ti_Write(imgInfo, 16, 1, database);
		//closes palette for next iteration
		ti_Close(palette);
	}

	//closes the database
	ti_Close(database);
	gfx_End();
	ti_SetArchiveStatus(true, database);
	gfx_Begin();

	drawSplashScreen();
	dbg_sprintf(dbgout,"Pics Detected: %d",imagesFound);
	loadingBar.increment();
	return imagesFound;
}

//checks if the database is already created. If not, it creates it.
uint8_t isDatabaseReady() {
	char* var_name;
	void* search_pos = NULL;
	uint8_t exists{ 0 }, ready{ 0 };
	ti_var_t database;
	char myData[9]{ "HDDATV10" }; //remember have one more space than text you're saving for null termiation
	char compare[9]{ "HDDATV10" };
	//tries to find database using known header
	//todo: why var_name = ???
	while ((var_name = ti_DetectVar(&search_pos, myData, OS_TYPE_APPVAR)) != NULL) {
		exists = 1;
	}
	//if file already exists, simply return
	if (exists == 1)
		ready = 2;
	else {
		//if file doesn't already exist, create it.
		//creates the database appvar and writes the header. Checks if wrote successfully
		database = ti_Open("HDPICDB", "w");
		if (!database)
			ready = 3;
		if (ti_Write(&myData, 8, 1, database) != 1)
			ready = 4;
		if (ti_Rewind(database) == EOF)
			ready = 5;
		if (ti_Read(&myData, 8, 1, database) != 1)
			ready = 6;
		if (strcmp(myData, compare) != 0)
			ready = 7;
		else {
			ready = 1;
		}
		ti_Close(database);
	}


	//checks what happened
	if (ready == 1) {
		dbg_sprintf(dbgout, "\nDatabase Created");
		return 1;
	}
	else if (ready == 2) {
		dbg_sprintf(dbgout, "\nDatabase Already Exists");
		return 2;
	}
	else {
		dbg_sprintf(dbgout, "\nDatabase Failed to Create: %d\n", ready);
		gfx_SetTextFGColor(XLIBC_RED);
		PrintCenteredX("DB Failure! Please report:", 180);
		gfx_SetTextXY(120, 200);
		gfx_PrintUInt(ready, 1);
		return 0;
	}


}

/* This UI keeps the user selection in the middle of the screen. */
void drawMenu(int24_t startName, char* picNames, const int24_t numOfPics) {
	gfx_SetColor(PALETTE_WHITE);
	gfx_VertLine(140, 20, 200);
	
	int24_t yPxlPos{ 0 };

	//clears old text and sets prev for new text
	gfx_SetTextScale(2, 2);
	gfx_SetColor(PALETTE_BLACK);
	gfx_FillRectangle_NoClip(0, 0, 140, 240);
	gfx_SetColor(PALETTE_WHITE);
	gfx_SetTextFGColor(PALETTE_WHITE);
	gfx_SetTextBGColor(PALETTE_BLACK);
	
	//re-draws UI lines
	gfx_HorizLine_NoClip(0, 120, 6);
	gfx_HorizLine_NoClip(136, 120, 5);
	gfx_HorizLine_NoClip(6, 110, 130);
	gfx_HorizLine_NoClip(6, 130, 130);
	gfx_VertLine_NoClip(6, 110, 20);
	gfx_VertLine_NoClip(136, 110, 21);

	/* draw image names above selected name */
	dbg_sprintf(dbgout,"startName %d",startName);
	if (startName > 0){
		yPxlPos = Y_MARGIN+75;
		for (int24_t curName { startName-1}; (curName >= 0) && (yPxlPos > 15); curName--) {

			//calculates where the text should be drawn
			yPxlPos -= Y_SPACING;

			//Prints out the correct name
			gfx_PrintStringXY(&picNames[curName * BYTES_PER_IMAGE_NAME], X_MARGIN, yPxlPos);
		}
	}
	
	//display selected image name in center of screen
	yPxlPos = Y_MARGIN+75;
	gfx_PrintStringXY(&picNames[startName * BYTES_PER_IMAGE_NAME], X_MARGIN, yPxlPos);
	
	/* Draw image names below selected name.
	* Iterates until out of pics or about to draw off the screen */
	if(startName+1 < numOfPics){
		for (int24_t curName{ startName+1 }; (curName <= numOfPics) && (yPxlPos < 210); curName++) {
			//calculates where the text should be drawn
			yPxlPos += Y_SPACING;

			//Prints out the correct name
			gfx_PrintStringXY(&picNames[curName * BYTES_PER_IMAGE_NAME], X_MARGIN, yPxlPos);
		}
	}
	drawWatermark();
}

// divide and round up if necessary
// x cannot be 0
int24_t ceilDiv(int24_t x, int24_t y)
{
	
	return 1 + ((x - 1) / y);
}