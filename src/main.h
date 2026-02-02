/* Function Prototypes */
void drawHomeScreen();
uint8_t drawMedia(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool refreshWholeScreen, int8_t shiftX = 0, int8_t shiftY = 0);
uint8_t drawImage(uint24_t picName, uint24_t desiredWidthInPxl, uint24_t desiredHeightInPxl, bool refreshWholeScreen, int8_t shiftX = 0, int8_t shiftY = 0);
uint8_t drawGIF(uint24_t picName, bool fullScreenPic);
void drawMenu_8bpp(uint24_t startName);
void drawMenu_16bpp(uint24_t startName);
uint24_t findPictures();
gfx_sprite_t *bitUnpackSprite(gfx_sprite_t *srcImg, gfx_sprite_t *outImg, uint8_t bpp);
static inline int24_t ceilDiv(int24_t x, int24_t y);
bool iterate(int24_t &xSubimgID, int24_t const &xFirstID, int24_t const &xLastID, int24_t &ySubimgID, int24_t const &yFirstID, int24_t const &yLastID, bool bDrawVertically, bool bDrawOppositeSideFirst, bool &bFirstRun);
static inline int24_t charToInt(char c);
uint24_t charArrToInt(const char *s, uint8_t size);
