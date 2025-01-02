#pragma once

#include "globals.h"
#include <keypadc.h>
#include "types/vector.h"

enum keyPress
{
	on,
	enter,
	up,
	down,
	left,
	right,
	clear,
	del,
	mode,
	graph,
	yequ,
	window,
	zoom,
	add,
	sub
};


class KeyPressHandler
{
private:

	// Private constructor to prevent instantiation from outside the class
	KeyPressHandler() {}

	// Private copy constructor and assignment operator to prevent copying
	KeyPressHandler(const KeyPressHandler &) = delete;
	KeyPressHandler &operator=(const KeyPressHandler &) = delete;

	Vector<keyPress> vecKeysPressed;


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
		if (kb_On) //important we return immediately for ON
		{
			kb_ClearOnLatch();
			vecKeysPressed.push_back(keyPress::on);
			return true;
		}
		
		reset();

		kb_Scan();
		if (kb_Data[6] & kb_Enter)
			vecKeysPressed.push_back(keyPress::enter);
		if (kb_Data[6] & kb_Clear)
			vecKeysPressed.push_back(keyPress::clear);
		if (kb_Data[1] & kb_Del)
			vecKeysPressed.push_back(keyPress::del);
		if (kb_Data[1] & kb_Mode)
			vecKeysPressed.push_back(keyPress::mode);
		if (kb_Data[7] & kb_Up)
			vecKeysPressed.push_back(keyPress::up);
		if (kb_Data[7] & kb_Down)
			vecKeysPressed.push_back(keyPress::down);
		if (bInFullscreen)
		{
			if (kb_Data[7] & kb_Left)
				vecKeysPressed.push_back(keyPress::left);
			if (kb_Data[7] & kb_Right)
				vecKeysPressed.push_back(keyPress::right);
			if (kb_Data[1] & kb_Graph)
				vecKeysPressed.push_back(keyPress::graph);
			if (kb_Data[1] & kb_Yequ)
				vecKeysPressed.push_back(keyPress::yequ);
			if (kb_Data[1] & kb_Window)
				vecKeysPressed.push_back(keyPress::window);
			if (kb_Data[1] & kb_Zoom)
				vecKeysPressed.push_back(keyPress::zoom);
			if (kb_Data[6] & kb_Add)
				vecKeysPressed.push_back(keyPress::add);
			if (kb_Data[6] & kb_Sub)
				vecKeysPressed.push_back(keyPress::sub);
			
		}

		while (kb_AnyKey() != 0); //wait for key lift
		return !vecKeysPressed.isEmpty();
	}

	//check if specific key was pressed
	bool wasKeyPressed(keyPress key)
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