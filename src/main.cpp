/*HD Picture Viewer
* By TheLastMillennial
* https://github.com/TheLastMillennial/HD-Picture-Viewer
* To build:
* 1. Open command prompt.
* 2. cd to root HD-Picture-Viewer folder.
* 3. run `make debug --directory=./`
*/

#include <tice.h>
#include <graphx.h>
#include <string.h>
#include <fileioc.h>
#include <debug.h>
#include <compression.h>
#include <gfx16.h>
#include <cstring>
#include <cmath>

#include "main.h"
#include "loadingBarHandler.h"
#include "keyPressHandler.h"
#include "gfxCompatibility.h"
#include "pictureDatabase.h"
#include "globals.h"
#include "guiUtils.h"
#include "types/vector.h"


int main(void)
{
	dbg_sprintf(dbgout, "\nStart");

	//initialize 8 & 16bpp compatibility functions
	HDpicGFX &gfx = HDpicGFX::getInstance();
	gfx.use16bpp();

	//draw loading screen
	drawSplashScreen();

	dbg_sprintf(dbgout, "\nFinding pictures");

	//Detect pictures on the calculator
	if (findPictures() == 0) {
		dbg_sprintf(dbgout, "\nNo pictures");

		drawNoImagesFound();
		KeyPressHandler::waitForAnyKey();
		HDpicGFX::end();
		return 0;
	}
	dbg_sprintf(dbgout, "\nHome screen ");


	//display the list of images
	drawHomeScreen();


	//quit
	HDpicGFX::end();
	kb_ClearOnLatch();
	return 0;

}


// Display UI to select an image
//starts at 16bpp
void drawHomeScreen()
{
	uint24_t selectedPicIndex{ 0 },
		desiredWidthInPxl{ MAX_THUMBNAIL_WIDTH }, desiredHeightInPxl{ MAX_THUMBNAIL_HEIGHT };
	//set up variable that checks if drawImage failed
	uint8_t imageErr{ 0 };
	PicDatabase &picDB = PicDatabase::getInstance();
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();
	HDpicGFX &gfx = HDpicGFX::getInstance();

	HDpicGFX::autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);

	/* main menu */
	if (gfx.is16bppMode()) {
		dbg_sprintf(dbgout, "\ndrawMenu_16bpp");
		gfx16_FillScreen(GFX16_BLACK);
		drawMenu_16bpp(selectedPicIndex);
	}
	else {
		dbg_sprintf(dbgout, "\ndrawMenu_8bpp");
		gfx_FillScreen(PALETTE_BLACK);
		drawMenu_8bpp(selectedPicIndex);
	}

	//thumbnail
	dbg_sprintf(dbgout, "\n drawImage");
	drawImage(selectedPicIndex, 180, 120, false);

	/* UI */
	bool quitProgram{ false };
	uint24_t errorID = 0;

	do {
		static bool fullScreenImage{ false };
		static uint8_t existingKeypressCount{ 0 };
		bool resetPic{ false }, redrawPic{ false };

		// Pressing on means halt immediately.
		if (kb_On) {
			kb_ClearOnLatch();
			keyHandler.reset();
			dbg_sprintf(dbgout, "\nRender aborted by ON.");

			if (gfx.is16bppMode()) {
				gfx16_SetTextBGColor(GFX16_BG_0);
				gfx16_SetTextFGColor(GFX16_TEXT_ERROR);
				gfx16_PrintCenteredX("Render Interrupted.", 10);
				gfx16_PrintCenteredX("Press enter to continue.", 215);
			}
			else {
				gfx_SetTextBGColor(PALETTE_BLACK);
				gfx_SetTextFGColor(PALETTE_WHITE);
				PrintCenteredX("Render Interrupted.", 10);
				PrintCenteredX("Press enter to continue.", 215);
			}
			KeyPressHandler::waitForAnyKey();
		}

		//scans the keys for existing keypress.
		if (!keyHandler.isAnyKeyPressed()) {
			//no existing key press. Check for new keypress.
			if (!keyHandler.scanKeys(fullScreenImage)) {
				continue; // no new keypress. There is nothing to do.
			}
			else {
				existingKeypressCount = 0;
				dbg_sprintf(dbgout, "\nNew Key press");
			}
		}
		else {
			// an existing key should not be held for this many times in a row
			if (existingKeypressCount++ > 10)
				keyHandler.reset();
			dbg_sprintf(dbgout, "\nExisting key press");
		}

		/* Key press detected */

		// clear. Go back.
		if (keyHandler.wasKeyPressed(kb_KeyClear)) {
			//If we're viewing an image, exit to menu. If we're already on menu, quit program.
			if (fullScreenImage) {
				fullScreenImage = false;
				resetPic = true;
				redrawPic = true;
				errorID = kb_KeyClear; //1600
				if (gfx.is16bppMode())
					gfx16_FillScreen(GFX16_BLACK);
				else
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
			errorID = kb_KeyEnter;//1537
		}

		// mode. Show help.
		if (keyHandler.wasKeyPressed(kb_KeyMode)) {
			bool prev16bpp = gfx.is16bppMode();
			HDpicGFX::use16bpp();
			drawHelp();
			KeyPressHandler::waitForAnyKey();

			if (prev16bpp) {
				HDpicGFX::use16bpp();
				gfx16_FillScreen(GFX16_BLACK);
			}
			else {
				HDpicGFX::use8bpp();
				gfx_FillScreen(PALETTE_BLACK);
			}

			resetPic = true;
			redrawPic = true;
			errorID = kb_KeyMode; //320
		}


		//Delete. delete all appvars related to current image
		if (keyHandler.wasKeyPressed(kb_KeyDel)) {
			HDpicGFX::use16bpp();
			//we don't want the user seeing the horrors of their image with the wrong palette
			gfx16_FillScreen(GFX16_BLACK);
			gfx16_SetTextFGColor(GFX16_TEXT);
			gfx16_SetTextBGColor(GFX16_BLACK);
			gfx16_PrintCenteredX("Deleting Picture...", 120);

			//delete the palette and all subimages
			PicDatabase &picDB = PicDatabase::getInstance();
			picDB.deleteImage(selectedPicIndex);

			gfx16_SetTextFGColor(GFX16_TEXT_SUCCESS);
			gfx16_PrintCenteredX("Picture deleted.", 130);
			gfx16_SetTextFGColor(GFX16_TEXT);
			gfx16_PrintCenteredX("Press any key.", 140);
			KeyPressHandler::waitForAnyKey();
			keyHandler.reset();

			//set color for splash screen
			gfx16_SetTextFGColor(GFX16_TEXT);
			gfx16_SetTextBGColor(GFX16_BLACK);

			//check if all images were deleted. If so, just quit.
			if (picDB.size() == 0) {
				drawNoImagesFound();
				KeyPressHandler::waitForAnyKey();
				return;
			}

			//select next picture
			if (selectedPicIndex > 0) {
				selectedPicIndex--;
			}

			//prepare for redrawing everything
			gfx16_FillScreen(GFX16_BLACK);

			resetPic = true;
			redrawPic = true;
			errorID = kb_KeyDel; //384
		}

		/* Graph or down. Increases the name to start on and redraws the text */
		if (keyHandler.wasKeyPressed(kb_KeyGraph) || (keyHandler.wasKeyPressed(kb_KeyDown) && !fullScreenImage)) {
			selectedPicIndex++;
			//make sure user can't scroll down too far
			if (selectedPicIndex > picDB.size() - 1) {
				dbg_sprintf(dbgout, "\ntoo high %d -> 0", selectedPicIndex);
				selectedPicIndex = 0;
			}
			gfx.autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);
			resetPic = fullScreenImage;
			redrawPic = true;
			errorID = kb_KeyGraph; //257 if an error is thrown, then we've scrolled past the safety barrier somehow.

		}

		/* Y= or up. Decreases the name to start on and redraws the text */
		if (keyHandler.wasKeyPressed(kb_KeyYequ) || (keyHandler.wasKeyPressed(kb_KeyUp) && !fullScreenImage)) {
			selectedPicIndex--;
			// Checks if selectedName underflowed. selectedName shouldn't be more than the max number of images possible.
			if (selectedPicIndex > MAX_IMAGES) {
				dbg_sprintf(dbgout, "\nunderflow: %d -> %d", selectedPicIndex, (picDB.size() - 1));
				selectedPicIndex = picDB.size() - 1;
			}
			gfx.autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);
			resetPic = fullScreenImage;
			redrawPic = true;
			errorID = kb_KeyYequ; //272 if an error is thrown, then we've scrolled past the safety barrier somehow.
		}

		//left, right, up, down. Image panning.
		if (fullScreenImage) {
			if (keyHandler.wasKeyPressed(kb_KeyLeft)) {
				errorID = kb_KeyLeft; //1794
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyRight)) {
				errorID = kb_KeyRight; //1796
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, -1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyUp)) {
				errorID = kb_KeyUp; //1800
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, -1);
			}
			if (keyHandler.wasKeyPressed(kb_KeyDown)) {
				errorID = kb_KeyDown; //1793
				imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, 1);
			}
		}//temporary }
		/* DISABLED until gfx16 lib supports resizing sprite */
		//	//Zoom key. Zoom in as far as possible while maintaining full quality
		//	if (keyHandler.wasKeyPressed(kb_KeyZoom)) {
		//		//pull image full dimensions from database
		//		PicDatabase &picDB = PicDatabase::getInstance();

		//		//convert subimg width to pixels width
		//		uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
		//		desiredWidthInPxl = picDB.getPicture(selectedPicIndex).horizSubImages * SUBIMAGE_DIMENSIONS;
		//		desiredHeightInPxl = picDB.getPicture(selectedPicIndex).vertSubImages * SUBIMAGE_DIMENSIONS;

		//		imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
		//		//this means we can't zoom in any more. Zoom back out.
		//		if (imageErr != 0) {
		//			dbg_sprintf(dbgout, "\nCant zoom in to Max. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
		//			desiredWidthInPxl = prevWidth;
		//			desiredHeightInPxl = prevHeight;
		//			redrawPic = true;
		//		}
		//		errorID = kb_KeyZoom; //260
		//	}

			/* DISABLED until gfx16 lib supports resizing sprite */
			//Plus key. Zoom in by double
			//if (keyHandler.wasKeyPressed(kb_KeyAdd)) {
			//	uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
			//	//calculate increased zoom
			//	desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
			//	desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
			//	dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom In\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
			//	//if (desiredWidthInPxl != 0 && desiredHeightInPxl != 0) {

			//		imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
			//		//this means we can't zoom in any more. Zoom back out.
			//		if (imageErr != 0) {
			//			dbg_sprintf(dbgout, "\nCant zoom in. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
			//			desiredWidthInPxl = prevWidth;
			//			desiredHeightInPxl = prevHeight;
			//		}
			//	//}
			//	errorID = kb_KeyAdd; //1538
			//}

			/* DISABLED until gfx16 lib supports resizing sprite */
			//subtract key. Zoom out by double.
			//if (keyHandler.wasKeyPressed(kb_KeySub)) {
			//	//apply the zoom out to the width and height
			//	uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
			//	desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
			//	desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;

			//	if (desiredWidthInPxl != 0 && desiredHeightInPxl != 0) {
			//		dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
			//		imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
			//		//this means we can't zoom out any more. Zoom back in.
			//		if (imageErr != 0) {
			//			dbg_sprintf(dbgout, "\nCant zoom out. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
			//			desiredWidthInPxl = prevWidth;
			//			desiredHeightInPxl = prevHeight;
			//		}
			//	}
			//	errorID = kb_KeySub; //1540
			//}
		//}

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
			//this can cover up other errors so append it
			errorID = errorID * 1000 + kb_KeyWindow; //264
		}

		// If necessary, draw the image with new settings.
		if (redrawPic) {
			// change gfx libraries, if necessary.
			HDpicGFX::autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);
			if (!fullScreenImage) {
				if (gfx.is16bppMode()) {
					gfx16_FillScreen(GFX16_BLACK);
					drawMenu_16bpp(selectedPicIndex);
				}
				else {
					gfx_FillScreen(PALETTE_BLACK);
					drawMenu_8bpp(selectedPicIndex);
				}
			}

			keyHandler.reset();
			imageErr = drawImage(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, fullScreenImage);
			if (imageErr != 0) {
				HDpicGFX::use8bpp();
				gfx_PrintStringXY("Error: ", (LCD_WIDTH - gfx_GetStringWidth("Error: ")) / 2, 150);
				gfx_PrintUInt(errorID, 6);
				gfx_PrintStringXY("Press any key to quit.", (LCD_WIDTH - gfx_GetStringWidth("Press any key to quit.")) / 2, 160);
				KeyPressHandler::waitForAnyKey();
				return;
			}
		}

		if (!fullScreenImage) {
			if (gfx.is16bppMode())
				drawWatermark_16bpp();
			else
				drawWatermark_8bpp();
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
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool fullScreenPic, int8_t shiftX, int8_t shiftY)
{
	dbg_sprintf(dbgout, "\n\n--DRAW IMAGE--");
	PicDatabase &picDB = PicDatabase::getInstance();
	imageData &curPicture = picDB.getPicture(picName);
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();
	HDpicGFX &gfx = HDpicGFX::getInstance();
	HDpicGFX::autoSelectLibrary(curPicture.BPP);


	//checks if it should scale an image horizontally or vertically.
	int24_t scaleNumerator{ 1 }, scaleDenominator{ 1 }, subimgNewDimNumerator{ 0 };
	if ((curPicture.horizSubImages * SUBIMAGE_DIMENSIONS) / LCD_WIDTH >= (curPicture.vertSubImages * SUBIMAGE_DIMENSIONS) / LCD_HEIGHT) {
		//width too wide
		scaleNumerator = desiredWidthInPxl;
		scaleDenominator = curPicture.horizSubImages * SUBIMAGE_DIMENSIONS;
	}
	else {
		// Height too tall
		scaleNumerator = desiredHeightInPxl;
		scaleDenominator = curPicture.vertSubImages * SUBIMAGE_DIMENSIONS;
	}

	//dbg_sprintf(dbgout, "\n horizSubImages: %d\n vertSubImages: %d",	curPicture.horizSubImages, curPicture.vertSubImages);
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



	//Final dimension of all subimages
	subimgNewDimNumerator = SUBIMAGE_DIMENSIONS * scaleNumerator;
	int24_t subimgScaledDim{ subimgNewDimNumerator / scaleDenominator };

	//dbg_sprintf(dbgout, "\n subimgScaledDim %d\n subimgNewDimNumerator: %d \n ScaleNum: %d \n scaleDenominator: %d \n xOffset: %d \n yOffset %d",
	//	subimgScaledDim, subimgNewDimNumerator, scaleNumerator, scaleDenominator, curPicture.xOffset, curPicture.yOffset);

	//ensure the resized subimage will fit within the dimensions of the screen.
	if (subimgScaledDim > LCD_HEIGHT) {
		dbg_sprintf(dbgout, "\nERR: Subimage will be too large: %d", subimgScaledDim);
		return 1;
	}

	if (subimgScaledDim < 2) {
		dbg_sprintf(dbgout, "\nERR: Subimage will be too small: %d", subimgScaledDim);
		return 1;
	}


	//sets correct palettes
	//requires 8bpp mode
	if (!gfx.is16bppMode()) {
		char palName[9];
		sprintf(palName, "HP%.2s0000", curPicture.ID);
		uint24_t iEntries = static_cast<uint24_t>(pow(2.0, static_cast<double>(curPicture.BPP)) * 2);
		if (!gfx.usePalette(palName, iEntries)) {
			PrintCenteredX(palName, 110);
			PrintCenteredX("ERR: Palette does not exist!", 120);
			PrintCenteredX("Image may have recently been deleted.", 130);
			PrintCenteredX("Try restarting the program.", 140);
			KeyPressHandler::waitForAnyKey();
			return 1;
		}
	}


	/* Apply Pan Offset */

	// Which direction to draw the subimages. 
	// By default it's the percieved most performant option
	// If the image gets panned, it prioritizes filling in the missing subimages first.
	bool bReverseDirection{ false }; //false means the image initially draws slow then speeds up.
	bool bDrawVertical{ true }; //true: draws columns. False: draws rows.

	picDB.getPicture(picName).xOffset += shiftX;
	picDB.getPicture(picName).yOffset += shiftY;

	//Check if we need to pan the image. If so, shift the contents of the screen over so we don't need to redraw as many subimages.
	if (shiftX != 0 || shiftY != 0) {
		if (HDpicGFX::is16bppMode()) {
			gfx16_SetColor(GFX16_BLACK);
		}
		else {
			//only 8bpp can double buffer
			gfx_SetColor(PALETTE_BLACK);
			gfx_SetDrawBuffer();
		}

		// Shift screen to right
		if (shiftX > 0) {
			bReverseDirection = true;
			bDrawVertical = true;
			if (HDpicGFX::is16bppMode()) {
				gfx16_CopyRectangle(0, 0, subimgScaledDim, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
				gfx16_FillRectangle_NoClip(0, 0, subimgScaledDim, LCD_HEIGHT);
			}
			else {
				gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, 0, subimgScaledDim, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
				gfx_FillRectangle_NoClip(0, 0, subimgScaledDim, LCD_HEIGHT);
			}
		}
		// Shift screen to left
		if (shiftX < 0) {
			bReverseDirection = false;
			bDrawVertical = true;
			if (HDpicGFX::is16bppMode()) {
				gfx16_CopyRectangle(subimgScaledDim, 0, 0, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
				gfx16_FillRectangle_NoClip(LCD_WIDTH - subimgScaledDim, 0, subimgScaledDim, LCD_HEIGHT);

			}
			else {
				gfx_CopyRectangle(gfx_screen, gfx_buffer, subimgScaledDim, 0, 0, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
				gfx_FillRectangle_NoClip(LCD_WIDTH - subimgScaledDim, 0, subimgScaledDim, LCD_HEIGHT);
			}
		}
		// Shift screen up
		if (shiftY > 0) {
			bReverseDirection = false;
			bDrawVertical = false;
			if (HDpicGFX::is16bppMode()) {
				gfx16_CopyRectangle(0, subimgScaledDim, 0, 0, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
				gfx16_FillRectangle_NoClip(0, LCD_HEIGHT - subimgScaledDim, LCD_WIDTH, subimgScaledDim);

			}
			else {
				gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, subimgScaledDim, 0, 0, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
				gfx_FillRectangle_NoClip(0, LCD_HEIGHT - subimgScaledDim, LCD_WIDTH, subimgScaledDim);
			}
		}
		// Shift screen down
		if (shiftY < 0) {
			bReverseDirection = true;
			bDrawVertical = false;
			if (HDpicGFX::is16bppMode()) {
				gfx16_CopyRectangle(0, 0, 0, subimgScaledDim, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
				gfx16_FillRectangle_NoClip(0, 0, LCD_WIDTH, subimgScaledDim);

			}
			else {
				gfx_CopyRectangle(gfx_screen, gfx_buffer, 0, 0, 0, subimgScaledDim, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
				gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, subimgScaledDim);
			}
		}
		//Show other 8bpp buffer
		if (!HDpicGFX::is16bppMode()) {
			gfx_BlitBuffer();
			gfx_SetDrawScreen();
		}
		while (!os_GetCSC()); //todo: remove
	}
	else if (fullScreenPic) {
		//If there's no panning, then we need to re-draw the entire image. 
		if (HDpicGFX::is16bppMode())
			gfx16_FillScreen(GFX16_BLACK);
		else
			gfx_FillScreen(PALETTE_BLACK);

	}

	/* Set up to display all the subimages */
	dbg_sprintf(dbgout, "\n-------------------------");

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

	/* Check for invalid situations */
	if (rightMostSubimg < 0 || bottomMostSubimg < 0 || (rightMostSubimg - 1 < leftMostSubimg) || (bottomMostSubimg - 1) < topMostSubimg) {
		dbg_sprintf(dbgout, "\nERR: Image Name: %s\n rightmost: %d\n leftmost:  %d\n topmost:    %d\n bottommost: %d", curPicture.imgName, rightMostSubimg, leftMostSubimg, topMostSubimg, bottomMostSubimg);
		return 1;
	}

	/* Display final image */

	//the -1 is to account for some loop weirdness. Specifically in the iterate() function.
	//this for loop outputs pic right to left, top to bottom
	const int24_t xFirstID{ (leftMostSubimg) }, xLastID{ (rightMostSubimg)-1 };
	const int24_t yFirstID{ (topMostSubimg) }, yLastID{ (bottomMostSubimg)-1 };

	const uint24_t thumbnailOffsetX = fullScreenPic ? 0 : 150;
	const uint24_t thumbnailOffsetY = fullScreenPic ? 0 : ((240 - (subimgScaledDim * bottomMostSubimg)) / 2);

	dbg_sprintf(dbgout, "\n Image Name: %s\n x-range: %d - %d\n y-range: %d - %d", curPicture.imgName, xFirstID, xLastID, yFirstID, yLastID);

	// If displaying thumbnail, cover up the last image
	if (!fullScreenPic) {
		if (gfx.is16bppMode()) {
			gfx16_SetColor(GFX16_BLACK);
			gfx16_FillRectangle_NoClip(150, 0, 170, 240);
		}
		else {
			gfx_SetColor(PALETTE_BLACK);
			gfx_FillRectangle_NoClip(150, 0, 170, 240);
		}
	}

	/* DISABLED until gfx16 lib supports resizing sprite */
	//allocates memory for resized image
	//gfx_sprite_t *outputImg{ gfx_MallocSprite(subimgScaledDim,subimgScaledDim) };
	//if (!outputImg) {
	//	dbg_sprintf(dbgout, "\nERR: Failed to allocate output memory!");
	//	return 1;
	//}

	//pointer to memory where each unsized subimage will be stored
	gfx_sprite_t *srcImg{ gfx_MallocSprite(SUBIMAGE_DIMENSIONS, SUBIMAGE_DIMENSIONS) };
	if (!srcImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate srcImg memory!");
		return 1;
	}

	//pointer to memory where each unsized subimage will be stored
	gfx_sprite_t *tempImg{ gfx_MallocSprite(SUBIMAGE_DIMENSIONS, SUBIMAGE_DIMENSIONS) };
	if (!tempImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate tempImg src memory!");
		return 1;
	}

	/* Loop through all subimages to create full image */
	bool bFirstRun{ true };
	//If there's no cache yet, don't bother even checking it.
	bool bDisableCache{ curPicture.cache.isEmpty() };
	//dbg_sprintf(dbgout, "\nbDisableCache: %d", bDisableCache);

	int24_t xSubimgID{ 0 };
	int24_t ySubimgID{ 0 };
	while (iterate(xSubimgID, xFirstID, xLastID, ySubimgID, yFirstID, yLastID, bDrawVertical, bReverseDirection, bFirstRun)) {

		// Figure out exactly which pixel the subimage need to be displayed
		const uint24_t subimgPxlPosX{ thumbnailOffsetX + static_cast<uint24_t>((xSubimgID + curPicture.xOffset) * (subimgNewDimNumerator / scaleDenominator)) };
		const uint24_t subimgPxlPosY{ thumbnailOffsetY + static_cast<uint24_t>((ySubimgID - curPicture.yOffset) * (subimgNewDimNumerator / scaleDenominator)) };

		dbg_sprintf(dbgout, "\nLooped.\n xSubimgID: %d @ %d pxl \n ySubimgID: %d @ %d pxl", xSubimgID, subimgPxlPosX, ySubimgID, subimgPxlPosY);

		//a key interrupted output. Quit immediately
		if (kb_On || keyHandler.scanKeys(fullScreenPic)) {
			dbg_sprintf(dbgout, "\nRender aborted!\n");
			//free up source and output memory
			free(srcImg);
			free(tempImg);
			//free(outputImg);
			return 0;
		}

		if (subimgPxlPosX > LCD_WIDTH || subimgPxlPosX < 0 || subimgPxlPosY > LCD_HEIGHT || subimgPxlPosY < 0)
			continue;

		//combines the separate parts into one name to search for
		char picAppvarToFind[9];

		dbg_sprintf(dbgout, "\nAppVar Name: %.2s%03u%03u", curPicture.ID, xSubimgID, ySubimgID);

		//Pull pointer to the subimage from the cache
		void *subimgPtr{ nullptr };
		if (!bDisableCache) {
			dbg_sprintf(dbgout, "\n Cache Hit.");
			subimgPtr = curPicture.cache[xSubimgID][ySubimgID];
		}

		//Check for cache miss
		ti_var_t subimgSlot = NULL;
		if (subimgPtr == nullptr) {
			//cache miss. Find the appvar by name
			sprintf(picAppvarToFind, "%.2s%03u%03u", curPicture.ID, xSubimgID, ySubimgID);

			//dbg_sprintf(dbgout, "\n Cache Miss. picAppvarToFind: %.8s", picAppvarToFind);

			subimgSlot = ti_Open(picAppvarToFind, "r");

			if (subimgSlot) {
				//seeks past header. 16bpp has different header size than 8bpp
				if (curPicture.BPP == 16) {
					ti_Seek(24, SEEK_CUR, subimgSlot);
				}
				else {
					ti_Seek(16, SEEK_CUR, subimgSlot);
				}

				//cache the pointer to the subimage for next time
				subimgPtr = ti_GetDataPtr(subimgSlot);

				curPicture.cache[xSubimgID][ySubimgID] = subimgPtr;
			}
			else {
				//subimage does not exist, display error image
				dbg_sprintf(dbgout, "\nERR: Subimage doesn't exist: %s", picAppvarToFind);
				continue;
			}
		}
		/* subimage exists, display it */

		//decompress subimage into srcImg
		zx0_Decompress(srcImg, subimgPtr);

		//resizes it to outputImg size
		//gfx_ScaleSprite(srcImg, outputImg);

		//displays subimage
		//if we are displaying an edge image, clip the subimage. Otherwise don't clip for extra speed.
		dbg_sprintf(dbgout, "\nsubImgX: %d\nsubImgY: %d\nsrcImg: %p", subimgPxlPosX, subimgPxlPosY, (void *)&srcImg);

		uint8_t pixelsPerByte = 8 / curPicture.BPP;
		uint24_t dataToRead = (SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS) / pixelsPerByte;
		uint24_t out{ 0 };

		/*dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosX < 0: %d < 0", subimgPxlPosX < 0, subimgPxlPosX);
		dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosX + subimgScaledDim > LCD_WIDTH: %d > %d", subimgPxlPosX + subimgScaledDim > LCD_WIDTH, subimgPxlPosX + subimgScaledDim, LCD_WIDTH);
		dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosY < 0: %d < 0", subimgPxlPosY < 0, subimgPxlPosY);
		dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosY + subimgScaledDim > LCD_HEIGHT: %d > %d", subimgPxlPosY + subimgScaledDim > LCD_HEIGHT, subimgPxlPosY + subimgScaledDim, LCD_HEIGHT);*/

		if (subimgPxlPosX < 0 || subimgPxlPosX + subimgScaledDim > LCD_WIDTH || subimgPxlPosY < 0 || subimgPxlPosY + subimgScaledDim > LCD_HEIGHT) {
			switch (curPicture.BPP) {
			case 1:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);

					tempImg->data[out++] = (byte >> 7) & 0x01;
					tempImg->data[out++] = (byte >> 6) & 0x01;
					tempImg->data[out++] = (byte >> 5) & 0x01;
					tempImg->data[out++] = (byte >> 4) & 0x01;
					tempImg->data[out++] = (byte >> 3) & 0x01;
					tempImg->data[out++] = (byte >> 2) & 0x01;
					tempImg->data[out++] = (byte >> 1) & 0x01;
					tempImg->data[out++] = byte & 0x01;
				}
				gfx_Sprite(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 2:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);

					tempImg->data[out++] = (byte >> 6) & 0x03;
					tempImg->data[out++] = (byte >> 4) & 0x03;
					tempImg->data[out++] = (byte >> 2) & 0x03;
					tempImg->data[out++] = byte & 0x03;
				}
				gfx_Sprite(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 4:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);

					tempImg->data[out++] = (byte >> 4) & 0x0F;
					tempImg->data[out++] = byte & 0x0F;
				}
				gfx_Sprite(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 8:
				gfx_Sprite(srcImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 16:
				gfx16_Sprite(srcImg, subimgPxlPosX, subimgPxlPosY);
				break;
			}
		}
		else {
			switch (curPicture.BPP) {
			case 1:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);
					tempImg->data[out++] = (byte >> 7) & 0x01;
					tempImg->data[out++] = (byte >> 6) & 0x01;
					tempImg->data[out++] = (byte >> 5) & 0x01;
					tempImg->data[out++] = (byte >> 4) & 0x01;
					tempImg->data[out++] = (byte >> 3) & 0x01;
					tempImg->data[out++] = (byte >> 2) & 0x01;
					tempImg->data[out++] = (byte >> 1) & 0x01;
					tempImg->data[out++] = byte & 0x01;
				}
				gfx_Sprite_NoClip(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 2:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);
					tempImg->data[out++] = (byte >> 6) & 0x03;
					tempImg->data[out++] = (byte >> 4) & 0x03;
					tempImg->data[out++] = (byte >> 2) & 0x03;
					tempImg->data[out++] = byte & 0x03;
				}
				gfx_Sprite_NoClip(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 4:
				for (size_t i = 0; i < dataToRead; i++) {
					uint8_t byte = static_cast<uint8_t>(srcImg->data[i]);
					tempImg->data[out++] = (byte >> 4) & 0x0F;
					tempImg->data[out++] = byte & 0x0F;
				}
				gfx_Sprite_NoClip(tempImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 8:
				gfx_Sprite_NoClip(srcImg, subimgPxlPosX, subimgPxlPosY);
				break;

			case 16:
				gfx16_Sprite_NoClip(srcImg, subimgPxlPosX, subimgPxlPosY);
				break;
			}
		}

		//cleans up
		ti_Close(subimgSlot);
	}

	//free up source and output memory
	free(srcImg);
	free(tempImg);
	//free(outputImg);

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
	HDpicGFX::use16bpp();
	drawSplashScreen();

	LoadingBar &loadingBar = LoadingBar::getInstance();
	loadingBar.resetLoadingBar(MAX_IMAGES);
	/*
	* Searches for first sub-image.
	* It contains all the useful information such as the image size and
	* the two letter ID for each appvar.
	* This makes it easy to find the other subimages via a loop.
	*/

	//find 16 bit pictures
	while ((var_name = ti_DetectVar(&search_pos, "HDPIC16A", OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}
	// find 1,2,4,8 bit pictures
	while ((var_name = ti_DetectVar(&search_pos, "HDPALV11", OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}

	PicDatabase &picDB = PicDatabase::getInstance();
	picDB.reserve(imagesFound);

	loadingBar.resetLoadingBar(imagesFound);
	search_pos = NULL;

	while ((var_name = ti_DetectVar(&search_pos, "HDPIC16A", OS_TYPE_APPVAR)) != NULL) {

		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t HORIZ_VERT_SIZE{ 3 };
		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };
		constexpr uint8_t HEADER_SIZE{ 18 };

		loadingBar.increment();

		imageData imgData;
		//finds the name, letter ID, and size of entire image this picture belongs to.
		ti_var_t  firstPic;
		dbg_sprintf(dbgout, "\nfirstPic %.8s", var_name);

		firstPic = ti_Open(var_name, "r");
		//seeks past HDPIC16A
		ti_Seek(8, SEEK_CUR, firstPic);
		//reads the important info
		//e.g. poppy___JT003002
		ti_Read(&imgInfo, HEADER_SIZE, 1, firstPic);

		char charArrImgInfo[HEADER_SIZE];
		std::strncpy(charArrImgInfo, imgInfo, HEADER_SIZE);
		std::strncpy(imgData.imgName, charArrImgInfo, IMAGE_NAME_SIZE);
		std::strncpy(imgData.ID, charArrImgInfo + IMAGE_NAME_SIZE, ID_SIZE);

		imgData.BPP = 16; //Images in this section will always be 16 BPP
		imgData.imgName[8] = '\0';
		imgData.ID[2] = '\0';

		// Get width of whole image. Then convert the number from a char representation to a int24_t
		char buffer[3];
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE, HORIZ_VERT_SIZE);
		imgData.horizSubImages = charToInt(buffer[0]) * 100 + charToInt(buffer[1]) * 10 + charToInt(buffer[2]) + 1;
		std::strncpy(buffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE + HORIZ_VERT_SIZE, HORIZ_VERT_SIZE);
		imgData.vertSubImages = charToInt(buffer[0]) * 100 + charToInt(buffer[1]) * 10 + charToInt(buffer[2]) + 1;

		dbg_sprintf(dbgout, "\nPicture found:\n imgName: %.8s\n ID: %.2s\n subImgHoriz: %d\n subImgVert: %d\n", imgData.imgName, imgData.ID, imgData.horizSubImages, imgData.vertSubImages);

		picDB.addPicture(imgData);

		//closes pic for next iteration
		ti_Close(firstPic);
	}

	while ((var_name = ti_DetectVar(&search_pos, "HDPALV11", OS_TYPE_APPVAR)) != NULL) {

		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t HORIZ_VERT_SIZE{ 3 };
		constexpr uint8_t PALETTE_NAME_SIZE{ 8 };
		constexpr uint8_t BITS_PER_PIXEL_SIZE{ 2 };
		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };
		constexpr uint8_t HEADER_SIZE{ 18 };

		loadingBar.increment();

		imageData imgData;
		//finds the name, letter ID, and size of entire image this palette belongs to.
		ti_var_t  palette;
		dbg_sprintf(dbgout, "\npalette %.8s", var_name);

		palette = ti_Open(var_name, "r");
		//seeks past HDPALV11
		ti_Seek(8, SEEK_CUR, palette);
		//reads the important info
		//e.g. 08poppy___JT003002
		ti_Read(&imgInfo, HEADER_SIZE, 1, palette);

		char charArrImgInfo[HEADER_SIZE];
		char BPPbuffer[BITS_PER_PIXEL_SIZE];
		std::strncpy(charArrImgInfo, imgInfo, HEADER_SIZE);
		dbg_sprintf(dbgout, "\n charArrImgInfo\n BPP: %.18s", charArrImgInfo);
		std::strncpy(BPPbuffer, charArrImgInfo, BITS_PER_PIXEL_SIZE);
		std::strncpy(imgData.imgName, charArrImgInfo + BITS_PER_PIXEL_SIZE, IMAGE_NAME_SIZE);
		std::strncpy(imgData.ID, charArrImgInfo + BITS_PER_PIXEL_SIZE + IMAGE_NAME_SIZE, ID_SIZE);
		std::strncpy(imgData.paletteName, var_name, PALETTE_NAME_SIZE);

		imgData.imgName[8] = '\0';
		imgData.paletteName[8] = '\0';
		imgData.ID[2] = '\0';

		// Get width of whole image. Then convert the number from a char representation to a int24_t
		char dimBuffer[3];
		std::strncpy(dimBuffer, charArrImgInfo + BITS_PER_PIXEL_SIZE + IMAGE_NAME_SIZE + ID_SIZE, HORIZ_VERT_SIZE);
		imgData.horizSubImages = charToInt(dimBuffer[0]) * 100 + charToInt(dimBuffer[1]) * 10 + charToInt(dimBuffer[2]) + 1;
		std::strncpy(dimBuffer, charArrImgInfo + BITS_PER_PIXEL_SIZE + IMAGE_NAME_SIZE + ID_SIZE + HORIZ_VERT_SIZE, HORIZ_VERT_SIZE);
		imgData.vertSubImages = charToInt(dimBuffer[0]) * 100 + charToInt(dimBuffer[1]) * 10 + charToInt(dimBuffer[2]) + 1;

		//convert the char BPP to a uint8_t
		imgData.BPP = static_cast<uint8_t>(charToInt(BPPbuffer[0]) * 10 + charToInt(BPPbuffer[1]));

		dbg_sprintf(dbgout, "\nPicture found:\n imgName: %.8s\n palName: %.8s\n ID: %.2s\n BPP: %d\n subImgHoriz: %d\n subImgVert: %d\n", imgData.imgName, imgData.paletteName, imgData.ID, imgData.BPP, imgData.horizSubImages, imgData.vertSubImages);

		picDB.addPicture(imgData);

		//closes palette for next iteration
		ti_Close(palette);
	}

	drawSplashScreen();
	dbg_sprintf(dbgout, "\nPics Detected: %d", imagesFound);
	return imagesFound;
}

/* This UI keeps the user selection in the middle of the screen. */
void drawMenu_16bpp(uint24_t selectedName)
{
	gfx16_SetColor(GFX16_WHITE);
	gfx16_VertLine(140, 20, 200);

	uint24_t yPxlPos{ 0 };

	//clears old text and sets prev for new text
	gfx16_SetTextScale(2, 2);
	gfx16_SetColor(GFX16_BLACK);
	gfx16_FillRectangle_NoClip(0, 0, 140, 240);
	gfx16_SetColor(GFX16_WHITE);
	gfx16_SetTextFGColor(GFX16_TEXT);
	gfx16_SetTextBGColor(GFX16_BLACK);

	//re-draws UI lines
	gfx16_HorizLine_NoClip(0, 120, 6);
	gfx16_HorizLine_NoClip(136, 120, 5);
	gfx16_HorizLine_NoClip(6, 110, 130);
	gfx16_HorizLine_NoClip(6, 130, 130);
	gfx16_VertLine_NoClip(6, 110, 20);
	gfx16_VertLine_NoClip(136, 110, 21);

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
			gfx16_PutStringXY(picDB.getPicture(curImg).imgName, X_MARGIN, yPxlPos);
		}
	}

	//display selected image name in center of screen
	yPxlPos = Y_MARGIN + 75;
	gfx16_PutStringXY(picDB.getPicture(selectedName).imgName, X_MARGIN, yPxlPos);

	/* Draw image names below selected name.
	* Iterates until out of pics or about to draw off the screen */
	if (selectedName + 1 < picDB.size()) {
		for (uint24_t curName{ selectedName + 1 }; (curName < picDB.size()) && (yPxlPos < 210); curName++) {
			//calculates where the text should be drawn
			yPxlPos += Y_SPACING;

			//Prints out the correct name
			gfx16_PutStringXY(picDB.getPicture(curName).imgName, X_MARGIN, yPxlPos);
		}
	}
	drawWatermark_16bpp();
}

/* This UI keeps the user selection in the middle of the screen. */
void drawMenu_8bpp(uint24_t selectedName)
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
	drawWatermark_8bpp();
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
bool iterate(int24_t &xSubimgID, int24_t const &xFirstID, int24_t const &xLastID, int24_t &ySubimgID, int24_t const &yFirstID, int24_t const &yLastID, bool bDrawVertically, bool bDrawOppositeSideFirst, bool &bFirstRun)
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

//converts number character to int24_t i.e. '5' -> 5
int24_t charToInt(char c)
{
	return static_cast<int24_t>(c) - '0';
}