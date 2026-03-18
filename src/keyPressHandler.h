#pragma once

#include "globals.h"
#include <keypadc.h>

class KeyPressHandler
{
private:
	// the most amount of keypresses to be stored. If we need more than 8 then something has gone wrong.
	static const uint8_t CACHE_SIZE{ 8 };

	// Private constructor to prevent instantiation from outside the class
	KeyPressHandler() {}

	// Private copy constructor and assignment operator to prevent copying
	KeyPressHandler(const KeyPressHandler &) = delete;
	KeyPressHandler &operator=(const KeyPressHandler &) = delete;

	// cache where keypresses will be stored.
	kb_lkey_t arrKeysPressed[CACHE_SIZE] = { NULL };
	uint8_t nextIndex = 0;

	void addKey(kb_lkey_t key)
	{
		if (nextIndex > CACHE_SIZE - 1) {
			dbg_sprintf(dbgout, "\nCan't cache more keypresses!");
			return;
		}
		arrKeysPressed[nextIndex++] = key;
	}

public:
	// Static method to get the instance of the Singleton
	static KeyPressHandler &getInstance()
	{
		static KeyPressHandler instance; // Guaranteed to be created once
		return instance;
	}

	static void waitForAnyKey()
	{
		while (kb_AnyKey() != 0); //wait for key lift
		while (!os_GetCSC()); // wait for key press
	}

	/*void dbg_printKeys()
	{
		for (uint24_t i{ 0 }; i < vecKeysPressed.getSize(); i++)
		{
			dbg_sprintf(dbgout, "\nKey[%d]: %d", i, vecKeysPressed[i]);

		}
	}*/

	//scans for new keypress and stores the pressed keys.
	// Returns true if any key was pressed
	// bInFullscreen will cause certain keypresses to be ignored when false.
	bool scanKeys(bool bInFullscreen = true)
	{
		reset();

		kb_Scan();
		if (kb_IsDown(kb_KeyEnter))
			addKey(kb_KeyEnter);
		if (kb_IsDown(kb_KeyClear))
			addKey(kb_KeyClear);
		if (kb_IsDown(kb_KeyDel))
			addKey(kb_KeyDel);
		if (kb_IsDown(kb_KeyMode))
			addKey(kb_KeyMode);
		if (kb_IsDown(kb_KeyUp))
			addKey(kb_KeyUp);
		if (kb_IsDown(kb_KeyDown))
			addKey(kb_KeyDown);
		if (kb_IsDown(kb_KeyGraph))
			addKey(kb_KeyGraph);
		if (kb_IsDown(kb_KeyYequ))
			addKey(kb_KeyYequ);
		if (bInFullscreen) {
			if (kb_IsDown(kb_KeyLeft))
				addKey(kb_KeyLeft);
			if (kb_IsDown(kb_KeyRight))
				addKey(kb_KeyRight);

			if (kb_IsDown(kb_KeyWindow))
				addKey(kb_KeyWindow);
			if (kb_IsDown(kb_KeyZoom))
				addKey(kb_KeyZoom);
			if (kb_IsDown(kb_KeyAdd))
				addKey(kb_KeyAdd);
			if (kb_IsDown(kb_KeySub))
				addKey(kb_KeySub);
		}

		// If a valid key was pressed, wait for it to be lifted
		if (nextIndex != 0) {
			while (kb_AnyKey() != 0);//wait for key lift
			return true;
		}
		return false;

	}

	//check if specific key was pressed
	bool wasKeyPressed(kb_lkey_t key)
	{

		for (uint8_t i{ 0 }; i < nextIndex; i++) {
			if (arrKeysPressed[i] == key)
				return true;
		}
		return false;
	}

	bool isAnyKeyPressed()
	{
		return nextIndex != 0;
	}

	void reset()
	{
		nextIndex = 0;
		//todo: this loop may not be necessary
		for (uint8_t i{ 0 }; i < CACHE_SIZE; i++) {
			arrKeysPressed[i] = NULL;
		}
	}
};