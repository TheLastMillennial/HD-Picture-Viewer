#pragma once

#include <cstring>

#include "globals.h"
#include "gfxCompatibility.h"
#include "types/pair.h"
#include "types/map.h"
#include "loadingBarHandler.h"
#include "types/fixedVector.h"

struct imageData
{
	bool isGIF{ false };
	char imgName[9];
	char paletteName[9];
	char ID[3];
	double zoomScale{ 1.0 };
	uint8_t BPP{ 0 };
	int24_t xOffset{ 0 };
	int24_t yOffset{ 0 };
	int24_t horizSubImages{ 0 };
	int24_t vertSubImages{ 0 };
	uint24_t numGIFFrames{ 0 };

	//When we find a subimage, store the pointer to it here.
	Map< uint24_t, Map< uint24_t, void *>> cache;
	//GIFs have their image location pointers stored beforehand

	FixedVector<void *> framesPtrList;
	FixedVector<uint24_t> framesDelayMSlist;
};

class PicDatabase
{
private:

	// Private constructor to prevent instantiation from outside the class
	PicDatabase() {}
	~PicDatabase() {}

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

	FixedVector <imageData> allImages;

	// Resets all memory this db uses
	void resetPicDB()
	{
		allImages.clear();
	}

	uint24_t size()
	{
		return allImages.size();
	}

	static inline void toLower(const char strIn[9], char strOut[9])
	{
		for (int i = 0; i < 9; ++i) {
			if (strIn[i] >= 'A' && strIn[i] <= 'Z') {
				strOut[i] = strIn[i] + ('a' - 'A'); // Convert to lowercase
			}
			else {
				strOut[i] = strIn[i];
			}
		}
	}

	//Add a picture to the database
	void addPicture(imageData const img)
	{

		if (allImages.size() == 1) {
			char left[9], right[9];
			toLower(img.imgName, left);
			toLower(allImages[0].imgName, right);

			int24_t result = std::strcmp(left, right);
			//dbg_sprintf(dbgout, "\n(1)Compare result %d for %s : %s", result, left, right);

			if (result < 0) {
				allImages.insert(0, img);
				//dbg_sprintf(dbgout, "\nCompare: found match");
				return;
			}
		}
		for (int24_t i = 0; i < static_cast<int24_t>(allImages.size()) - 2; i++) {
			char left[9], right[9];
			toLower(img.imgName, left);
			toLower(allImages[i].imgName, right);

			int24_t result = std::strcmp(left, right);
			//dbg_sprintf(dbgout, "\nCompare result %d for %s : %s", result, left, right);
			if (result < 0) {
				allImages.insert(i, img);
				//dbg_sprintf(dbgout, "\nCompare: found match");

				return;
			}
		}
		//dbg_sprintf(dbgout, "\nCompare: no results");
		allImages.push_back(img);
	}

	void *searchCache(imageData &img, uint24_t imgXID, uint24_t imgYID) const
	{
		return img.cache.find(imgXID)->find(imgYID);

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

		//sets up loading bar finish line
		gfx16_SetColor(GFX16_WHITE);
		gfx16_VertLine_NoClip(280, 160, 7);


		if (imgToDelete->isGIF) {
			LoadingBar &loadingBar = LoadingBar::getInstance();
			loadingBar.resetLoadingBar(imgToDelete->numGIFFrames);

			// find the palette
			sprintf(picAppvarToFind, "HP%.2s0000", imgToDelete->ID);
			int delSuccess = ti_Delete(picAppvarToFind);

			//checks if the palette does not exist
			if (delSuccess == 0) {
				//subimage does not exist
				dbg_sprintf(dbgout, "\nERR: Issue deleting palette");
				dbg_sprintf(dbgout, "\nHP%.2s0000", imgToDelete->ID);
			}

			for (uint24_t iFrame = (imgToDelete->numGIFFrames - 1); iFrame < MAX_UINT; iFrame--) {
				//combines the separate parts into one name to search for
				sprintf(picAppvarToFind, "%.2s%06d", imgToDelete->ID, iFrame);
				int delSuccess = ti_Delete(picAppvarToFind);

				//checks if the subimage does not exist
				if (delSuccess == 0) {
					//subimage does not exist
					dbg_sprintf(dbgout, "\nERR: Issue deleting subimage");
					dbg_sprintf(dbgout, "\n%.8s", picAppvarToFind);
				}
				loadingBar.increment();
			}
		}
		else {
			int24_t const &picWidthInSubimages{ imgToDelete->horizSubImages };
			int24_t const &picHeightInSubimages{ imgToDelete->vertSubImages };

			LoadingBar &loadingBar = LoadingBar::getInstance();
			loadingBar.resetLoadingBar(picWidthInSubimages * picHeightInSubimages);

			// 1,2,4, & 8 bpp pictures use a palette
			if (imgToDelete->BPP != 16) {
				// find the palette
				sprintf(picAppvarToFind, "HP%.2s0000", imgToDelete->ID);
				int delSuccess = ti_Delete(picAppvarToFind);

				//checks if the palette does not exist
				if (delSuccess == 0) {
					//subimage does not exist
					dbg_sprintf(dbgout, "\nERR: Issue deleting palette");
					dbg_sprintf(dbgout, "\nHP%.2s0000", imgToDelete->ID);
				}
			}

			//delete every subimage
			for (uint24_t xSubimage = (picWidthInSubimages - 1); xSubimage < MAX_UINT; xSubimage--) {
				for (uint24_t ySubimage = (picHeightInSubimages - 1); ySubimage < MAX_UINT; ySubimage--) {
					//combines the separate parts into one name to search for
					sprintf(picAppvarToFind, "%.2s%03u%03u", imgToDelete->ID, xSubimage, ySubimage);
					int delSuccess = ti_Delete(picAppvarToFind);

					//checks if the subimage does not exist
					if (delSuccess == 0) {
						//subimage does not exist
						dbg_sprintf(dbgout, "\nERR: Issue deleting subimage");
						dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgToDelete->ID, xSubimage, ySubimage);
					}
					loadingBar.increment();
				}
			}
		}
		allImages.removeAt(picName);
	}
};