#pragma once

#include <globals.h>
#include <graphx.h>

class MemHandler
{


private:

	// Private constructor to prevent instantiation from outside the class
	MemHandler() {}

	// Private copy constructor and assignment operator to prevent copying
	MemHandler(const MemHandler &) = delete;
	MemHandler &operator=(const MemHandler &) = delete;

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
			//output will be dynamically allocated
		} picture8bpp;

		struct
		{
			gfx_sprite_t *srcImg;
			gfx_sprite_t *tempImg;
			//output will be dynamically allocated
		} picture16bpp;
	};

	/* Allocate maximum required space ONCE */
	static uint8_t memForGif[
		//(320 * 240 + 2) +    
		(GIF_SRC_WIDTH * GIF_SRC_HEIGHT + 2)
	];

	static uint8_t memFor8bpp[
		(SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS + 2) * 2
	];

	static uint8_t memFor16bpp[
		((SUBIMAGE_DIMENSIONS * 2) * SUBIMAGE_DIMENSIONS + 2) * 2
	];


public:
	static union MediaMemory allocation;

	// Static method to get the instance of the Singleton
	static MemHandler &getInstance()
	{
		static MemHandler instance; // Guaranteed to be created once
		return instance;
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
};//namespace MemoryHandler