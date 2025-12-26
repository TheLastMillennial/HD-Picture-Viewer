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
			b16bppModeEnabled = false;
			dbg_sprintf(dbgout, "\nINFO: Changed to 8bpp");
			return;
		}
		dbg_sprintf(dbgout, "\nINFO: Already 8bpp");

		//if 8bpp already enabled, no need to do anything.
	}

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