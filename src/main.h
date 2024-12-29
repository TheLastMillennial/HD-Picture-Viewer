/* Function Prototypes */
uint8_t isDatabaseReady();
void drawHomeScreen(uint24_t pics);
uint8_t drawImage(uint24_t picName, uint24_t maxAllowedWidthInPxl, uint24_t maxAllowedHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen);
void deleteImage(uint24_t picName);
void drawMenu(uint24_t startName, const uint24_t numOfPics);
uint24_t rebuildDB();
int24_t ceilDiv(int24_t x, int24_t y);
