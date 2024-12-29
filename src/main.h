/* Function Prototypes */
uint8_t DatabaseReady();
void DisplayHomeScreen(uint24_t pics);
uint8_t DrawImage(uint24_t picName, uint24_t maxAllowedWidthInPxl, uint24_t maxAllowedHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen);
void DeleteImage(uint24_t picName);
void DisplayMenu(int24_t startName, char* picNames, const int24_t numOfPics);
uint24_t RebuildDB();
