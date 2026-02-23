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
#include <time.h>
#include <lcddrvce.h>

#include "main.h"
#include "loadingBarHandler.h"
#include "keyPressHandler.h"
#include "memoryHandler.h"
#include "gfxCompatibility.h"
#include "pictureDatabase.h"
#include "globals.h"
#include "guiUtils.h"
#include "hdlib/hdlib.h"

void SetHalfResMode(bool enable)
{
	typedef struct lcd_timing
	{
		uint32_t : 2;
		uint32_t PPL : 6;
		uint32_t HSW : 8;
		uint32_t HFP : 8;
		uint32_t HBP : 8;

		uint32_t LPP : 10;
		uint32_t VSW : 6;
		uint32_t VFP : 8;
		uint32_t VBP : 8;

		uint32_t PCD_LO : 5;
		uint32_t CLKSEL : 1;
		uint32_t ACB : 5;
		uint32_t IVS : 1;
		uint32_t IHS : 1;
		uint32_t IPC : 1;
		uint32_t IOE : 1;
		uint32_t : 1;
		uint32_t CPL : 10;
		uint32_t BCD : 1;
		uint32_t PCD_HI : 5;
	} lcd_timing_t;

	typedef struct res_settings
	{
		uint8_t frctrl;
		uint8_t bp;
		uint16_t xe;
		uint8_t tfa;
		uint8_t ramctrl1;
		lcd_timing_t timing;
	} res_settings_t;

	static res_settings_t settings[2] =
	{
		{
			/* Default ST7789 settings */
			.frctrl = LCD_FRCTRL_DEFAULT,
			.bp = LCD_BP_DEFAULT,
			.xe = LCD_WIDTH - 1,
			.tfa = 0,
			.ramctrl1 = LCD_RAMCTRL1_DEFAULT
		},
		{
			/* With the following ST7789 timing:
			 * Refreshes LCD in at most 16.51 ms after VSYNC, assuming worst case 9.5 MHz clock
			 * Waits at least 3.42 ms after VSYNC to read LCD memory, assuming worst case 10.5 MHz clock
			 */
			.frctrl = LCD_RTN_378 | LCD_NL_DEFAULT, /* 378 clocks per line */
			.bp = 95, /* 95 lines of back porch */
			.xe = HALF_LCD_WIDTH - 1,
			.tfa = HALF_LCD_WIDTH,
			.ramctrl1 = LCD_DM_VSYNC | LCD_RAM_DEFAULT,
			/* With the following PL111 timing:
			 * Refreshes LCD at 60 Hz = 24 MHz / (800*250*2), a VSYNC period of 16.67 ms
			 * Outputs 38400 pixels to LCD memory within the first 3.40 ms after VSYNC
			 */
			.timing =
			{
				.PPL = 768 / 16 - 1, /* 768 pixels per line */
				.HSW = 1 - 1,
				.HFP = (800 - 768 - 1 - 1) - 1, /* 800 total clocks per line */
				.HBP = 1 - 1,
				.LPP = HALF_LCD_WIDTH * LCD_HEIGHT / 768, /* 50 lines */
				.VSW = 1 - 1,
				.VFP = 250 - (HALF_LCD_WIDTH * LCD_HEIGHT / 768) - 1, /* 250 total lines */
				.VBP = 0,
				.PCD_LO = (2 - 2) & 0x1F, /* clock divisor of 2 */
				.CLKSEL = 0,
				.ACB = 0,
				.IVS = 1,
				.IHS = 1,
				.IPC = 1,
				.IOE = 1,
				.CPL = 768 - 1,
				.BCD = 0,
				.PCD_HI = (2 - 2) >> 5
			}
		}
	};

	const res_settings_t *p = &settings[enable];

	/* Initialize LCD driver */
	lcd_Init();

	/* Set clocks per line */
	lcd_SetNormalFrameRateControl(p->frctrl);
	/* Set back porch */
	lcd_SetNormalBackPorchControl(p->bp);
	/* Set horizontal output window */
	lcd_SetColumnAddress(0, p->xe);
	/* Set fixed left scroll area */
	lcd_SetScrollArea(p->tfa, LCD_WIDTH - p->tfa, 0);
	/* Set starting vertical scroll address to 0 */
	lcd_SetScrollAddress(0);
	/* Set display mode */
	lcd_SetRamInterface(p->ramctrl1);
	/* Set interlace mode */
	lcd_SetInterlacedMode(enable);

	/* Save old display timing when enabling */
	if (enable) {
		memcpy(&settings[0].timing, (const void *)&lcd_Timing0, sizeof(settings[0].timing));
	}
	/* Set display timing */
	memcpy((void *)&lcd_Timing0, &p->timing, sizeof(p->timing));

	/* Cleanup LCD driver */
	lcd_Cleanup();
}



int main(void)
{
	dbg_sprintf(dbgout, "\n==START==");

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
	dbg_sprintf(dbgout, "\n quitter 2");

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
	//set up variable that checks if drawMedia failed
	uint8_t imageErr{ 0 };
	PicDatabase &picDB = PicDatabase::getInstance();
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();
	HDpicGFX &gfx = HDpicGFX::getInstance();

	//UI is always 16bpp
	HDpicGFX::use16bpp();

	/* main menu */

	dbg_sprintf(dbgout, "\ndrawMenu_16bpp");
	gfx16_FillScreen(GFX16_BLACK);
	drawMenu_16bpp(selectedPicIndex);


	//thumbnail
	dbg_sprintf(dbgout, "\n drawMedia");
	drawMedia(selectedPicIndex, 180, 120, false);

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
				gfx_SetTextBGColor(1);//todo: fix
				gfx_SetTextFGColor(2);
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
				gfx16_FillScreen(GFX16_BLACK);

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

			if (prev16bpp)
				HDpicGFX::use16bpp();
			else
				HDpicGFX::use8bpp();

			gfx16_FillScreen(GFX16_BLACK);


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
			if (fullScreenImage)
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
			if (fullScreenImage)
				gfx.autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);
			resetPic = fullScreenImage;
			redrawPic = true;
			errorID = kb_KeyYequ; //272 if an error is thrown, then we've scrolled past the safety barrier somehow.
		}

		//left, right, up, down. Image panning.
		if (fullScreenImage) {
			if (keyHandler.wasKeyPressed(kb_KeyLeft)) {
				errorID = kb_KeyLeft; //1794
				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyRight)) {
				errorID = kb_KeyRight; //1796
				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, -1, 0);
			}
			if (keyHandler.wasKeyPressed(kb_KeyUp)) {
				errorID = kb_KeyUp; //1800
				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, -1);
			}
			if (keyHandler.wasKeyPressed(kb_KeyDown)) {
				errorID = kb_KeyDown; //1793
				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true, 0, 1);
			}

			//Zoom key. Zoom in as far as possible while maintaining full quality
			if (keyHandler.wasKeyPressed(kb_KeyZoom)) {
				//pull image full dimensions from database
				PicDatabase &picDB = PicDatabase::getInstance();

				//convert subimg width to pixels width
				uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
				desiredWidthInPxl = picDB.getPicture(selectedPicIndex).horizSubImages * SUBIMAGE_DIMENSIONS;
				desiredHeightInPxl = picDB.getPicture(selectedPicIndex).vertSubImages * SUBIMAGE_DIMENSIONS;

				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					desiredWidthInPxl = prevWidth;
					desiredHeightInPxl = prevHeight;
					dbg_sprintf(dbgout, "\nCant zoom out. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
					redrawPic = true;
				}
				errorID = kb_KeyZoom; //260
			}

			//Plus key. Zoom in by double
			if (keyHandler.wasKeyPressed(kb_KeyAdd)) {
				uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
				//calculate increased zoom
				desiredWidthInPxl = desiredWidthInPxl * ZOOM_SCALE;
				desiredHeightInPxl = desiredHeightInPxl * ZOOM_SCALE;
				dbg_sprintf(dbgout, "\n\n--KEYPRESS--\n Zoom In\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
				//if (desiredWidthInPxl != 0 && desiredHeightInPxl != 0) {

				imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
				//this means we can't zoom in any more. Zoom back out.
				if (imageErr != 0) {
					desiredWidthInPxl = prevWidth;
					desiredHeightInPxl = prevHeight;
					dbg_sprintf(dbgout, "\nCant zoom out. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
					redrawPic = true;
				}
				errorID = kb_KeyAdd; //1538
			}

			//subtract key.Zoom out by double.
			if (keyHandler.wasKeyPressed(kb_KeySub)) {
				//apply the zoom out to the width and height
				uint24_t prevWidth{ desiredWidthInPxl }, prevHeight{ desiredHeightInPxl };
				desiredWidthInPxl = desiredWidthInPxl / ZOOM_SCALE;
				desiredHeightInPxl = desiredHeightInPxl / ZOOM_SCALE;

				if (desiredWidthInPxl != 0 && desiredHeightInPxl != 0) {
					dbg_sprintf(dbgout, "\n desiredWidthInPxl: %d\n desiredHeightInPxl: %d ", desiredWidthInPxl, desiredHeightInPxl);
					imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, true);
					//this means we can't zoom out any more. Zoom back in.
					if (imageErr != 0) {
						desiredWidthInPxl = prevWidth;
						desiredHeightInPxl = prevHeight;
						dbg_sprintf(dbgout, "\nCant zoom out. Reverting to %d x %d...", desiredWidthInPxl, desiredHeightInPxl);
						redrawPic = true;
					}
				}
				errorID = kb_KeySub; //1540
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
			//this can cover up other errors so append it
			errorID = errorID * 1000 + kb_KeyWindow; //264
		}
		dbg_sprintf(dbgout, "\n test 2");

		// If necessary, draw the image with new settings.
		if (redrawPic) {
			// change gfx libraries, if necessary.
			dbg_sprintf(dbgout, "\n test 1");

			if (fullScreenImage)
				HDpicGFX::autoSelectLibrary(picDB.getPicture(selectedPicIndex).BPP);
			else {
				gfx16_SetColor(GFX16_BLACK);
				gfx16_FillRectangle(136, 0, 184, 240);
				drawMenu_16bpp(selectedPicIndex);
			}

			keyHandler.reset();
			imageErr = drawMedia(selectedPicIndex, desiredWidthInPxl, desiredHeightInPxl, fullScreenImage);
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
			drawWatermark_16bpp();
		}
	} while (!quitProgram);
	dbg_sprintf(dbgout, "\n quitter 1");

}

// Draws animated frames. Changes draw to the buffer.
uint8_t drawGIF(uint24_t picName, bool fullScreenPic)
{
	dbg_sprintf(dbgout, "GIF --");

	PicDatabase &picDB = PicDatabase::getInstance();
	imageData &curPicture = picDB.getPicture(picName);
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();
	HDpicGFX &gfx = HDpicGFX::getInstance();
	HDpicGFX::useGIFMode();
	if (fullScreenPic) {
		HDpicGFX::use8bpp(); //GIF is always 8bpp
	}
	else {
		HDpicGFX::use16bpp(); //thumbnail is always 16bpp
	}

	MemHandler &mem = MemHandler::getInstance();
	mem.useGifMemory();

	//requires 8bpp mode
	char palName[9];
	sprintf(palName, "HP%.2s0000", curPicture.ID);



	//allocate memory for resized image
	//gfx_rletsprite_t *srcGif{ nullptr };
	gfx_sprite_t **srcGif{ nullptr };
	srcGif = &mem.allocation.gif.thumbnail;
	if (srcGif == nullptr) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate srcGif memory!");
		return 1;
	}


	const uint24_t finalFrame{ curPicture.numGIFFrames - 1 };
	const uint24_t x = fullScreenPic ? 0 : 160;
	const uint24_t y = fullScreenPic ? 0 : 80;

	// Local references for faster access
	auto &frames = curPicture.framesPtrList;
	auto &delays = curPicture.framesDelayMSlist;
	KeyPressHandler &kh = keyHandler;

	// Find first valid frame index
	uint24_t curFrame{ 0 };
	if (curPicture.numGIFFrames == 0)
		return 1;

	while (frames[curFrame] == nullptr) {
		if (++curFrame > finalFrame)
			curFrame = 0;
		if (kh.scanKeys(fullScreenPic))
			return 0;
	}


	//thumbnail only shows first frame
	if (!fullScreenPic) {
		lz4_Decompress(static_cast<void *>(*srcGif), curPicture.framesPtrList[curFrame]);//pre-decompress first frame

		ti_var_t palSlot{ ti_Open(palName,"r") };
		if (!palSlot) {
			dbg_sprintf(dbgout, "\nERR: Couldn't find gif palette to convert!");
			return 0;
		}

		// skips past palette header
		ti_Seek(gfx.getPaletteHeaderSize(), SEEK_SET, palSlot);
		gfx_sprite_t *outputImg{ static_cast<gfx_sprite_t *>(mem.getFreeMemoryPtr()) };
		outputImg->width = GIF_SRC_WIDTH;
		outputImg->height = GIF_SRC_HEIGHT;
		gfx16_Sprite8bppTo16bpp(ti_GetDataPtr(palSlot), *srcGif, outputImg);

		HDpicGFX::sprite(outputImg, x, y, false);

		ti_Close(palSlot);
		return 0;
	}

	if (!gfx.usePalette(palName, 512)) {
		PrintCenteredX(palName, 110);
		PrintCenteredX("ERR: Palette does not exist!", 120);
		PrintCenteredX("Image may have recently been deleted.", 130);
		PrintCenteredX("Try restarting the program.", 140);
		KeyPressHandler::waitForAnyKey();
		return 1;
	}
	gfx_SetTransparentColor(GIF_TRANSPARENT_COLOR);

	// Cover up the last image
	gfx16_FillScreen(GFX16_BLACK);
	dbg_sprintf(dbgout, "\nis 16bpp %d", gfx.is16bppMode() ? 1 : 0);

	SetHalfResMode(true);
	//dbg_WatchpointSet(*srcGif, 1, DBG_WATCHPOINT_ALL);
	lz4_Decompress(*srcGif, curPicture.framesPtrList[curFrame]);//pre-decompress first frame
	//gfx_SetDrawBuffer();
	clock_t frameTimer{ clock() };
	while (!keyHandler.isAnyKeyPressed()) {
		// Display the already-decompressed curFrame frame
		dbg_sprintf(dbgout, "\nTimes: ");

		const clock_t spriteTimer{ clock() };
		hdl_HalfResSprite_NoClip(*srcGif);
		//gfx_TransparentSprite_NoClip(*srcGif, 0, 0);
		dbg_sprintf(dbgout, "\n Sprite: %lu ticks", clock() - spriteTimer);

		// Find next valid frame index (wrap-around)
		uint24_t next{ curFrame };
		do {
			if (++next > finalFrame)
				next = 0;
			if (keyHandler.scanKeys(fullScreenPic)) {
				SetHalfResMode(false);
				return 0;
			}
		} while (frames[next] == nullptr);

		// Decompress next frame
		const clock_t compTimer{ clock() };
		lz4_Decompress(*srcGif, frames[next]);
		dbg_sprintf(dbgout, "\n Decompression: %lu ticks", clock() - compTimer);


		// Wait until current frame's delay has elapsed. Press any key to skip.
		dbg_sprintf(dbgout, "\n Finished in: %lu / %d ticks", clock() - frameTimer, delays[curFrame]);
		while (clock() - frameTimer < static_cast<clock_t>(delays[curFrame])) {
			if (os_GetCSC())
				break;
		}
		// Move to next frame, reset timer
		frameTimer = clock();
		curFrame = next;
	}
	SetHalfResMode(false);
	return 0;
}

uint8_t drawMedia(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool fullScreenPic, int8_t shiftX, int8_t shiftY)
{
	dbg_sprintf(dbgout, "\n\n-- DRAWING: ");

	PicDatabase &picDB = PicDatabase::getInstance();
	imageData &curPicture = picDB.getPicture(picName);

	if (curPicture.isGIF) {
		return drawGIF(picName, fullScreenPic);
	}
	else {
		return drawImage(picName, desiredWidthInPxl, desiredHeightInPxl, fullScreenPic, shiftX, shiftY);
	}
}

/* Draws the image stored in database at position picName.
* Resizes the image while maintaining aspect ratio to fit desiredWidthInPxl and desiredHeightInPxl
* fullScreenPic: when false, assumes displaying thumbnails and uses 16bpp. If true, will black out the screen and maintain bpp mode.
* Draws the image at location shiftX,shiftY starting at top left corner.

* Image will automatically be resized to same aspect ratio so you just set the max width and height (4,3 will fit the screen normally)
* If successful, returns 0. Otherwise returns 1
*/
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool fullScreenPic, int8_t shiftX, int8_t shiftY)
{
	dbg_sprintf(dbgout, "PICTURE --");
	PicDatabase &picDB = PicDatabase::getInstance();
	imageData &curPicture = picDB.getPicture(picName);
	KeyPressHandler &keyHandler = KeyPressHandler::getInstance();
	MemHandler &mem = MemHandler::getInstance();
	HDpicGFX &gfx = HDpicGFX::getInstance();
	HDpicGFX::usePictureMode();

	//thumbnails are always 16bpp
	//Convert 8bpp thumbnails to 16bpp
	const bool bConvertTo16bpp{ !fullScreenPic && curPicture.BPP != 16 };
	if (bConvertTo16bpp)
		gfx.use16bpp();
	else
		HDpicGFX::autoSelectLibrary(curPicture.BPP);

	gfx_sprite_t **srcImg = { nullptr };  //Appvar data initially stored here
	gfx_sprite_t **tempImg = { nullptr }; //If 1,2, or 4bpp, we'll need to bit-unpacked to here.

	if (gfx.is16bppMode()) {
		mem.use16bppMemory();
		srcImg = &mem.allocation.picture16bpp.srcImg;
		tempImg = &mem.allocation.picture16bpp.tempImg;

		if (!srcImg || !tempImg)
			return 1;
		//manually set width and ehight for 16bpp to account for library bug
		(*srcImg)->width = SUBIMAGE_DIMENSIONS;
		(*srcImg)->height = SUBIMAGE_DIMENSIONS;
		(*tempImg)->width = SUBIMAGE_DIMENSIONS;
		(*tempImg)->height = SUBIMAGE_DIMENSIONS;
	}
	else {
		mem.use8bppMemory();
		srcImg = &mem.allocation.picture8bpp.srcImg;
		tempImg = &mem.allocation.picture8bpp.tempImg;

		if (!srcImg || !tempImg)
			return 1;
	}


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
	//requires 8bpp picture
	char palName[9];
	if (!gfx.is16bppMode() || bConvertTo16bpp) {
		sprintf(palName, "HP%.2s0000", curPicture.ID);
		uint24_t iEntries = (1u << curPicture.BPP) * 2u;
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
		if (HDpicGFX::is16bppMode())
			gfx16_SetColor(GFX16_BLACK);
		else {
			gfx_SetColor(1);
			gfx_SetDraw(gfx_buffer);
		}

		// Shift screen to right
		if (shiftX > 0) {
			bReverseDirection = true;
			bDrawVertical = true;
			dbg_sprintf(dbgout, "\nInfo, right. \n x1: %d y1: %d \n x2: %d y2: %d \n w: %d h: %d", 0, 0, subimgScaledDim, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);

			HDpicGFX::copyRectangle(0, 0, subimgScaledDim, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
			HDpicGFX::fillRectangle(0, 0, subimgScaledDim, LCD_HEIGHT, false);
		}
		// Shift screen to left
		if (shiftX < 0) {
			bReverseDirection = false;
			bDrawVertical = true;
			dbg_sprintf(dbgout, "\nInfo, left. \n x1: %d y1: %d \n x2: %d y2: %d \n w: %d h: %d", subimgScaledDim, 0, 0, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);

			HDpicGFX::copyRectangle(subimgScaledDim, 0, 0, 0, (LCD_WIDTH - subimgScaledDim), LCD_HEIGHT);
			HDpicGFX::fillRectangle(LCD_WIDTH - subimgScaledDim, 0, subimgScaledDim, LCD_HEIGHT, false);
		}
		// Shift screen up
		if (shiftY > 0) {
			bReverseDirection = false;
			bDrawVertical = false;
			dbg_sprintf(dbgout, "\nInfo, up. \n x1: %d y1: %d \n x2: %d y2: %d \n w: %d h: %d", 0, subimgScaledDim, 0, 0, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));

			HDpicGFX::copyRectangle(0, subimgScaledDim, 0, 0, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
			HDpicGFX::fillRectangle(0, LCD_HEIGHT - subimgScaledDim, LCD_WIDTH, subimgScaledDim, false);
		}
		// Shift screen down
		if (shiftY < 0) {
			bReverseDirection = true;
			bDrawVertical = false;
			dbg_sprintf(dbgout, "\nInfo, down. \n x1: %d y1: %d \n x2: %d y2: %d \n w: %d h: %d", 0, 0, 0, subimgScaledDim, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));

			HDpicGFX::copyRectangle(0, 0, 0, subimgScaledDim, LCD_WIDTH, (LCD_HEIGHT - subimgScaledDim));
			HDpicGFX::fillRectangle(0, 0, LCD_WIDTH, subimgScaledDim, false);
		}
		if (!HDpicGFX::is16bppMode()) {
			gfx_Blit(gfx_buffer);
			gfx_SetDraw(gfx_screen);
		}
	}
	else if (fullScreenPic) {
		//If there's no panning, then we need to re-draw the entire image. 
		gfx16_FillScreen(GFX16_BLACK);
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
		//thumbnails are always 16bpp
		gfx16_SetColor(GFX16_BLACK);
		gfx16_FillRectangle_NoClip(150, 0, 170, 240);
	}

	//find free memory for resized image. Need twice the memory for 16bpp
	const uint24_t iRequiredMem{ static_cast<uint24_t>(subimgScaledDim) * static_cast<uint24_t>(subimgScaledDim) * (HDpicGFX::is16bppMode() ? 2 : 1) + 2 };
	if (mem.getFreeMemoryBytes() < iRequiredMem) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate outputImg memory! %d > %d", iRequiredMem, mem.getFreeMemoryBytes());
		return 1;
	}
	dbg_sprintf(dbgout, "\nINFO: outputImg is using %d / %d bytes of free mem.", iRequiredMem, mem.getFreeMemoryBytes());
	gfx_sprite_t *outputImg{ static_cast<gfx_sprite_t *>(mem.getFreeMemoryPtr()) };
	//we manually set the width and height to the correct values later

	dbg_sprintf(dbgout, "\noutptImg \n ptr: %p \n subimgscaldim: %d \n %p", outputImg, subimgScaledDim, &(outputImg->width));


	//pointer to memory where each unsized subimage will be stored
	dbg_sprintf(dbgout, "\nMediaMemory: %p", *srcImg);
	//dbg_sprintf(dbgout, "\n data: %.10s w: %d h: %d", mem.picture8bpp.srcImg->data, mem.picture8bpp.srcImg->width, mem.picture8bpp.srcImg->height);

	if (!*srcImg) {
		dbg_sprintf(dbgout, "\nERR: Failed to allocate srcImg memory!");
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

		//dbg_sprintf(dbgout, "\nLooped.\n xSubimgID: %d @ %d pxl \n ySubimgID: %d @ %d pxl", xSubimgID, subimgPxlPosX, ySubimgID, subimgPxlPosY);

		//a key interrupted output. Quit immediately
		if (kb_On || keyHandler.scanKeys(fullScreenPic)) {
			dbg_sprintf(dbgout, "\nRender aborted!\n");
			return 0;
		}

		// Skip subimages outside screen.
		if (subimgPxlPosX > LCD_WIDTH || subimgPxlPosX < 0 || subimgPxlPosY > LCD_HEIGHT || subimgPxlPosY < 0)
			continue;

		//dbg_sprintf(dbgout, "\nAppVar Name: %.2s%03u%03u", curPicture.ID, xSubimgID, ySubimgID);

		/* Check if subimg exists */

		//Pull pointer to the subimage from the cache
		void *subimgPtr{ nullptr };
		if (!bDisableCache) {
			subimgPtr = curPicture.cache[xSubimgID][ySubimgID];
		}

		//Check for cache miss
		char picAppvarToFind[9];
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
				else
					ti_Seek(16, SEEK_CUR, subimgSlot);

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
		/* We now know the subimg exists */

		//dbg_sprintf(dbgout, "\n CHECK 1: outputImg W x H: %d x %d ptr: %p", outputImg->width, outputImg->height, outputImg);

		/* Decompress subimage into srcImg */
		if (bConvertTo16bpp) {
			// Convert 8bpp thumbnails to 16bpp
			dbg_sprintf(dbgout, "\nINFO: Converting 8bpp to 16bpp. Palette %.8s", palName);
			ti_var_t palSlot{ ti_Open(palName,"r") };
			if (!palSlot) {
				//Reverts to xlibc palette
				dbg_sprintf(dbgout, "\nWARN: Couldn't find thumbnail palette!");
				return 1;
			}
			HDpicGFX &gfx = HDpicGFX::getInstance();
			ti_Seek(gfx.getPaletteHeaderSize(), SEEK_SET, palSlot);			// skips past palette header
			if (curPicture.BPP < 8) {
				// low bpp images must be unpacked before 16bpp conversion
				zx0_Decompress(*srcImg, subimgPtr);
				if (!bitUnpackSprite(*srcImg, *tempImg, curPicture.BPP)) {
					ti_Close(palSlot);
					return 1;
				}
				gfx16_Sprite8bppTo16bpp(static_cast<void *>(ti_GetDataPtr(palSlot)), *tempImg, *srcImg); //store 16bpp conversion to srcImg
			}
			else {
				zx0_Decompress(outputImg, subimgPtr);
				gfx16_Sprite8bppTo16bpp(static_cast<void *>(ti_GetDataPtr(palSlot)), outputImg, *srcImg); //store 16bpp conversion to srcImg
			}
			ti_Close(palSlot);
		}
		else {
			if (curPicture.BPP < 8) {
				// low bpp images must be unpacked before 16bpp conversion
				zx0_Decompress(*tempImg, subimgPtr);
				if (!bitUnpackSprite(*tempImg, *srcImg, curPicture.BPP))
					return 1;
			}
			else {
				zx0_Decompress(*srcImg, subimgPtr);
			}
		}
		/* ^ subimg is now in srcImg ^ */

		/*dbg_sprintf(dbgout, "\nsubImgX: %d\nsubImgY: %d\nsrcImg: %p", subimgPxlPosX, subimgPxlPosY, (void *)&srcImg);
		* dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosX < 0: %d < 0", subimgPxlPosX < 0, subimgPxlPosX);
		* dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosX + subimgScaledDim > LCD_WIDTH: %d > %d", subimgPxlPosX + subimgScaledDim > LCD_WIDTH, subimgPxlPosX + subimgScaledDim, LCD_WIDTH);
		* dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosY < 0: %d < 0", subimgPxlPosY < 0, subimgPxlPosY);
		* dbg_sprintf(dbgout, "\n CHECK %d subimgPxlPosY + subimgScaledDim > LCD_HEIGHT: %d > %d", subimgPxlPosY + subimgScaledDim > LCD_HEIGHT, subimgPxlPosY + subimgScaledDim, LCD_HEIGHT);*/

		/* Scale then Display Subimage */
		outputImg->width = outputImg->height = subimgScaledDim; // set outputImg to the desired width/height
		HDpicGFX::scaleSprite(*srcImg, outputImg);
		//if we are displaying an edge image, clip the subimage. Otherwise don't clip for extra speed.
		bool bClipPicture = subimgPxlPosX < 0 || subimgPxlPosX + subimgScaledDim > LCD_WIDTH || subimgPxlPosY < 0 || subimgPxlPosY + subimgScaledDim > LCD_HEIGHT;
		HDpicGFX::sprite(outputImg, subimgPxlPosX, subimgPxlPosY, bClipPicture);

		//cleans up
		ti_Close(subimgSlot);
	}

	dbg_sprintf(dbgout, "\nDraw Finished.\n");
	return 0;
}

// Unpacks bit-packed data. 
// srcImg: sprite to unpack
// outImg: destination pointer
// bpp: bits per pixel
// returns pointer to outImg
gfx_sprite_t *bitUnpackSprite(gfx_sprite_t *srcImg, gfx_sprite_t *outImg, uint8_t bpp)
{
	uint24_t out{ 0 };
	uint8_t pixelsPerByte = 8 / bpp;
	uint24_t dataToRead = (SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS) / pixelsPerByte;
	dbg_sprintf(dbgout, "\n  INFO: Unpacking %d bpp. Data: %d", bpp, dataToRead);

	switch (bpp) {
	case 1:
		for (size_t i = 0; i < dataToRead; i++) {
			uint8_t byte = static_cast<uint8_t>((srcImg)->data[i]);

			(outImg)->data[out++] = (byte >> 7) & 0x01;
			(outImg)->data[out++] = (byte >> 6) & 0x01;
			(outImg)->data[out++] = (byte >> 5) & 0x01;
			(outImg)->data[out++] = (byte >> 4) & 0x01;
			(outImg)->data[out++] = (byte >> 3) & 0x01;
			(outImg)->data[out++] = (byte >> 2) & 0x01;
			(outImg)->data[out++] = (byte >> 1) & 0x01;
			(outImg)->data[out++] = byte & 0x01;
		}
		break;
	case 2:
		for (size_t i = 0; i < dataToRead; i++) {
			uint8_t byte = static_cast<uint8_t>((srcImg)->data[i]);

			(outImg)->data[out++] = (byte >> 6) & 0x03;
			(outImg)->data[out++] = (byte >> 4) & 0x03;
			(outImg)->data[out++] = (byte >> 2) & 0x03;
			(outImg)->data[out++] = byte & 0x03;
		}
		break;
	case 4:
		for (size_t i = 0; i < dataToRead; i++) {
			uint8_t byte = static_cast<uint8_t>((srcImg)->data[i]);

			(outImg)->data[out++] = (byte >> 4) & 0x0F;
			(outImg)->data[out++] = byte & 0x0F;
		}
		break;
	default:
		dbg_sprintf(dbgout, "\n  ERR: Unknown bpp value: %d", bpp);
		outImg = nullptr;
	}
	return outImg;
}

/* Rebuilds the database of images on the calculator */
uint24_t findPictures()
{
	char *var_name, imgInfo[16];
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
	void *search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_16BPP, OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}
	// find 1,2,4,8 bit pictures
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_8BPP, OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}
	// find GIFs
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_GIF, OS_TYPE_APPVAR)) != NULL) {
		imagesFound++;
		loadingBar.increment();
	}

	PicDatabase &picDB = PicDatabase::getInstance();
	if (!picDB.allImages.init(imagesFound)) {
		dbg_sprintf(dbgout, "\nERR: Could not init picDB.AllImages for %d images", imagesFound);
		return 0;

	}

	loadingBar.resetLoadingBar(imagesFound);

	/* Find and store 16bpp image data */
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_16BPP, OS_TYPE_APPVAR)) != NULL) {

		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t HORIZ_VERT_SIZE{ 3 };
		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };
		constexpr uint8_t HEADER_SIZE{ 19 };

		loadingBar.increment();

		imageData imgData;
		imgData.isGIF = false;
		//finds the name, letter ID, and size of entire image this picture belongs to.
		ti_var_t  firstPic;
		dbg_sprintf(dbgout, "\nfirstPic %.8s", var_name);

		firstPic = ti_Open(var_name, "r");
		//seeks past 7 bytes header + 1 byte placeholder
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

		dbg_sprintf(dbgout, "\nPicture found:\n imgName: %.8s\n ID: %.2s\n subImgHoriz: %d\n subImgVert: %d\n ", imgData.imgName, imgData.ID, imgData.horizSubImages, imgData.vertSubImages);

		picDB.addPicture(imgData);

		//closes pic for next iteration
		ti_Close(firstPic);
	}

	/* Find a store data for 1,2,4,&8bpp images*/
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_8BPP, OS_TYPE_APPVAR)) != NULL) {

		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t HORIZ_VERT_SIZE{ 3 };
		constexpr uint8_t PALETTE_NAME_SIZE{ 8 };
		constexpr uint8_t BITS_PER_PIXEL_SIZE{ 2 };
		constexpr uint8_t HEADER_SIZE{ 18 };
		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };

		loadingBar.increment();

		imageData imgData;
		imgData.isGIF = false;
		//finds the name, letter ID, and size of entire image this palette belongs to.
		ti_var_t  palette;
		dbg_sprintf(dbgout, "\npalette %.8s", var_name);

		palette = ti_Open(var_name, "r");
		//seeks past 7 byte header + 1 unused parameter
		ti_Seek(8, SEEK_CUR, palette);
		//reads the important info
		//e.g. 08poppy___JT003002
		ti_Read(&imgInfo, HEADER_SIZE, 1, palette);

		char charArrImgInfo[HEADER_SIZE];
		char BPPbuffer[BITS_PER_PIXEL_SIZE];
		std::strncpy(charArrImgInfo, imgInfo, HEADER_SIZE);
		dbg_sprintf(dbgout, "\n charArrImgInfo %.18s", charArrImgInfo);
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
		imgData.horizSubImages = charArrToInt(dimBuffer, 3) + 1;
		std::strncpy(dimBuffer, charArrImgInfo + BITS_PER_PIXEL_SIZE + IMAGE_NAME_SIZE + ID_SIZE + HORIZ_VERT_SIZE, HORIZ_VERT_SIZE);
		imgData.vertSubImages = charArrToInt(dimBuffer, 3) + 1;

		//convert the char BPP to a uint8_t
		imgData.BPP = static_cast<uint8_t>(charToInt(BPPbuffer[0]) * 10 + charToInt(BPPbuffer[1]));

		dbg_sprintf(dbgout, "\nPicture found:\n imgName: %.8s\n palName: %.8s", imgData.imgName, imgData.paletteName);
		dbg_sprintf(dbgout, "\n ID: %.2s\n BPP: %d\n subImgHoriz: %d\n subImgVert: %d\n ", imgData.ID, imgData.BPP, imgData.horizSubImages, imgData.vertSubImages);

		picDB.addPicture(imgData);

		//closes palette for next iteration
		ti_Close(palette);
	}


	/* Find a store data for GIFs*/
	search_pos = NULL;
	while ((var_name = ti_DetectVar(&search_pos, SEARCH_HEADER_GIF, OS_TYPE_APPVAR)) != NULL) {
		constexpr uint8_t ID_SIZE{ 2 };
		constexpr uint8_t PALETTE_NAME_SIZE{ 8 };
		constexpr uint8_t GIF_FRAMES_SIZE{ 6 };
		constexpr uint8_t HEADER_SIZE{ 16 };

		constexpr uint8_t IMAGE_NAME_SIZE{ 8 };
		constexpr uint8_t GIF_FRAME_DELAY_SIZE{ 4 };

		loadingBar.increment();

		imageData imgData;
		imgData.isGIF = true;
		//finds the name, letter ID, and size of entire image this palette belongs to.
		ti_var_t  palette{ ti_Open(var_name, "r") };

		//seeks past 7 byte header + 1 unused identifier
		ti_Seek(8, SEEK_CUR, palette);

		//reads the important info
		//e.g. poppy___JT
		ti_Read(&imgInfo, HEADER_SIZE, 1, palette);

		char charArrImgInfo[HEADER_SIZE];
		char numFramesBuffer[GIF_FRAMES_SIZE];
		std::strncpy(charArrImgInfo, imgInfo, HEADER_SIZE);
		std::strncpy(imgData.imgName, charArrImgInfo, IMAGE_NAME_SIZE);
		std::strncpy(imgData.ID, charArrImgInfo + IMAGE_NAME_SIZE, ID_SIZE);
		std::strncpy(numFramesBuffer, charArrImgInfo + IMAGE_NAME_SIZE + ID_SIZE, GIF_FRAMES_SIZE);
		imgData.numGIFFrames = charArrToInt(numFramesBuffer, GIF_FRAMES_SIZE) + 1;;//+1 because first image starts at 0
		std::strncpy(imgData.paletteName, var_name, PALETTE_NAME_SIZE);
		//dbg_sprintf(dbgout, "\n charArrImgInfo\n BPP: %.16s", charArrImgInfo);

		imgData.imgName[8] = '\0';
		imgData.paletteName[8] = '\0';
		imgData.ID[2] = '\0';
		imgData.BPP = 8;//GIF is always 8bpp

		dbg_sprintf(dbgout, "\n\nGIF found:\n gifName: %.8s\n palName: %.8s", imgData.imgName, imgData.paletteName);
		dbg_sprintf(dbgout, "\n ID: %.2s\n Frames string: %.6s\n Frames int   : %d\n", imgData.ID, numFramesBuffer, imgData.numGIFFrames);

		//dbg_sprintf(dbgout, "\nCaching frame pointers...");
		if (imgData.numGIFFrames == MAX_UINT) {
			dbg_sprintf(dbgout, "\n ERR: Invalid frame length!");
			ti_Close(palette);
			continue;
		}
		//dbg_sprintf(dbgout, "\n framesDelayListSize = %d", sizeof(uint24_t) * imgData.numGIFFrames);

		if (!imgData.framesPtrList.init(imgData.numGIFFrames)) {
			dbg_sprintf(dbgout, "\n ERR: Not enough mem for frame pointers!");
			ti_Close(palette);
			continue;
		}
		if (!imgData.framesDelayMSlist.init(imgData.numGIFFrames)) {
			dbg_sprintf(dbgout, "\n ERR: Not enough mem for frame delay!");
			ti_Close(palette);
			continue;
		}
		loadingBar.resetLoadingBar(imgData.numGIFFrames);

		for (uint24_t i{ 0 }; i < imgData.numGIFFrames; i++) {
			loadingBar.increment();
			char result[GIF_FRAMES_SIZE + 1];//+1 to account for needing \0
			sprintf(result, "%06d", i);

			char picAppvarToFind[9]; //combines the separate parts into one name to search for
			sprintf(picAppvarToFind, "%.2s%.6s", imgData.ID, result);
			picAppvarToFind[8] = '\0';
			//dbg_sprintf(dbgout, "\n gifAppvarToFind: %s", picAppvarToFind);

			ti_var_t subimgSlot = NULL;
			subimgSlot = ti_Open(picAppvarToFind, "r");
			if (subimgSlot) {
				char frameDelay[4];
				ti_Read(&frameDelay, GIF_FRAME_DELAY_SIZE, 1, subimgSlot);
				//dbg_sprintf(dbgout, "\n FrameDelay string: %.4s", frameDelay);
				const uint24_t delayBuffer{ static_cast<uint24_t>(
					charToInt(frameDelay[0]) * 1000 +
					charToInt(frameDelay[1]) * 100 +
					charToInt(frameDelay[2]) * 10 +
					charToInt(frameDelay[3])) };
				//dbg_sprintf(dbgout, "\n FrameDelay int   : %d = %d ms -> index %d", delayBuffer, delayBuffer * 32, i);
				imgData.framesDelayMSlist[i] = delayBuffer * 32;//The CE does 32.768 clocks per millisecond

				//seek past frame delay and placeholder byte
				ti_Seek(GIF_FRAME_DELAY_SIZE + 1, SEEK_SET, subimgSlot);

				//cache the pointer to the image data
				void *subimgPtr{ ti_GetDataPtr(subimgSlot) };
				imgData.framesPtrList[i] = subimgPtr;

				ti_Close(subimgSlot);
			}
			else {
				//subimage does not exist, display error image
				dbg_sprintf(dbgout, "\nERR: GIF frame doesn't exist: %s", picAppvarToFind);
				imgData.framesDelayMSlist[i] = 0;
				imgData.framesPtrList[i] = nullptr;
				continue;
			}
		}
		picDB.addPicture(imgData);

		//closes palette for next iteration
		ti_Close(palette);
	}

	drawSplashScreen();
	dbg_sprintf(dbgout, "\nPics Detected: %d", imagesFound);

	MemHandler &mem = MemHandler::getInstance();
	mem.lockCache();

	return imagesFound;
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
static inline int24_t ceilDiv(int24_t x, int24_t y)
{
	return (x + y - 1) / y;
}

//converts number character to int24_t i.e. '5' -> 5
static inline int24_t charToInt(char c)
{
	return static_cast<int24_t>(c) - '0';
}

uint24_t charArrToInt(const char *s, uint8_t size)
{
	uint24_t result = 0;
	for (uint8_t i{ 0 }; i < size; i++)
		result = result * 10 + (s[i] - '0');

	return result;
}
