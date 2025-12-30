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

	inline static const uint8_t PALETTE_HEADER_SIZE = 27;

	//once 8 or 16bpp library set, this gets set to true.
	inline static bool bGfxLibSet = false;
	inline static bool b16bppModeEnabled = false;
	//last used palette is stored here
	inline static char cPalette[9] = "";
	inline static uint24_t iPaletteEntries = 0;


public:
	// Static method to get the instance of the Singleton
	static HDpicGFX &getInstance()
	{
		static HDpicGFX instance; // Guaranteed to be created once
		return instance;
	}

	//Returns true if gfx16 library in use
	static bool is16bppMode()
	{
		return b16bppModeEnabled;
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

		//26 skips past palette header
		ti_Seek(PALETTE_HEADER_SIZE, SEEK_SET, palSlot);
		gfx_SetPalette(ti_GetDataPtr(palSlot), iEntries, 0);
		ti_Close(palSlot);
		std::strncpy(cPalette, palName, 9);
		iPaletteEntries = iEntries;
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
		//dbg_sprintf(dbgout, "\n Drawing... %d x %d", picture->width, picture->height);

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
	// WARNING: gfx16 library has a visual but when copying screen to the right.
	static void copyRectangle(uint24_t srcX, uint24_t srcY, uint24_t dstX, uint24_t dstY, uint24_t width, uint8_t height, gfx_location_t srcBuffer = gfx_screen, gfx_location_t dstBuffer = gfx_buffer)
	{
		if (is16bppMode())
			gfx16_CopyRectangle(srcX, srcY, dstX, dstY, width, height);
		else
			gfx_CopyRectangle(srcBuffer, dstBuffer, srcX, srcY, dstX, dstY, width, height);

	}
};