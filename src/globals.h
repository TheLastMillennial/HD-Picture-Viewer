#pragma once

/* Global Constants */

/* Valid list of locations HDPICV can be obtained from:
* Github Bin Folder //source files on github
* Github.com        //release page on github
* Cemetech.net
* tiplanet.org
* ticalc.org
*/
#define OBTAINED_FROM " from github bin folder"
#define VERSION "3.0.0-alpha" OBTAINED_FROM
#define YEAR "2025"
#define TUTORIAL_LINK "https://youtu.be/uixL9t5ZTJs"


/* MAX_IMAGES is the maximum number of images that can be on the calculator.
* It's obtained from the two characters used for appvar identification.
* First character can be alphabetic (26 options).
* Second character can be alphanumeric (36 options).
* 26 * 36 = 936 */
#define MAX_IMAGES 936 
//8 for image name, 1 for null terminator
#define BYTES_PER_IMAGE_NAME 9 
#define X_MARGIN 8
#define Y_MARGIN 38
#define Y_SPACING 25
#define MAX_THUMBNAIL_WIDTH 160
#define MAX_THUMBNAIL_HEIGHT 200
#define THUMBNAIL_ZOOM 0
#define ZOOM_SCALE 1.1
#define SUBIMAGE_DIMENSIONS 80
#define MAX_UINT 16777215
//colors
#define XLIBC_GREY 181 //the best grey xlibc has to offer
#define XLIBC_RED 192 //xlibc red
#define PALETTE_BLACK 0 //the xlibc palette and all hdpic generated palettes will have black as 0
#define PALETTE_WHITE 255 //the xlibc palette and all hdpic generated palettes will have white as 255

#define GFX16_WHITE 0xffff
#define GFX16_BLACK 0x0
#define GFX16_RED 0xf800
#define GFX16_LIGHT_GREY 0xbdf7

/* Color scheme based off DuckDuckGo
* Useful color visualizers:
*  https://rgbcolorpicker.com/565
*  https://roccoloxprograms.github.io/XlibcColorPicker/ */

// background levels, higher number is brighter.
#define GFX16_BG_0 0x0
#define GFX16_BG_1 0x18e3

#define GFX16_TEXT 0xad55
#define GFX16_TEXT_ERROR 0xdac6

