#include "globals.h"

class LoadingBar {
private:
    // Private constructor to prevent instantiation from outside the class
    LoadingBar() {}

    // Private copy constructor and assignment operator to prevent copying
    LoadingBar(const LoadingBar&) = delete;
    LoadingBar& operator=(const LoadingBar&) = delete;

public:
    // Static method to get the instance of the Singleton
    static LoadingBar& getInstance() {
        static LoadingBar instance; // Guaranteed to be created once
        return instance;
    }

    //makes a loading bar and fills it in depending on progress made / tasks left
    void SetLoadingBarProgress(uint24_t progress, const uint24_t tasks) {
        progress = ((double)progress / (double)tasks) * 200.0;
        //ensures loading bar doesn't go past max point
        if (progress > 200)
            progress = 200;

        gfx_SetColor(PALETTE_WHITE);
        gfx_FillRectangle_NoClip(60, 153, progress, 7);

    }
};


    // Accessing the Singleton instance
    //LoadingBar& s1 = LoadingBar::getInstance();
    //s1.doSomething();
