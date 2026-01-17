#pragma once

#include <globals.h>
#include <graphx.h>

class MemHandler
{


private:
	//this is dumb this function doesn't exist already.
	uint24_t maxInt(uint24_t a, uint24_t b)
	{
		return a > b ? a : b;
	}
	// Private constructor to prevent instantiation from outside the class
	MemHandler()
	{
		usedMem += maxInt(MEM_FOR_GIF, maxInt(MEM_FOR_8BPP, MEM_FOR_16BPP));
		dbg_sprintf(dbgout, "\nINFO: usedMem: %d / %d", usedMem, totalFreeMem);

	}

	// Private copy constructor and assignment operator to prevent copying
	MemHandler(const MemHandler &) = delete;
	MemHandler &operator=(const MemHandler &) = delete;

	void *pFreeMem{ nullptr };
	uint24_t totalFreeMem = os_MemChk(&pFreeMem);

	uint24_t usedMem{ 0 };
	//Statically allocate memory for pictures and GIFs. The memory will never be shared across modes.
	union MediaMemory
	{
		//clang bug requires a dummy constructor
		constexpr MediaMemory() : dummy{ false } {}
		bool dummy;

		struct
		{
			gfx_sprite_t *fullscreen;
			gfx_sprite_t *thumbnail;
		} gif;

		struct
		{
			gfx_sprite_t *srcImg;
			gfx_sprite_t *tempImg;
			//output will be allocated base on leftover RAM
		} picture8bpp;

		struct
		{
			gfx_sprite_t *srcImg;
			gfx_sprite_t *tempImg;
			//output will be allocated based on leftover RAM
		} picture16bpp;
	};

	/* Allocate maximum required space ONCE */
	static constexpr uint24_t MEM_FOR_GIF{ (GIF_SRC_WIDTH * GIF_SRC_HEIGHT + 2) };
	static constexpr uint24_t MEM_FOR_8BPP{ (SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS + 2) * 2 };
	static constexpr uint24_t MEM_FOR_16BPP{ ((SUBIMAGE_DIMENSIONS * 2) * SUBIMAGE_DIMENSIONS + 2) * 2 };

	inline static uint8_t memForGif[MEM_FOR_GIF];
	inline static uint8_t memFor8bpp[MEM_FOR_8BPP];
	inline static uint8_t memFor16bpp[MEM_FOR_16BPP];



public:
	inline static union MediaMemory allocation;

	// Static method to get the instance of the Singleton
	static MemHandler &getInstance()
	{
		static MemHandler instance; // Guaranteed to be created once
		return instance;
	}

	bool validateMemIntegrity()
	{
		void *pTemp{ nullptr };
		//if amount of free mem has changed, that's bad but potentially manageable.
		if (uint24_t newFreeMem{ os_MemChk(&pTemp) }; totalFreeMem != newFreeMem)
		{
			dbg_sprintf(dbgout, "\nWARN: Free Mem amount has changed from %d to %d", totalFreeMem, newFreeMem);
			totalFreeMem = newFreeMem;
		}

		//If mem pointer has moved that is catastrophic.
		if (pTemp != pFreeMem) {
			dbg_sprintf(dbgout, "\nWARN: Free Mem Ptr has changed from %p to %p", pFreeMem, pTemp);
			//return false;
		}

		dbg_sprintf(dbgout, "\nINFO: Mem check pass.");

		return true;
	}

	//Returns amount of memory available in bytes
	uint24_t getFreeMemoryBytes()
	{
		if (!validateMemIntegrity())
			return 0;
		dbg_sprintf(dbgout, "\nINFO: getFreeMemoryBytes: %d", totalFreeMem - usedMem);

		return totalFreeMem - usedMem;
	}

	//Returns the pointer to the amount of free memory remaining.
	void* getFreeMemoryPtr()
	{
		dbg_sprintf(dbgout, "\nINFO: getFreeMemoryPtr: %p", pFreeMem);

		return pFreeMem;
	}

	// Permenantly reserves an amount of memory.
	// Returns a pointer to that memory.
	void* permaAllocMemory(uint24_t mem)
	{
		if (totalFreeMem - usedMem < mem)
			return nullptr;
		usedMem += mem;
		void *pPrevFreeMem = pFreeMem;
		pFreeMem = static_cast<char *>(pFreeMem) + usedMem;
		return pPrevFreeMem;
	}

	static void use8bppMemory()
	{
		uint8_t iOffset = 0;
		allocation.picture8bpp.srcImg = (gfx_sprite_t *)&memFor8bpp[iOffset];
		allocation.picture8bpp.srcImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture8bpp.srcImg->height = SUBIMAGE_DIMENSIONS;
		iOffset += SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS + 2; //2 accounts for storing width and height
		allocation.picture8bpp.tempImg = (gfx_sprite_t *)&memFor8bpp[iOffset];
		allocation.picture8bpp.tempImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture8bpp.tempImg->height = SUBIMAGE_DIMENSIONS;
	}

	static void use16bppMemory()
	{
		uint8_t iOffset = 0;
		allocation.picture8bpp.srcImg = (gfx_sprite_t *)&memFor16bpp[iOffset];
		allocation.picture8bpp.srcImg->width = SUBIMAGE_DIMENSIONS * 2;
		allocation.picture8bpp.srcImg->height = SUBIMAGE_DIMENSIONS;
		iOffset += (SUBIMAGE_DIMENSIONS * 2) * SUBIMAGE_DIMENSIONS + 2; //2 accounts for storing width and height
		allocation.picture8bpp.tempImg = (gfx_sprite_t *)&memFor16bpp[iOffset];
		allocation.picture8bpp.tempImg->width = SUBIMAGE_DIMENSIONS * 2;
		allocation.picture8bpp.tempImg->height = SUBIMAGE_DIMENSIONS;
	}

	static void useGifMemory()
	{
		uint8_t iOffset = 0;
		allocation.gif.thumbnail = (gfx_sprite_t *)&memForGif[iOffset];
	}
};//namespace MemoryHandler