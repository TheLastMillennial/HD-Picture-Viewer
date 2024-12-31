#pragma once

#include "globals.h"
#include "types/vector.h"

class PicDatabase
{
private:

	// Private constructor to prevent instantiation from outside the class
	PicDatabase() {}

	// Private copy constructor and assignment operator to prevent copying
	PicDatabase(const PicDatabase &) = delete;
	PicDatabase &operator=(const PicDatabase &) = delete;


public:
	// Static method to get the instance of the Singleton
	static PicDatabase &getInstance()
	{
		static PicDatabase instance; // Guaranteed to be created once
		return instance;
	}

	Vector <imageData> allImages;

	// Resets all memory this db uses
	void resetPicDB()
	{
		allImages.clear();
	}

	uint24_t size()
	{
		return allImages.getSize();
	}

	//pre-allocate memory for the database (optional but faster than only calling addPicture())
	void reserve(uint24_t size)
	{
		allImages.reserve(size);
	}

	//Add a picture to the database
	void addPicture(imageData const img)
	{
		allImages.push_back(img);
	}

	imageData &getPicture(uint24_t index)
	{
		return allImages[index];
	}

	// Delete image from calculator and remove from allImages
	void deleteImage(uint24_t picName)
	{
		char picAppvarToFind[9];

		imageData *imgToDelete{ &allImages[picName] };

		//deletes palette
		int delSuccess{ ti_Delete(imgToDelete->palletName) };
		if (delSuccess == 0) {
			dbg_sprintf(dbgout, "\nERR: Issue deleting palette");
		}
		//sets up loading bar finish line
		gfx_SetColor(PALETTE_WHITE);
		gfx_VertLine_NoClip(260, 153, 7);

		int24_t const &picWidthInSubimages{ imgToDelete->horizSubImages };
		int24_t const &picHeightInSubimages{ imgToDelete->vertSubImages };

		LoadingBar &loadingBar = LoadingBar::getInstance();
		loadingBar.resetLoadingBar(picWidthInSubimages * picHeightInSubimages);

		//delete every subimage
		for (uint24_t xSubimage = (picWidthInSubimages - 1); xSubimage < MAX_UINT; xSubimage--) {
			for (uint24_t ySubimage = (picHeightInSubimages - 1); ySubimage < MAX_UINT; ySubimage--) {

				//combines the separate parts into one name to search for
				sprintf(picAppvarToFind, "%.2s%03u%03u", imgToDelete->ID, xSubimage, ySubimage);
				delSuccess = ti_Delete(picAppvarToFind);

				//checks if the subimage does not exist
				if (delSuccess == 0) {
					//subimage does not exist
					dbg_sprintf(dbgout, "\nERR: Issue deleting subimage");
					dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgToDelete->ID, xSubimage, ySubimage);
				}
				loadingBar.increment();
			}
		}

		allImages.removeAt(picName);
	}
};