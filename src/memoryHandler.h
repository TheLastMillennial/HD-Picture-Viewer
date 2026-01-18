#pragma once

#include <globals.h>
#include <graphx.h>


/* MemHandler puts variables in free user memory
* The Setup looks like this:
* -------------------------------------
* Cached sprite pointers     (constant)
* -------------------------------------
* Sprite data              (adjustable)
* -------------------------------------
* Free memory for outputImg    (Random)
* -------------------------------------
*/

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
		dbg_sprintf(dbgout, "\nINFO: Mem 1 m_usedMem %d / %d", m_usedMem, m_totalFreeMem);
	}

	// Private copy constructor and assignment operator to prevent copying
	MemHandler(const MemHandler &) = delete;
	MemHandler &operator=(const MemHandler &) = delete;

	// Location where new data can be stored
	inline static void *m_pFreeMem{ nullptr };
	uint24_t m_totalFreeMem = os_MemChk(&m_pFreeMem);
	// location where new data can be stored once cache is locked
	inline static void *m_ptrAfterCache{ nullptr };
	//Memory used for caching all pics and gifs 
	inline static uint24_t m_usedMemForCache{ 0 };
	//Total user memory used
	inline static uint24_t m_usedMem{ 0 };

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


public:
	inline static union MediaMemory allocation;

	// Static method to get the instance of the Singleton
	static MemHandler &getInstance()
	{
		static MemHandler instance; // Guaranteed to be created once
		return instance;
	}

	// Once we've cached all necessary data, we don't want that memory overwritten.
	// Returns true on success
	// Returns false if already locked
	bool lockCache()
	{
		if (m_ptrAfterCache != nullptr)
			return false;
		m_ptrAfterCache = m_pFreeMem;
		m_usedMemForCache = m_usedMem;
		return true;
	}

	//bool validateMemIntegrity()
	//{
	//	void *pTemp{ nullptr };
	//	//if amount of free mem has changed, that's bad but potentially manageable.
	//	if (uint24_t newFreeMem{ os_MemChk(&pTemp) }; m_totalFreeMem != newFreeMem) {
	//		dbg_sprintf(dbgout, "\nWARN: Free Mem amount has changed from %d to %d", m_totalFreeMem, newFreeMem);
	//		m_totalFreeMem = newFreeMem;
	//	}

	//	//If mem pointer has moved that is catastrophic.
	//	if (pTemp != m_pFreeMem) {
	//		dbg_sprintf(dbgout, "\nWARN: Free Mem Ptr has changed from %p to %p", pTemp, m_pFreeMem);
	//		//return false;
	//	}

	//	dbg_sprintf(dbgout, "\nINFO: Mem check pass.");

	//	return true;
	//}

	//Returns amount of memory available in bytes
	uint24_t getFreeMemoryBytes()
	{
		dbg_sprintf(dbgout, "\nINFO: getFreeMemoryBytes: %d", m_totalFreeMem - m_usedMem);

		return m_totalFreeMem - m_usedMem;
	}

	//Returns the pointer to the amount of free memory remaining.
	void *getFreeMemoryPtr()
	{
		dbg_sprintf(dbgout, "\nINFO: getFreeMemoryPtr: %p", m_pFreeMem);

		return m_pFreeMem;
	}

	// Permenantly reserves an amount of memory.
	// Returns a pointer to that memory.
	void *permaAllocMemory(uint24_t mem)
	{
		dbg_sprintf(dbgout, "\nINFO:\n Before:\n  permaAlloc'ing %d / %d @ %p", mem, m_totalFreeMem - m_usedMem, m_pFreeMem);

		if (m_totalFreeMem - m_usedMem < mem)
			return nullptr;
		m_usedMem += mem;
		void *pPrevFreeMem = m_pFreeMem;
		m_pFreeMem = static_cast<char *>(m_pFreeMem) + m_usedMem;
		dbg_sprintf(dbgout, "\n After:  \n  m_usedMem %d / %d @ %p", m_usedMem, m_totalFreeMem, m_pFreeMem);


		return pPrevFreeMem;
	}

	static void use8bppMemory()
	{
		m_pFreeMem = m_ptrAfterCache;
		m_usedMem = m_usedMemForCache;

		allocation.picture8bpp.srcImg = static_cast<gfx_sprite_t *>(m_pFreeMem);
		allocation.picture8bpp.srcImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture8bpp.srcImg->height = SUBIMAGE_DIMENSIONS;

		const uint24_t iOffset = SUBIMAGE_DIMENSIONS * SUBIMAGE_DIMENSIONS + 2; //2 accounts for storing width and height

		m_pFreeMem = static_cast<char *>(m_pFreeMem) + iOffset;
		m_usedMem += iOffset;

		allocation.picture8bpp.tempImg = static_cast<gfx_sprite_t *>(m_pFreeMem);
		allocation.picture8bpp.tempImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture8bpp.tempImg->height = SUBIMAGE_DIMENSIONS;

		m_pFreeMem = static_cast<char *>(m_pFreeMem) + iOffset;
		m_usedMem += iOffset;
		//dbg_sprintf(dbgout, "\n After 2: usedMem: %d @ %p ", m_usedMem, m_pFreeMem);

	}

	static void use16bppMemory()
	{

		m_pFreeMem = m_ptrAfterCache;
		m_usedMem = m_usedMemForCache;

		const uint24_t iOffset = (SUBIMAGE_DIMENSIONS * 2) * SUBIMAGE_DIMENSIONS + 2; //2 accounts for storing width and height

		allocation.picture16bpp.srcImg = static_cast<gfx_sprite_t *>(m_pFreeMem);
		allocation.picture16bpp.srcImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture16bpp.srcImg->height = SUBIMAGE_DIMENSIONS;
		m_pFreeMem = static_cast<char *>(m_pFreeMem) + iOffset;
		m_usedMem += iOffset;

		allocation.picture16bpp.tempImg = static_cast<gfx_sprite_t *>(m_pFreeMem);
		allocation.picture16bpp.tempImg->width = SUBIMAGE_DIMENSIONS;
		allocation.picture16bpp.tempImg->height = SUBIMAGE_DIMENSIONS;
		m_pFreeMem = static_cast<char *>(m_pFreeMem) + iOffset;
		m_usedMem += iOffset;

		//dbg_sprintf(dbgout, "\n After 2: usedMem: %d @ %p ", m_usedMem, m_pFreeMem);

	}

	static void useGifMemory()
	{
		m_pFreeMem = m_ptrAfterCache;
		m_usedMem = m_usedMemForCache;

		allocation.gif.thumbnail = static_cast<gfx_sprite_t *>(m_pFreeMem);

		const uint24_t iOffset = GIF_SRC_WIDTH * GIF_SRC_HEIGHT + 2; //2 accounts for storing width and height

		m_pFreeMem = static_cast<char *>(m_pFreeMem) + iOffset;
		m_usedMem += iOffset;

		//dbg_sprintf(dbgout, "\n After 2: usedMem: %d @ %p", m_usedMem, m_pFreeMem);


	}
};//namespace MemoryHandler