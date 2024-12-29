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
#include <cstring>

#include "main.h"
#include "loadingBarHandler.h"
#include "pictureDatabase.h"
#include "globals.h"
#include "guiUtils.h"
#include "gfx/errorgfx.h"
#include "types/vector.h"


int main(void)
{
	gfx_Begin();
	gfx_SetTextTransparentColor(254);

	drawSplashScreen();
	if (findPictures() == 0)
	{
		drawNoImagesFound();
		while (!os_GetCSC());
		gfx_End();
		return 0;
	}
	else
	{
		//display the list of images
		drawHomeScreen();
		//quit
		gfx_End();
		return 0;
	}
}

// Display UI to select an image
void drawHomeScreen() {
	uint24_t startName{ 0 },
		desiredWidthInPxl{ MAX_THUMBNAIL_WIDTH }, desiredHeightInPxl{ MAX_THUMBNAIL_HEIGHT };
	int24_t xOffset{ 0 }, yOffset{ 0 };
	bool menuEnter, menuQuit,
		menuUp, menuDown, menuHelp,
		prev, next,
		deletePic, resetPic, redrawPic,
		zoomIn, zoomOut, zoomMax,
		panUp, panDown, panLeft, panRight;
	//set up variable that checks if drawImage failed
	uint8_t imageErr{ 0 };
	PicDatabase& picDB = PicDatabase::getInstance();
	
	/* main menu */
	gfx_FillScreen(PALETTE_BLACK);

	drawMenu(startName);

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
				PicDatabase& picDB = PicDatabase::getInstance();

				//convert subimg width to pixels width
				desiredWidthInPxl = picDB.getPicture(startName).numOfSubImagesHorizontal * SUBIMG_WIDTH_AND_HEIGHT;
				desiredHeightInPxl = picDB.getPicture(startName).numOfSubImagesVertical * SUBIMG_WIDTH_AND_HEIGHT;

				imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
					//if zooming back out didn't fix it, abort.
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Cant zoom in!!");

						PrintCenteredX("Error zooming in.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (!os_GetCSC());
						gfx_End();
						return;
					}
				}
			}
			
			//Plus key. Zoom in by double
			if (zoomIn && fullScreenImage) {
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomIn error"); }
				//doubles zoom
				desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
				desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom In\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
				imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
					//if zooming back out didn't fix it, abort.
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Cant zoom in!!");

						PrintCenteredX("Error zooming in.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (!os_GetCSC());
						gfx_End();
						return;
					}
				}
			}

			//subtract key. Zoom out by double.
			if (zoomOut && fullScreenImage) {
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom Out");
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomOut error"); }

				//ensure we can zoom out without desiredWidthInPxl or desiredHeightInPxl becoming 0
				if (desiredWidthInPxl / ZOOM_SCALE != 0 && desiredHeightInPxl / ZOOM_SCALE != 0) {
					//apply the zoom out to the width and height
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;

					//dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
					//this means we can't zoom out any more. Zoom back in.
					if (imageErr != 0) {
						//dbg_sprintf(dbgout, "\nCant zoom out trying zooming in...");
						desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
						desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
						//dbg_sprintf(dbgout, "\n Zoomed in\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);

						imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
						//if zooming back in didn't fix it, abort.
						if (imageErr != 0) {
							dbg_sprintf(dbgout, "\nERR: Cant zoom out!!");

							PrintCenteredX("Error zooming out.", 130);
							PrintCenteredX("Press any key to quit.", 140);
							while (!os_GetCSC());
							gfx_End();
							return;
						}
					}
				}
				else
				{
					//dbg_sprintf(dbgout, "\nmaxWidth or desiredHeightInPxl too small. \n Zoom out aborted.");
					//dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					//redraw the image. If it fails, I dunno why. It should be the exact same image as was previously displayed
					imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, true);
					//err handler
					if (imageErr != 0) {
						dbg_sprintf(dbgout, "\nERR: Issue displaying same image??");

						PrintCenteredX("Error with zoom.", 130);
						PrintCenteredX("Press any key to quit.", 140);
						while (kb_AnyKey() != 0); //wait for key lift
						while (!os_GetCSC());
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
				PicDatabase& picDB = PicDatabase::getInstance();
				picDB.deleteImage(startName);

				PrintCenteredX("Picture deleted.", 130);
				PrintCenteredX("Press any key.", 140);
				while (kb_AnyKey() != 0); //wait for key lift
				while (!os_GetCSC());

				//picture names will change. Delete what we currently have
				startName = 0;
				
				//set color for splash screen
				gfx_SetTextFGColor(XLIBC_GREY);
				gfx_SetTextBGColor(PALETTE_BLACK);
				
				//rebuild the database to account for the deleted image. 
				//picsCount = findPictures();

				//check if all images were deleted. If so, just quit.
				if (picDB.size() == 0) {
					drawNoImagesFound();
					while (kb_AnyKey() != 0); //wait for key lift
					while (!os_GetCSC());
					gfx_End();
					return;
				}

				//ensure text is readable
				gfx_SetTextFGColor(XLIBC_GREY);
				gfx_SetTextBGColor(PALETTE_BLACK);

				//re construct the GUI
				gfx_FillScreen(PALETTE_BLACK);
				resetPic=true;
				redrawPic = true;
			}

			/* Graph or down. Increases the name to start on and redraws the text */
			if (next || (menuDown &&  !fullScreenImage)) {
				startName++;
				//make sure user can't scroll down too far
				if (startName > picDB.size()-1)
				{
					startName = picDB.size()-1;
				}

				resetPic = fullScreenImage;
				redrawPic = true;
				errorID = 8; //if an error is thrown, then we've scrolled past the safety barrier somehow.
			}

			/* Y= or up. Decreases the name to start on and redraws the text */
			if (prev || (menuUp && !fullScreenImage)) {
				startName--;
				// Checks if selectedName underflowed. selectedName shouldn't be more than the max number of images possible.
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
					desiredWidthInPxl = LCD_WIDTH;
					desiredHeightInPxl = LCD_HEIGHT;
				}else{
					xOffset = 0;
					yOffset = 0;
					desiredWidthInPxl = MAX_THUMBNAIL_WIDTH;
					desiredHeightInPxl = MAX_THUMBNAIL_HEIGHT;
				}
				redrawPic = true;
			}
			
			// If necessary, draw the image with new settings.
			if (redrawPic) {
				if(!fullScreenImage)
				{
					drawMenu(startName);
				}
				while(kb_AnyKey()!=0); //wait for key lift
				imageErr = drawImage(startName, desiredWidthInPxl, desiredHeightInPxl, xOffset, yOffset, fullScreenImage);
				if (imageErr != 0) {
					PrintCenteredX("Error: ", 150);
					gfx_PrintUInt(errorID,3);
					PrintCenteredX("Press any key to quit.", 160);
					while (!os_GetCSC());
					gfx_End();
					return;
				}
			}
			if(!fullScreenImage)
				drawWatermark();
		}
	} while (!quitProgram);

}

/* Draws the image stored in database at position selectedName.
* Draws the image at location x,y starting at top left corner.
* If x=-1 then make image horizontally centered in the screen.
* If y=-1 then make image vertically centered on the screen.
* Image will automatically be resized to same aspect ratio so you just set the max width and height (4,3 will fit the screen normally)
* If successful, returns 0. Otherwise returns 1
*/
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen) {
	dbg_sprintf(dbgout, "\n\n--IMAGE CHANGE--");
	PicDatabase& picDB = PicDatabase::getInstance();

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


	//Converts the width/height from a char array into two integers by converting char into decimal value
	//then subtracting 48 to get the actual number.
	dbg_sprintf(dbgout, "\nimg width in subimg: %d \nimg height in subimg: %d\n", picDB.getPicture(picName).numOfSubImagesHorizontal, picDB.getPicture(picName).numOfSubImagesVertical);

	/*converts the char numbers from the header appvar into uint numbers
	(uint24_t)imgWH[?]-'0')*100 covers the 100's place
	(uint24_t)imgWH[?]-'0')*10 covers the 10's place
	(uint24_t)imgWH[?]-'0' covers the 1's place
	+1 accounts for 0 being the starting number
	*/
	uint24_t desiredWidthInSubimages{ (desiredWidthInPxl / SUBIMG_WIDTH_AND_HEIGHT) }; 
	uint24_t desiredHeightInSubimages{ (desiredHeightInPxl / SUBIMG_WIDTH_AND_HEIGHT) };
	dbg_sprintf(dbgout, "\n maxWS: %d\n widthS: %d\n maxHS: %d\n heightS: %d\n", desiredWidthInSubimages, picDB.getPicture(picName).numOfSubImagesHorizontal, desiredHeightInSubimages, picDB.getPicture(picName).numOfSubImagesVertical);

	//checks if it should scale an image horizontally or vertically.
	if((picDB.getPicture(picName).numOfSubImagesHorizontal * 80)/320 >= (picDB.getPicture(picName).numOfSubImagesVertical * 80)/240)
	{
		scaleNum = desiredWidthInSubimages;
		scaleDen = picDB.getPicture(picName).numOfSubImagesHorizontal;
		//dbg_sprintf(dbgout, "\nWidth too wide.");
	}
	else
	{
		scaleNum = desiredHeightInSubimages;
		scaleDen = picDB.getPicture(picName).numOfSubImagesVertical;
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
	char palName[9];
	sprintf(palName, "HP%.2s0000", picDB.getPicture(picName).ID);
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
	dbg_sprintf(dbgout, "\nwS: %d\nxO: %d", picDB.getPicture(picName).numOfSubImagesHorizontal, xOffset);
	dbg_sprintf(dbgout, "\nhS: %d\nyO: %d", picDB.getPicture(picName).numOfSubImagesVertical, yOffset);


	//This calculates the number of subimages you can fit in the screen horizontally
	//we know the horizontal resolution of the screen is 320px. 
	//We can get the width of each subimage by doing newSubimgWidthHeight/scaleDen
	//ceilDiv since we don't want missing subimages.
	int24_t rightMostSubimg{ ceilDiv(static_cast<int24_t>(LCD_WIDTH) , (newSubimgWidthHeight / scaleDen))+1 };
	//leftmost and topmost always starts at 0
	int24_t leftMostSubimg{ 0 };
	int24_t topMostSubimg { 0 };
	//This calculates the number of subimages you can fit in the screen vertically
	//we know the vertical resolution of the screen is 240px. 
	//We can get the width of each subimages by doing newSubimgWidthHeight/scaleDen
	//ceilDiv since we don't want missing subimages. (Overflow is compensated for, if necessary, below)
	int24_t bottomMostSubimg{ ceilDiv(static_cast<int24_t>(LCD_HEIGHT) , (newSubimgWidthHeight / scaleDen)) };


	/* Apply pan offsets */
	//if we're panning horizontally, shift the rightmost and leftmost subimages (xOffset is negative in this case)
	rightMostSubimg -= xOffset;
	leftMostSubimg -= xOffset;
	//if we're panning vertically, shift the topmost and bottommost subimages (yOffset is negative in this case)
	bottomMostSubimg += yOffset;
	topMostSubimg += yOffset;


	/* Ensure we don't try to display more subimages than exist */
	if (rightMostSubimg > picDB.getPicture(picName).numOfSubImagesHorizontal)
		rightMostSubimg = picDB.getPicture(picName).numOfSubImagesHorizontal;
	if (leftMostSubimg < 0)
		leftMostSubimg = 0;
	if (bottomMostSubimg > picDB.getPicture(picName).numOfSubImagesVertical)
		bottomMostSubimg = picDB.getPicture(picName).numOfSubImagesVertical;
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
			char picAppvarToFind[9];
			sprintf(picAppvarToFind, "%.2s%03u%03u", picDB.getPicture(picName).ID, xSubimgID, ySubimgID);
			//dbg_sprintf(dbgout, "\n%.2s%03u%03u", picDB.getPicture(picName).ID, xSubimgID, ySubimgID);
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
uint24_t findPictures() {
	char* var_name, imgInfo[16];
	void* search_pos = NULL;
	uint24_t imagesFound{ 0 };

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

	while ((var_name = ti_DetectVar(&search_pos, "HDPALV10", OS_TYPE_APPVAR)) != NULL)
	{
		imagesFound++;
		loadingBar.increment();
	}

	PicDatabase& picDB = PicDatabase::getInstance();
	picDB.reserve(imagesFound);

	loadingBar.resetLoadingBar(imagesFound);
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, "HDPALV10", OS_TYPE_APPVAR)) != NULL) {
		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t HORIZ_VERT_SIZE{ 3 };
		constexpr uint8_t PALETTE_NAME_SIZE{ 8 };
		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };
		constexpr uint8_t HEADER_SIZE{ 16 };
		
		loadingBar.increment();

		imageData imgData;
		//sets progress of how many images were found
		//finds the name, letter ID, and size of entire image this palette belongs to.
		ti_var_t  palette;
		palette = ti_Open(var_name, "r");
		//seeks past HDPALV10
		ti_Seek(8, SEEK_CUR, palette);
		//reads the important info (16 bytes)
		//e.g. poppy___JT003002
		ti_Read(&imgInfo, HEADER_SIZE, 1, palette);

		char charArrImgInfo[16];
		std::strncpy(charArrImgInfo, imgInfo, HEADER_SIZE);
		std::strncpy(imgData.palletName, var_name, PALETTE_NAME_SIZE);
		std::strncpy(imgData.imgName, charArrImgInfo, IMAGE_NAME_SIZE);
		std::strncpy(imgData.ID, charArrImgInfo + IMAGE_NAME_SIZE, ID_SIZE);

		imgData.imgName[8] = '\0';
		imgData.palletName[8] = '\0';
		imgData.ID[2] = '\0';

		// Get width of whole image. Then convert the number from a char representation to a int24_t
		char buffer[3];
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE, HORIZ_VERT_SIZE );
		imgData.numOfSubImagesHorizontal = (((static_cast<int24_t>(buffer[0]) - '0') * 100 + (static_cast<int24_t>(buffer[1]) - '0') * 10 + static_cast<int24_t>(buffer[2]) - '0') + 1);
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE + HORIZ_VERT_SIZE, HORIZ_VERT_SIZE);
		imgData.numOfSubImagesVertical = (((static_cast<int24_t>(buffer[0]) - '0') * 100 + (static_cast<int24_t>(buffer[1]) - '0') * 10 + static_cast<int24_t>(buffer[2]) - '0') + 1);

		dbg_sprintf(dbgout,"\nPicture found:\n imgName: %.8s\n palletName: %.8s\n ID: %.2s\n subImgHoriz: %d\n subImgVert: %d\n", imgData.imgName, imgData.palletName, imgData.ID, imgData.numOfSubImagesHorizontal, imgData.numOfSubImagesVertical);

		picDB.addPicture(imgData);

		//closes palette for next iteration
		ti_Close(palette);


	}

	drawSplashScreen();
	dbg_sprintf(dbgout,"\nPics Detected: %d",imagesFound);
	loadingBar.increment();
	return imagesFound;
}


/* This UI keeps the user selection in the middle of the screen. */
void drawMenu(uint24_t selectedName) {
	gfx_SetColor(PALETTE_WHITE);
	gfx_VertLine(140, 20, 200);
	
	uint24_t yPxlPos{ 0 };

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

	PicDatabase& picDB = PicDatabase::getInstance();

	/* draw image names above selected name */
	dbg_sprintf(dbgout,"\nselectedName %d",selectedName);
	if (selectedName > 0){
		yPxlPos = Y_MARGIN+75; 
		for (uint24_t curImg { selectedName-1 }; (curImg < MAX_UINT) && (yPxlPos > 15); curImg--) {
			//calculates where the text should be drawn
			yPxlPos -= Y_SPACING;

			dbg_sprintf(dbgout, "\ncurImg: %d", curImg);
			//Prints out the correct name
			gfx_PrintStringXY(picDB.getPicture(curImg).imgName, X_MARGIN, yPxlPos);
		}
	}
	
	//display selected image name in center of screen
	yPxlPos = Y_MARGIN+75;
	gfx_PrintStringXY(picDB.getPicture(selectedName).imgName, X_MARGIN, yPxlPos);
	
	/* Draw image names below selected name.
	* Iterates until out of pics or about to draw off the screen */
	if(selectedName+1 < picDB.size()){
		for (uint24_t curName{ selectedName+1 }; (curName < picDB.size()) && (yPxlPos < 210); curName++) {
			//calculates where the text should be drawn
			yPxlPos += Y_SPACING;

			//Prints out the correct name
			gfx_PrintStringXY(picDB.getPicture(curName).imgName, X_MARGIN, yPxlPos);
		}
	}
	drawWatermark();
}

// divide and round up if necessary
int24_t ceilDiv(int24_t x, int24_t y)
{
	return (x + y - 1) / y;
}