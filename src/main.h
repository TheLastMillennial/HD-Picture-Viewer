/* Function Prototyptes */
uint8_t DatabaseReady();
void DisplayHomeScreen(uint24_t pics);
uint8_t DrawImage(uint24_t picName, uint24_t maxAllowedWidthInPxl, uint24_t maxAllowedHeightInPxl, int24_t xOffset, int24_t yOffset, bool refreshWholeScreen);
void DeleteImage(uint24_t picName);
void NoImagesFound();
void PrintCentered(const char* str);
void PrintCenteredX(const char* str, uint24_t y);
void PrintCenteredY(const char* str, uint8_t x);
void PrintText(const int8_t xpos, const int8_t ypos, const char* text);
void PrintHelpText(const char* button, const char* help, uint24_t yPos);
void DisplayMenu(int24_t startName, char* picNames, const int24_t numOfPics);
uint24_t RebuildDB(uint8_t progress);
void SplashScreen();
void DisplayWatermark();
void SetLoadingBarProgress(uint24_t progress, uint24_t t);