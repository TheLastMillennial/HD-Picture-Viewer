#include "globals.h"

class LoadingBar
{
private:

	uint24_t tasksFinished, tasksToFinish;

	// Private constructor to prevent instantiation from outside the class
	LoadingBar()
	{
		tasksFinished = 0;
		tasksToFinish = 0;
	}

	// Private copy constructor and assignment operator to prevent copying
	LoadingBar(const LoadingBar &) = delete;
	LoadingBar &operator=(const LoadingBar &) = delete;


	//makes a loading bar and fills it in depending on progress made / tasks left
	void draw() const
	{
		constexpr double maxPixelWidth{ 200.0 };
		const double progress = (static_cast<double>(tasksFinished) / static_cast<double>(tasksToFinish)) * maxPixelWidth;

		gfx_SetColor(PALETTE_WHITE);
		gfx_FillRectangle_NoClip(60, 153, progress, 7);

	}

public:
	// Static method to get the instance of the Singleton
	static LoadingBar &getInstance()
	{
		static LoadingBar instance; // Guaranteed to be created once
		return instance;
	}

	// Resets the loading bar back to 0%.
	// Draws the loading bar
	void resetLoadingBar(uint24_t newTasksToFinish = 0)
	{
		tasksFinished = 0;
		tasksToFinish = newTasksToFinish;
		if (newTasksToFinish != 0)
			draw();
	}

	// Increments progress by 1 by default
	// Wont go past 100% completion
	// Draws the loading bar
	void increment(uint24_t amount = 1)
	{
		tasksFinished += amount;
		if (amount > tasksToFinish)
			amount = tasksFinished;
		draw();
	}
};