#pragma once

#include "globals.h"
#include <keypadc.h>
#include "types/vector.h"


class KeyPressHandler
{
private:

	// Private constructor to prevent instantiation from outside the class
	KeyPressHandler() 
	{
		kb_EnableOnLatch();
	}

	// Private copy constructor and assignment operator to prevent copying
	KeyPressHandler(const KeyPressHandler &) = delete;
	KeyPressHandler &operator=(const KeyPressHandler &) = delete;

	Vector<kb_lkey_t> vecKeysPressed;


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

	//scans for new keypress and stores the pressed keys.
	// Returns true if any key was pressed
	// bInFullscreen will cause certain keypresses to be ignored when false.
	bool scanKeys(bool bInFullscreen = true)
	{

		reset();

		kb_Scan();
		if (kb_IsDown(kb_KeyEnter))
			vecKeysPressed.push_back(kb_KeyEnter);
		if (kb_IsDown(kb_KeyClear))
			vecKeysPressed.push_back(kb_KeyClear);
		if (kb_IsDown(kb_KeyDel))
			vecKeysPressed.push_back(kb_KeyDel);
		if (kb_IsDown(kb_KeyMode))
			vecKeysPressed.push_back(kb_KeyMode);
		if (kb_IsDown(kb_KeyUp))
			vecKeysPressed.push_back(kb_KeyUp);
		if (kb_IsDown(kb_KeyDown))
			vecKeysPressed.push_back(kb_KeyDown);
		if (kb_IsDown(kb_KeyGraph))
			vecKeysPressed.push_back(kb_KeyGraph);
		if (kb_IsDown(kb_KeyYequ))
			vecKeysPressed.push_back(kb_KeyYequ);
		if (bInFullscreen) {
			if (kb_IsDown(kb_KeyLeft))
				vecKeysPressed.push_back(kb_KeyLeft);
			if (kb_IsDown(kb_KeyRight))
				vecKeysPressed.push_back(kb_KeyRight);
			
			if (kb_IsDown(kb_KeyWindow))
				vecKeysPressed.push_back(kb_KeyWindow);
			if (kb_IsDown(kb_KeyZoom))
				vecKeysPressed.push_back(kb_KeyZoom);
			if (kb_IsDown(kb_KeyAdd))
				vecKeysPressed.push_back(kb_KeyAdd);
			if (kb_IsDown(kb_KeySub))
				vecKeysPressed.push_back(kb_KeySub);
		}
		
		// If a valid key was pressed, wait for it to be lifted
		if (!vecKeysPressed.isEmpty())
		{
			while (kb_AnyKey() != 0);//wait for key lift
			return true;
		}
		return false;
		
	}

	//check if specific key was pressed
	bool wasKeyPressed(kb_lkey_t key)
	{

		for (uint24_t i{ 0 }; i < vecKeysPressed.getSize(); i++) {

			if (vecKeysPressed[i] == key)
				return true;
		}
		return false;
	}

	bool isAnyKeyPressed()
	{

		return !vecKeysPressed.isEmpty();
	}

	void reset()
	{
		vecKeysPressed.clear();
	}
};