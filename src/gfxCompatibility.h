#pragma once

class HDpicGFX
{
private:
	// Private constructor to prevent instantiation from outside the class
	HDpicGFX() {}

	// Private copy constructor and assignment operator to prevent copying
	HDpicGFX(const HDpicGFX &) = delete;
	HDpicGFX &operator=(const HDpicGFX &) = delete;

	//once 8 or 16bpp library set, this gets set to true permenantly.
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
		ti_Seek(26, SEEK_SET, palSlot);
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

	static void end()
	{
		if (!bGfxLibSet)
			return;
		if (is16bppMode())
			gfx16_End();
		else
			gfx_End();
	}
};