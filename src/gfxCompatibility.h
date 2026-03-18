#pragma once
#include "debug.h"
#include <fileioc.h>

class HDpicGFX
{
private:
	// Private constructor to prevent instantiation from outside the class
	HDpicGFX() {}

	// Private copy constructor and assignment operator to prevent copying
	HDpicGFX(const HDpicGFX &) = delete;
	HDpicGFX &operator=(const HDpicGFX &) = delete;

	//GIF and Pictures have different palette header sizes. 
	inline static const uint8_t IMG_PALETTE_HEADER_SIZE = 26;
	inline static const uint8_t GIF_PALETTE_HEADER_SIZE = 24;
	inline static uint8_t paletteHeaderSize = IMG_PALETTE_HEADER_SIZE;

	//once 8 or 16bpp library set, this gets set to true.
	inline static bool bGfxLibSet = false;
	inline static bool b16bppModeEnabled = false;
	//last used palette is stored here
	inline static char cPalette[9] = "";
	inline static uint24_t iPaletteEntries = 0;
	inline static bool bGIFmode = false;
	inline static bool bHalfResMode = false;

	static void SetHalfResMode()
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

		const res_settings_t *p = &settings[bHalfResMode];

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
		lcd_SetInterlacedMode(bHalfResMode);

		/* Save old display timing when enabling */
		if (bHalfResMode) {
			memcpy(&settings[0].timing, (const void *)&lcd_Timing0, sizeof(settings[0].timing));
		}
		/* Set display timing */
		memcpy((void *)&lcd_Timing0, &p->timing, sizeof(p->timing));

		/* Cleanup LCD driver */
		lcd_Cleanup();
	}


public:
	static HDpicGFX &getInstance()
	{
		static HDpicGFX instance; 
		return instance;
	}

	static void useGIFMode()
	{
		bGIFmode = true;
		paletteHeaderSize = GIF_PALETTE_HEADER_SIZE;
	}

	static void usePictureMode()
	{
		bGIFmode = false;
		paletteHeaderSize = IMG_PALETTE_HEADER_SIZE;
	}

	// Sets LCD timings/settings to use 160 x 120
	static void useHalfResMode()
	{
		if (!bHalfResMode)
		{
			bHalfResMode = true;
			SetHalfResMode();
		}
	}

	// Sets LCD timings/settings to use 320 x 240 (default)
	static void useFullResMode()
	{
		if (bHalfResMode)
		{
			bHalfResMode = false;
			SetHalfResMode();
		}
	}

	//Returns true if gfx16 library in use
	static bool is16bppMode()
	{
		return b16bppModeEnabled;
	}

	//Returns size of palette header in bytes
	static uint8_t getPaletteHeaderSize()
	{
		return paletteHeaderSize;
	}

	//16bpp pictures must use 16bpp mode
	static void use16bpp()
	{
		//if no graphics library set, just enable 16bpp
		if (!bGfxLibSet) {
			bGfxLibSet = true;
			gfx16_Begin();
			b16bppModeEnabled = true;
			dbg_sprintf(dbgout, "\nINFO: Started with 16bpp");

			return;
		}
		//if 8bpp library in use, end it.
		//edits transparent color
		if (!b16bppModeEnabled) {
			gfx_End();
			gfx16_Begin();
			gfx16_SetTextTransparentColor(0xfffe);
			b16bppModeEnabled = true;
			dbg_sprintf(dbgout, "\nINFO: Changed to 16bpp");
			return;
		}
		dbg_sprintf(dbgout, "\nINFO: Already 16bpp");

		//if 16bpp already enabled, no need to do anything.
	}

	//1, 2, 4, & 8bpp pictures must use 8bpp mode
	static void use8bpp()
	{
		//if no graphics library set, just enable 8bpp
		if (!HDpicGFX::bGfxLibSet) {
			bGfxLibSet = true;
			gfx_Begin();
			b16bppModeEnabled = false;
			dbg_sprintf(dbgout, "\nINFO: Started with 8bpp");
			return;
		}
		//if 16bpp library in use, end it.
		//edits transparent color
		if (HDpicGFX::b16bppModeEnabled) {
			gfx16_End();
			gfx_Begin();
			HDpicGFX::usePalette(cPalette, iPaletteEntries);
			b16bppModeEnabled = false;
			dbg_sprintf(dbgout, "\nINFO: Changed to 8bpp");
			return;
		}
		dbg_sprintf(dbgout, "\nINFO: Already 8bpp");

		//if 8bpp already enabled, no need to do anything.
	}

	//set the default palette for when 8bpp mode is active
	//Returns false and uses xlibc palette if parameters are invalid.
	static bool usePalette(char palName[9], uint24_t iEntries)
	{
		palName[8] = '\0';//avoid invalid input
		dbg_sprintf(dbgout, "\nINFO: Switching to palette %.8s, entries: %d", palName, iEntries);
		ti_var_t palSlot{ ti_Open(palName,"r") };
		if (!palSlot) {
			//Reverts to xlibc palette
			dbg_sprintf(dbgout, "\nWARN: Using xlibc palette!");
			gfx_SetDefaultPalette(gfx_mode_t::gfx_8bpp);
			std::strncpy(cPalette, "\0\0\0\0\0\0\0\0", 9);
			iEntries = 0;
			return false;
		}

		// skips past palette header
		ti_Seek(HDpicGFX::paletteHeaderSize, SEEK_SET, palSlot);
		gfx_SetPalette(ti_GetDataPtr(palSlot), iEntries, 0);
		ti_Close(palSlot);
		std::strncpy(cPalette, palName, 9);
		HDpicGFX::iPaletteEntries = iEntries;
		return true;
	}

	//given a bpp level, chooses either 8bpp gfx or 16bpp gfx
	static void autoSelectLibrary(uint8_t bpp)
	{
		if (bpp <= 8)
			use8bpp();
		else
			use16bpp();
	}

	//quits the correct gfx library, if necessary
	static void end()
	{
		if (!bGfxLibSet)
			return;
		if (is16bppMode())
			gfx16_End();
		else
			gfx_End();
		bGfxLibSet = false;
	}

	// Resize picture to new dimensions
	static void scaleSprite(gfx_sprite_t *src, gfx_sprite_t *dst)
	{
		dbg_sprintf(dbgout, "\n Scaling... %d x %d to %d x %d", src->width, src->height, dst->width, dst->height);

		if (dst->width != dst->height) {
			dbg_sprintf(dbgout, "\n  ERR: Not square. Reverting to 80x80");
			dst->width = 80; dst->height = 80;
		}
		if (HDpicGFX::is16bppMode())
			gfx16_ScaleSprite(src, dst);
		else
			gfx_ScaleSprite(src, dst);
	}

	// Draw clipped or unclipped sprite. Uses currently set bpp mode.
	static void sprite(gfx_sprite_t *picture, uint24_t x, uint24_t y, bool bClipPicture = true)
	{
		if (bClipPicture) {
			if (is16bppMode())
				gfx16_Sprite(picture, x, y);
			else
				gfx_Sprite(picture, x, y);
		}
		else {
			if (is16bppMode())
				gfx16_Sprite_NoClip(picture, x, y);
			else
				gfx_Sprite_NoClip(picture, x, y);
		}
	}

	// Draw clipped or unclipped rectangle. Uses currently set bpp mode.
	static void fillRectangle(uint24_t x, uint24_t y, uint24_t width, uint24_t height, bool bClip = true)
	{
		if (bClip) {
			if (is16bppMode())
				gfx16_FillRectangle(x, y, width, height);
			else
				gfx_FillRectangle(x, y, width, height);
		}
		else {
			if (is16bppMode())
				gfx16_FillRectangle_NoClip(x, y, width, height);
			else
				gfx_FillRectangle_NoClip(x, y, width, height);
		}
	}

	// Copies portion of screen from one place to another. Uses currently set bpp mode.
	static void copyRectangle(uint24_t srcX, uint24_t srcY, uint24_t dstX, uint24_t dstY, uint24_t width, uint8_t height, gfx_location_t srcBuffer = gfx_screen, gfx_location_t dstBuffer = gfx_buffer)
	{
		if (is16bppMode())
			gfx16_CopyRectangle(srcX, srcY, dstX, dstY, width, height);
		else
			gfx_CopyRectangle(srcBuffer, dstBuffer, srcX, srcY, dstX, dstY, width, height);
	}
};