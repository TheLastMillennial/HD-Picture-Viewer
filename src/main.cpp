/*HD Picture Viewer
* By TheLastMillennial
* https://github.com/TheLastMillennial/HD-Picture-Viewer
* Build With:    ./make debug --directory="path/to/HD-Picture-Viewer"
*/

#include <tice.h>
#include <graphx.h>
#include <string.h>
#include <fileioc.h>
#include <debug.h>
#include <compression.h>
#include <cstring>
#include <cmath>

#include "main.h"
#include "loadingBarHandler.h"
#include "keyPressHandler.h"
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
	if (findPictures() == 0) {
		drawNoImagesFound();
		while (!os_GetCSC());
		gfx_End();
		return 0;
	}
	else {
		//display the list of images
		drawHomeScreen();
		//quit
		gfx_End();
		kb_ClearOnLatch();
		return 0;
	}
}



// Display UI to select an image
void drawHomeScreen()
{
	uint24_t selectedPicIndex{ 0 },
		desiredWidthInPxl{ MAX_THUMBNAIL_WIDTH }, desiredHeightInPxl{ MAX_THUMBNAIL_HEIGHT };

	//set up variable that checks if drawImage failed
	uint8_t imageErr{ 0 };
	PicDatabase &picDB = PicDatabase::getInstance();
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();


	/* main menu */
	gfx_FillScreen(PALETTE_BLACK);
	drawMenu(selectedPicIndex);

	//thumbnail
	drawImage(selectedPicIndex, 180, 120, false);

	/* UI */
	bool quitProgram{ false };
	uint8_t errorID = 0;	

	do {
		static bool fullScreenImage{ false };
		bool resetPic{ false }, redrawPic{ false };

		// Pressing on means halt immediately.
		if (kb_On) {
			kb_ClearOnLatch();
			keyHandler.reset();
			dbg_sprintf(dbgout, "\nRender aborted by ON.");
			PrintCenteredX("Render Interrupted.", 10);
			PrintCenteredX("Press any key to continue.", 215);
			while (!os_GetCSC()); //wait for key press
		}

		//scans the keys for existing keypress.
		if (!keyHandler.isAnyKeyPressed()) {
			//no existing key press. Check for new keypress.
			if (!keyHandler.scanKeys(fullScreenImage)) {
				continue; // no new keypress. There is nothing to do.
			}
			else {
				dbg_sprintf(dbgout, "\nNew Key press");
			}
		}
		else {
			dbg_sprintf(dbgout, "\nExisting key press");
		}
		// Key press detected


			// clear. Go back.
		if (keyHandler.wasKeyPressed(kb_KeyClear)) {
			//If we're viewing an image, exit to menu. If we're already on menu, quit program.
			if (fullScreenImage) {
				fullScreenImage = false;
				resetPic = true;
				redrawPic = true;
				errorID = 1;
				gfx_FillScreen(PALETTE_BLACK);
			}
			else {
				quitProgram = true;
				break;
			}
		}

		// enter. Fullscreen image
		if (keyHandler.wasKeyPressed(kb_KeyEnter)) {
			if (!fullScreenImage)
				resetPic = true;
			fullScreenImage = true;
			redrawPic = true;
			errorID = 2;
		}

		// mode. Show help.
		if (keyHandler.wasKeyPressed(kb_KeyMode)) {
			drawHelp();
			KeyPressHandler::waitForAnyKey();
			gfx_FillScreen(PALETTE_BLACK);
			resetPic = true;
			redrawPic = true;
		}


		//Delete. delete all appvars related to current image
		if (keyHandler.wasKeyPressed(kb_KeyDel)) {
			//the current palette is about to be deleted. Set the default palette
			gfx_SetDefaultPalette(gfx_8bpp);
			//we don't want the user seeing the horrors of their image with the wrong palette
			gfx_FillScreen(PALETTE_BLACK);
			gfx_SetTextFGColor(XLIBC_GREY);
			gfx_SetTextBGColor(PALETTE_BLACK);
			gfx_SetTextScale(1, 1);
			PrintCenteredX("Deleting Picture...", 120);

			//delete the palette and all subimages
			PicDatabase &picDB = PicDatabase::getInstance();
			picDB.deleteImage(selectedPicIndex);

			PrintCenteredX("Picture deleted.", 130);
			PrintCenteredX("Press any key.", 140);
			KeyPressHandler::waitForAnyKey();

			//set color for splash screen
			gfx_SetTextFGColor(XLIBC_GREY);
			gfx_SetTextBGColor(PALETTE_BLACK);

			//check if all images were deleted. If so, just quit.
			if (picDB.size() == 0) {
				drawNoImagesFound();
				KeyPressHandler::waitForAnyKey();
				gfx_End();
				return;
			}

			//select next picture
			if (selectedPicIndex > 0) {
				selectedPicIndex--;
			}

			//ensure text is readable
			//re construct the GUI
			gfx_SetTextFGColor(XLIBC_GREY);
			gfx_SetTextBGColor(PALETTE_BLACK);
			gfx_FillScreen(PALETTE_BLACK);
			resetPic = true;
			redrawPic = true;
		}

		/* Graph or down. Increases the name to start on and redraws the text */
		if (keyHandler.wasKeyPressed(kb_KeyGraph) || (keyHandler.wasKeyPressed(kb_KeyDown) && !fullScreenImage)) {
			selectedPicIndex++;
			//make sure user can't scroll down too far
			if (selectedPicIndex > picDB.size() - 1) {
				dbg_sprintf(dbgout, "\ntoo high %d -> 0", selectedPicIndex);

				selectedPicIndex = 0;
			}

			resetPic = fullScreenImage;
			redrawPic = true;
			errorID = 8; //if an error is thrown, then we've scrolled past the safety barrier somehow.

		}

		/* Y= or up. Decreases the name to start on and redraws the text */
		if (keyHandler.wasKeyPressed(kb_KeyYequ) || (keyHandler.wasKeyPressed(kb_KeyUp) && !fullScreenImage)) {
			selectedPicIndex--;
			// Checks if selectedName underflowed. selectedName shouldn't be more than the max number of images possible.
			if (selectedPicIndex > MAX_IMAGES) {
				dbg_sprintf(dbgout, "\nunderflow: %d -> %d", selectedPicIndex, (picDB.size() - 1));

				selectedPicIndex = picDB.size() - 1;
			}
			resetPic = fullScreenImage;
			redrawPic = true;
			errorID = 9; //if an error is thrown, then we've scrolled past the safety barrier somehow.
		}

		//left, right, up, down. Image panning.
		if (fullScreenImage) {
			if (keyHandler.wasKeyPressed(kb_KeyLeft)) {
				errorID = 3;
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyRight)) {
				errorID = 4;
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, -1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyUp)) {
				errorID = 5;
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, -1);
			}
			if (keyHandler.wasKeyPressed(kb_KeyDown)) {
				errorID = 6;
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, 1);
			}

			if (imageErr != 0) {
				PrintCenteredX("Error: ", 150);
				gfx_PrintUInt(errorID, 3);
				PrintCenteredX("Press any key to quit.", 160);
				while (!os_GetCSC());
				gfx_End();
				return;
			}


			//Zoom key. Zoom in as far as possible while maintaining full quality
			if (keyHandler.wasKeyPressed(kb_KeyZoom)) {
				//pull image full dimensions from database
				PicDatabase &picDB = PicDatabase::getInstance();

				//convert subimg width to pixels width
				desiredWidthInPxl = picDB.getPicture(selectedPicIndex).horizSubImages * SUBIMAGE_DIMENSIONS;
				desiredHeightInPxl = picDB.getPicture(selectedPicIndex).vertSubImages * SUBIMAGE_DIMENSIONS;

				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
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
			if (keyHandler.wasKeyPressed(kb_KeyAdd)) {
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomIn error"); }
				//doubles zoom
				desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
				desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom In\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					dbg_sprintf(dbgout, "\nCant zoom in trying zooming out...");
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;
					dbg_sprintf(dbgout, "\n Zoomed out\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
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
			if (keyHandler.wasKeyPressed(kb_KeySub)) {
				//dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom Out");
				if (imageErr != 0) { dbg_sprintf(dbgout, "\npre-zoomOut error"); }

				//ensure we can zoom out without desiredWidthInPxl or desiredHeightInPxl becoming 0
				if (desiredWidthInPxl / ZOOM_SCALE != 0 && desiredHeightInPxl / ZOOM_SCALE != 0) {
					//apply the zoom out to the width and height
					desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
					desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;

					//dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
					//this means we can't zoom out any more. Zoom back in.
					if (imageErr != 0) {
						//dbg_sprintf(dbgout, "\nCant zoom out trying zooming in...");
						desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
						desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
						//dbg_sprintf(dbgout, "\n Zoomed in\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);

						imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
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
				else {
					//dbg_sprintf(dbgout, "\nmaxWidth or desiredHeightInPxl too small. \n Zoom out aborted.");
					//dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					//redraw the image. If it fails, I dunno why. It should be the exact same image as was previously displayed
					imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
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
		}

		//Window. Reset zoom and pan
		if (resetPic || keyHandler.wasKeyPressed(kb_KeyWindow)) {
			if (fullScreenImage) {

				picDB.getPicture(selectedPicIndex).xOffset = 0;
				picDB.getPicture(selectedPicIndex).yOffset = 0;

				desiredWidthInPxl = LCD_WIDTH;
				desiredHeightInPxl = LCD_HEIGHT;
			}
			else {
				picDB.getPicture(selectedPicIndex).xOffset = 0;
				picDB.getPicture(selectedPicIndex).yOffset = 0;
				desiredWidthInPxl = MAX_THUMBNAIL_WIDTH;
				desiredHeightInPxl = MAX_THUMBNAIL_HEIGHT;
			}
			redrawPic = true;
		}

		// If necessary, draw the image with new settings.
		if (redrawPic) {
			dbg_sprintf(dbgout, "\nredrawPic Call");

			if (!fullScreenImage) {
				drawMenu(selectedPicIndex);
			}
			keyHandler.reset();
			imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, fullScreenImage);
			if (imageErr != 0) {
				PrintCenteredX("Error: ", 150);
				gfx_PrintUInt(errorID, 3);
				PrintCenteredX("Press any key to quit.", 160);
				while (!os_GetCSC());
				gfx_End();
				return;
			}
		}

		if (!fullScreenImage)
			drawWatermark();


	} while (!quitProgram);

}

/* Draws the image stored in database at position selectedName.
* Draws the image at location x,y starting at top left corner.
* If x=-1 then make image horizontally centered in the screen.
* If y=-1 then make image vertically centered on the screen.
* Image will automatically be resized to same aspect ratio so you just set the max width and height (4,3 will fit the screen normally)
* If successful, returns 0. Otherwise returns 1
*/
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool fullScreenPic, int8_t shiftX, int8_t shiftY)
{
	dbg_sprintf(dbgout, "\n\n--DRAW IMAGE--");
	PicDatabase &picDB = PicDatabase::getInstance();
	imageData const &curPicture = picDB.getPicture(picName);
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();


	int24_t scaleNumerator{ 1 }, scaleDenominator{ 1 }, subimgNewDimNumerator{ 0 };
	if (!fullScreenPic) {
		//used for thumbnails
		gfx_SetColor(PALETTE_BLACK);
		gfx_FillRectangle_NoClip(150, 0, 170, 240);
	}


	//Converts the width/height from a char array into two integers by converting char into decimal value
	//then subtracting 48 to get the actual number.
	//dbg_sprintf(dbgout, "\nimg width in subimg: %d \nimg height in subimg: %d\n", curPicture.horizSubImages, curPicture.vertSubImages);

	/*converts the char numbers from the header appvar into uint numbers
	(uint24_t)imgWH[?]-'0')*100 covers the 100's place
	(uint24_t)imgWH[?]-'0')*10 covers the 10's place
	(uint24_t)imgWH[?]-'0' covers the 1's place
	+1 accounts for 0 being the starting number
	*/
	uint24_t desiredWidthInSubimages{ (desiredWidthInPxl / SUBIMAGE_DIMENSIONS) };
	uint24_t desiredHeightInSubimages{ (desiredHeightInPxl / SUBIMAGE_DIMENSIONS) };
	//dbg_sprintf(dbgout, "\n maxWS: %d\n widthS: %d\n maxHS: %d\n heightS: %d\n",
	//	desiredWidthInSubimages, curPicture.horizSubImages, desiredHeightInSubimages, curPicture.vertSubImages);

	//checks if it should scale an image horizontally or vertically.
	if ((curPicture.horizSubImages * SUBIMAGE_DIMENSIONS) / LCD_WIDTH >= (curPicture.vertSubImages * SUBIMAGE_DIMENSIONS) / LCD_HEIGHT) {
		//width too wide
		scaleNumerator = desiredWidthInSubimages;
		scaleDenominator = curPicture.horizSubImages;
	}
	else {
		// Height too tall
		scaleNumerator = desiredHeightInSubimages;
		scaleDenominator = curPicture.vertSubImages;
	}

	// Check for invalid fractions
	if (scaleNumerator == 0 || scaleDenominator == 0) {
		dbg_sprintf(dbgout, "\nERR: Cant zoom out\n scaleNumerator:%d\n scaleDenominator:%d", scaleNumerator, scaleDenominator);
		return 1;
	}

	/*
	[jacobly] so now whenever we want to compute
	`x * scale`
	we instead want to compute
	`x * (scaleNumerator / scaleDenominator)`
	which can now be reordered to use strictly integer math as
	`(x * scaleNumerator) / scaleDenominator`

	[jacobly] if you don't know:
	floorDiv(x, y) := x / y;
	roundDiv(x, y) := (x + (y / 2)) / y;
	ceilDiv(x, y) := (x + y - 1) / y;
	[MateoC] huh I didn't know about roundDiv
	*/

	//dbg_sprintf(dbgout, "\n newWH: %d \n ScaleNum: %d \n scaleDenominator: %d \n xOffset: %d \n yOffset %d", subimgNewDimNumerator, scaleNumerator, scaleDenominator, curPicture.xOffset, curPicture.yOffset);

	//pointer to memory where each unsized subimage will be stored
	gfx_sprite_t *srcImg{ gfx_MallocSprite(SUBIMAGE_DIMENSIONS, SUBIMAGE_DIMENSIONS) };
	if (!srcImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate src memory!");
		return 1;
	}
	//Final dimension of all subimages
	subimgNewDimNumerator = SUBIMAGE_DIMENSIONS * scaleNumerator;
	int24_t subimgScaledDim{ subimgNewDimNumerator / scaleDenominator };

	//ensure the resized subimage will fit within the dimensions of the screen.
	if (subimgScaledDim > LCD_HEIGHT) {
		dbg_sprintf(dbgout, "\nERR: Subimage will be too large: %d", subimgScaledDim);
		free(srcImg);
		return 1;
	}
	//allocates memory for resized image
	gfx_sprite_t *outputImg{ gfx_MallocSprite(subimgScaledDim,subimgScaledDim) };
	if (!outputImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate output memory!");
		free(srcImg);
		return 1;
	}

	//sets correct palettes
	char palName[9];
	sprintf(palName, "HP%.2s0000", curPicture.ID);
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


	/* Apply Offset */

	// Which direction to draw the subimages. 
	// By default it's the percieved most performant option
	// If the image gets panned, it prioritizes filling in the missing subimages first.
	bool bReverseDirection{ false }; //false means the image initially draws slow then speeds up.
	bool bDrawVertical{ true }; //true: draws columns. False: draws rows.

	picDB.getPicture(picName).xOffset += shiftX;
	picDB.getPicture(picName).yOffset += shiftY;

	//Check if we need to pan the image. If so, shift the contents of the screen over so we don't need to redraw as many subimages.
	if (shiftX != 0 || shiftY != 0) {
		gfx_SetDrawBuffer();
		gfx_FillScreen(0);
		// Shift screen to right
		if (shiftX > 0) {
			bReverseDirection = true;
			bDrawVertical = true;
			gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, 0, subimgScaledDim, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
		}
		// Shift screen to left
		if (shiftX < 0) {
			bReverseDirection = false;
			bDrawVertical = true;
			gfx_CopyRectangle(gfx_screen, gfx_buffer, subimgScaledDim, 0, 0, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
		}
		// Shift screen up
		if (shiftY > 0) {
			bReverseDirection = false;
			bDrawVertical = false;
			gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, subimgScaledDim, 0, 0, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
		}
		// Shift screen down
		if (shiftY < 0) {
			bReverseDirection = true;
			bDrawVertical = false;
			gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, 0, 0, subimgScaledDim, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
		}

		gfx_BlitBuffer();
		gfx_SetDrawScreen();
	}
	else if (fullScreenPic) {
		//If there's no panning, then we need to re-draw the entire image. 
		gfx_FillScreen(PALETTE_BLACK);
	}


	//Displays all the images
	dbg_sprintf(dbgout, "\n-------------------------");
	//dbg_sprintf(dbgout, "\nwS: %d\nxO: %d", curPicture.horizSubImages, curPicture.xOffset);
	//dbg_sprintf(dbgout, "\nhS: %d\nyO: %d", curPicture.vertSubImages, curPicture.yOffset);


	//This calculates the number of subimages you can fit in the screen horizontally
	//we know the horizontal resolution of the screen is 320px. 
	//We can get the width of each subimage by doing subimgNewDimNumerator/scaleDenominator
	//ceilDiv since we don't want missing subimages.
	int24_t rightMostSubimg{ ceilDiv(static_cast<int24_t>(LCD_WIDTH) , (subimgNewDimNumerator / scaleDenominator)) + 1 };
	//leftmost and topmost always starts at 0
	int24_t leftMostSubimg{ 0 };
	int24_t topMostSubimg{ 0 };
	//This calculates the number of subimages you can fit in the screen vertically
	//we know the vertical resolution of the screen is 240px. 
	//We can get the width of each subimages by doing subimgNewDimNumerator/scaleDenominator
	//ceilDiv since we don't want missing subimages. (Overflow is compensated for, if necessary, below)
	int24_t bottomMostSubimg{ ceilDiv(static_cast<int24_t>(LCD_HEIGHT) , (subimgNewDimNumerator / scaleDenominator)) };


	/* Apply pan offsets */
	//if we're panning horizontally, shift the rightmost and leftmost subimages (xOffset is negative in this case)
	rightMostSubimg -= curPicture.xOffset;
	leftMostSubimg -= curPicture.xOffset;
	//if we're panning vertically, shift the topmost and bottommost subimages (yOffset is negative in this case)
	bottomMostSubimg += curPicture.yOffset;
	topMostSubimg += curPicture.yOffset;


	/* Ensure we don't try to display more subimages than exist */
	if (rightMostSubimg > curPicture.horizSubImages)
		rightMostSubimg = curPicture.horizSubImages;
	if (leftMostSubimg < 0)
		leftMostSubimg = 0;
	if (bottomMostSubimg > curPicture.vertSubImages)
		bottomMostSubimg = curPicture.vertSubImages;
	if (topMostSubimg < 0)
		topMostSubimg = 0;
	
	//dbg_sprintf(dbgout, "\nrightMost %d\nLeftMost %d", rightMostSubimg, leftMostSubimg);
	//dbg_sprintf(dbgout, "\ntopMost %d\nbottomMost %d", topMostSubimg, bottomMostSubimg);

	/* Display final image */

	//the -1 is to account for both the
	//the +1 is to prevent underflow which would cause an infinite loop
	//this for loop outputs pic right to left, top to bottom
	const int24_t xFirstID{ (leftMostSubimg) }, xLastID{ (rightMostSubimg)-1 };
	const int24_t yFirstID{ (topMostSubimg) }, yLastID{ (bottomMostSubimg)-1 };

	const uint24_t thumbnailOffsetX = fullScreenPic ? 0 : 150;
	const uint24_t thumbnailOffsetY = fullScreenPic ? 0 : ((240 - (subimgScaledDim * bottomMostSubimg)) / 2);

	dbg_sprintf(dbgout, "\n Image Name: %s\n x-range: %d - %d\n y-range: %d - %d", curPicture.imgName, xFirstID, xLastID, yFirstID, yLastID);

	int24_t xSubimgID{ 0 };
	int24_t ySubimgID{ 0 };

	// Loop through all subimages to create full image
	bool bFirstRun{ true };
	while (iterate(xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID, bDrawVertical, bReverseDirection, bFirstRun)) {
		
		
		const uint24_t subimgPxlPosX{ thumbnailOffsetX + static_cast<uint24_t>((xSubimgID + curPicture.xOffset) * (subimgNewDimNumerator / scaleDenominator)) };
		const uint24_t subimgPxlPosY{ thumbnailOffsetY + static_cast<uint24_t>((ySubimgID - curPicture.yOffset) * (subimgNewDimNumerator / scaleDenominator)) };

		//dbg_sprintf(dbgout, "\nLooped.\n xSubimgID: %d @ %d pxl \n ySubimgID: %d @ %d pxl",xSubimgID, subimgPxlPosX, ySubimgID, subimgPxlPosY);


		//a key interrupted output. Quit immediately
		if (keyHandler.scanKeys(fullScreenPic)) {
			dbg_sprintf(dbgout, "\nRender aborted!\n");
			//free up source and output memory
			free(srcImg);
			free(outputImg);
			return 0;
		}

		if (subimgPxlPosX > LCD_WIDTH || subimgPxlPosX < 0 || subimgPxlPosY > LCD_HEIGHT || subimgPxlPosY < 0)
			continue;

		//combines the separate parts into one name to search for
		char picAppvarToFind[9];
		sprintf(picAppvarToFind, "%.2s%03u%03u", curPicture.ID, xSubimgID, ySubimgID);
		//dbg_sprintf(dbgout, "\n%.2s%03u%03u @ %d x %d", curPicture.ID, xSubimgID, ySubimgID, subimgPxlPosX, subimgPxlPosY);
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
			if (subimgPxlPosX < 0 || subimgPxlPosX + subimgScaledDim > LCD_WIDTH || subimgPxlPosY < 0 || subimgPxlPosY + subimgScaledDim > LCD_HEIGHT)

				//if (xSubimgID >= xLastID -1 || ySubimgID >= yLastID -1 || xSubimgID <= xFirstID +1 || ySubimgID <= yFirstID +1) 
			{
				//dbg_sprintf(dbgout, "\n\nCLIPPED");
				gfx_Sprite(outputImg, subimgPxlPosX, subimgPxlPosY);
			}
			else {
				//dbg_sprintf(dbgout, "\nnoclip");
				gfx_Sprite_NoClip(outputImg, subimgPxlPosX, subimgPxlPosY);
			}
		}
		else {
			//subimage does not exist, display error image
			dbg_sprintf(dbgout, "\nERR: Subimage doesn't exist!");
			dbg_sprintf(dbgout, "\n %s", picAppvarToFind);
			//dbg_sprintf(dbgout,"\nERR: \nxsubimage: %d \subimgNewDimNumerator: %d \nscaleDen: %d",xSubimage,subimgNewDimNumerator,scaleDenominator);
			zx7_Decompress(srcImg, errorTriangle_compressed);
			//resizes it to outputImg size
			gfx_ScaleSprite(srcImg, outputImg);
			//displays the output image
			//dbg_sprintf(dbgout,"\nxsubimage: %d \subimgNewDimNumerator: %d \nscaleDen: %d\n",xSubimage,subimgNewDimNumerator,scaleDenominator);
			gfx_Sprite(outputImg, subimgPxlPosX, subimgPxlPosY);
			//while(!os_GetCSC());
			continue;
		}

		//cleans up
		ti_Close(subimgSlot);

	}
	//free up source and output memory
	free(srcImg);
	free(outputImg);
	dbg_sprintf(dbgout, "\nDraw Finished.\n");
	return 0;
}


/* Rebuilds the database of images on the calculator */
uint24_t findPictures()
{
	char *var_name, imgInfo[16];
	void *search_pos = NULL;
	uint24_t imagesFound{ 0 };

	//resets splash screen for new loading bar
	drawSplashScreen();

	LoadingBar &loadingBar = LoadingBar::getInstance();
	loadingBar.resetLoadingBar(MAX_IMAGES);
	/*
	* Searches for palettes. This is a lot easier than searching for every single
	* subimage because there is guaranteed to only be one palette per image.
	* The palette contains all the useful information such as the image size and
	* the two letter ID for each appvar. This makes it easy to find every subimage via a loop.
	*/

	while ((var_name = ti_DetectVar(&search_pos, "HDPALV10", OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}

	PicDatabase &picDB = PicDatabase::getInstance();
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
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE, HORIZ_VERT_SIZE);
		imgData.horizSubImages = (((static_cast<int24_t>(buffer[0]) - '0') * 100 + (static_cast<int24_t>(buffer[1]) - '0') * 10 + static_cast<int24_t>(buffer[2]) - '0') + 1);
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE + HORIZ_VERT_SIZE, HORIZ_VERT_SIZE);
		imgData.vertSubImages = (((static_cast<int24_t>(buffer[0]) - '0') * 100 + (static_cast<int24_t>(buffer[1]) - '0') * 10 + static_cast<int24_t>(buffer[2]) - '0') + 1);

		dbg_sprintf(dbgout, "\nPicture found:\n imgName: %.8s\n palletName: %.8s\n ID: %.2s\n subImgHoriz: %d\n subImgVert: %d\n", imgData.imgName, imgData.palletName, imgData.ID, imgData.horizSubImages, imgData.vertSubImages);

		picDB.addPicture(imgData);

		//closes palette for next iteration
		ti_Close(palette);


	}

	drawSplashScreen();
	dbg_sprintf(dbgout, "\nPics Detected: %d", imagesFound);
	loadingBar.increment();
	return imagesFound;
}


/* This UI keeps the user selection in the middle of the screen. */
void drawMenu(uint24_t selectedName)
{
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

	PicDatabase &picDB = PicDatabase::getInstance();

	/* draw image names above selected name */
	dbg_sprintf(dbgout, "\nselectedName %d", selectedName);
	if (selectedName > 0) {
		yPxlPos = Y_MARGIN + 75;
		for (uint24_t curImg{ selectedName - 1 }; (curImg < MAX_UINT) && (yPxlPos > 15); curImg--) {
			//calculates where the text should be drawn
			yPxlPos -= Y_SPACING;

			dbg_sprintf(dbgout, "\ncurImg: %d", curImg);
			//Prints out the correct name
			gfx_PrintStringXY(picDB.getPicture(curImg).imgName, X_MARGIN, yPxlPos);
		}
	}

	//display selected image name in center of screen
	yPxlPos = Y_MARGIN + 75;
	gfx_PrintStringXY(picDB.getPicture(selectedName).imgName, X_MARGIN, yPxlPos);

	/* Draw image names below selected name.
	* Iterates until out of pics or about to draw off the screen */
	if (selectedName + 1 < picDB.size()) {
		for (uint24_t curName{ selectedName + 1 }; (curName < picDB.size()) && (yPxlPos < 210); curName++) {
			//calculates where the text should be drawn
			yPxlPos += Y_SPACING;

			//Prints out the correct name
			gfx_PrintStringXY(picDB.getPicture(curName).imgName, X_MARGIN, yPxlPos);
		}
	}
	drawWatermark();
}

// Allows iterating a 2D grid in multiple different directions.
// Returns true as long as there is still iterating to do.
// Returns false when finished iterating.
// Directions:
// column then row (bDrawVertically = true)
// row then column (bDrawVertically = false)
// bDrawOppositeSide switches which side the function starts drawing on
// xSubimgID and/or ySubimgID will be modified with the next x and y coordinate.
// xFirstID and xLastID are the bounds for xSubimgID
// yFirstID and yLastID are the bounds for ySubimgID
// bFirstRun MUST be set to true. It will be changed to false after the first iteration.
bool iterate(int24_t &xSubimgID, int24_t const &xFirstID, int24_t const &xLastID, int24_t &ySubimgID, int24_t const &yFirstID, int24_t const &yLastID, bool bDrawVertically, bool bDrawOppositeSideFirst, bool & bFirstRun)
{
	if (bDrawVertically) {
		if (bDrawOppositeSideFirst) {
			//draw columns starting at bottom left
			if (bFirstRun) {
				xSubimgID = xFirstID;
				ySubimgID = yLastID;
				bFirstRun = false;
				return true;
			}
			if (--ySubimgID < yFirstID) {
				ySubimgID = yLastID;
				if (++xSubimgID > xLastID) {
					bFirstRun = true;
					return false;
				}
			}
			//dbg_sprintf(dbgout, "\n1. xSubimgID %d : ( %d - %d ) \n   ySubimgID %d : ( %d - %d )", xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID);
			return true;
		}
		else {
			//draw columns starting at bottom right
			if (bFirstRun) {
				xSubimgID = xLastID;
				ySubimgID = yLastID;
				bFirstRun = false;
				return true;
			}
			if (--ySubimgID < yFirstID) {
				ySubimgID = yLastID;
				if (--xSubimgID < xFirstID) {
					bFirstRun = true;
					return false;
				}
			}
			//dbg_sprintf(dbgout, "\n2. xSubimgID %d : ( %d - %d ) \n   ySubimgID %d : ( %d - %d )", xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID);
			return true;
		}
	}
	else {
		if (bDrawOppositeSideFirst) //todo: top left corner not drawn sometimes
		{
			//draw rows starting at top right
			if (bFirstRun) {
				ySubimgID = yFirstID;
				xSubimgID = xLastID;
				bFirstRun = false;
				return true;
			}
			if (--xSubimgID < xFirstID) {
				xSubimgID = xLastID;
				if (++ySubimgID > yLastID) {
					bFirstRun = true;
					return false;
				}
			}
			//dbg_sprintf(dbgout, "\n3. xSubimgID %d : ( %d - %d ) \n   ySubimgID %d : ( %d - %d )", xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID);
			return true;
		}
		else {
			//draw rows starting at bottom right?

			if (bFirstRun) {
				xSubimgID = xLastID;
				ySubimgID = yLastID;
				bFirstRun = false;
				return true;
			}
			if (--xSubimgID < xFirstID) {
				xSubimgID = xLastID;
				if (--ySubimgID < yFirstID) {
					bFirstRun = true;
					return false;
				}
			}
			//dbg_sprintf(dbgout, "\n4. xSubimgID %d : ( %d - %d ) \n   ySubimgID %d : ( %d - %d )", xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID);
			return true;
		}
	}
	return false;
}

// divide and round up if necessary
int24_t ceilDiv(int24_t x, int24_t y)
{
	return (x + y - 1) / y;
}