/* Function Prototypes */
void drawHomeScreen();
uint8_t drawImage(uint24_t picName, uint24_t maxAllowedWidthInPxl, uint24_t maxAllowedHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen);
void drawMenu(uint24_t startName);
uint24_t findPictures();
int24_t ceilDiv(int24_t x, int24_t y);
