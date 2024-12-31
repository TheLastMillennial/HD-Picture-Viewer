/* Function Prototypes */
#include "globals.h"
void drawHomeScreen();
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool refreshWholeScreen, int8_t shiftX = 0, int8_t shiftY = 0);
void drawMenu(uint24_t startName);
uint24_t findPictures();
int24_t ceilDiv(int24_t x, int24_t y);
bool iterate(int24_t& xSubimgID, int24_t const& xFirstID, int24_t const& xLastID, int24_t& ySubimgID, int24_t const& yFirstID, int24_t const& yLastID, bool bDrawVertically = true, bool bDrawBackwards = false);
keyPress scanKeys(bool bInFullscreen);