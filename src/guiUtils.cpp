#include <tice.h>
#include <graphx.h>
#include "guiUtils.h"
#include "globals.h"


// Simple text that displays program name and how to open help.
void drawWatermark()
{
	gfx_SetTextScale(1, 1);
	gfx_SetTextFGColor(PALETTE_WHITE);
	gfx_SetTextBGColor(PALETTE_BLACK);
	gfx_PrintStringXY("HD Picture Viewer", 2, 2);
	gfx_PrintStringXY("[mode] = help", 2, 232);
}

/* Prints a screen centered string */
void PrintCentered(const char* str)
{
	gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, (LCD_HEIGHT - 8) / 2);
}
/* Prints a X centered string */
void PrintCenteredX(const char* str, const uint24_t y)
{
	gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);
}
/* Prints a Y centered string */
void PrintCenteredY(const char* str, const uint8_t x)
{
	gfx_PrintStringXY(str, x, (LCD_HEIGHT - 8) / 2);
}

/* Draw text on the homescreen at the given X/Y location */
void PrintText(const int8_t xpos, const int8_t ypos, const char* text) {
	os_SetCursorPos(ypos, xpos);
	os_PutStrFull(text);
}

/* Easy way to align help with a horizontal separator */
void PrintHelpText(const char* button, const char* help, uint24_t yPos) {
	gfx_PrintStringXY(button, 10, yPos);
	gfx_PrintStringXY(help, 120, yPos);
	gfx_HorizLine_NoClip(10, yPos + 8, 301);
}

//creates a simple splash screen when program starts
void drawSplashScreen() {
	//gfx_SetColor(PALETTE_BLACK);
	//gfx_FillRectangle_NoClip(60, 80, LCD_WIDTH - 120, LCD_HEIGHT - 160);
	gfx_FillScreen(PALETTE_BLACK);
	gfx_SetColor(PALETTE_WHITE);
	gfx_Rectangle_NoClip(60, 80, LCD_WIDTH - 120, LCD_HEIGHT - 160);
	gfx_SetTextFGColor(PALETTE_WHITE);
	gfx_SetTextBGColor(PALETTE_BLACK);
	/* Print title screen */
	PrintCenteredX(VERSION, 125);
	PrintCenteredX("HD Picture Viewer", 110);
}

// Display full help screen
void drawHelp()
{
	gfx_FillScreen(PALETTE_BLACK);
	gfx_SetTextBGColor(PALETTE_BLACK);
	gfx_SetTextFGColor(PALETTE_WHITE);
	gfx_SetColor(PALETTE_WHITE);
	PrintCenteredX("HD Picture Viewer Help", 6);
	gfx_PrintStringXY("Keymap in Menu:", 1, 20);

	PrintHelpText("Clear", "Quit program.", 30);
	PrintHelpText("Enter", "Open picture fullscreen.", 40);
	PrintHelpText("Up   ", "Select previous.", 50);
	PrintHelpText("Down ", "Select next.", 60);

	gfx_PrintStringXY("Keymap in Fullscreen:", 1, 80);
	PrintHelpText("Clear ", " Quit to menu.", 90);
	PrintHelpText("Y= ", " Show previous.", 100);
	PrintHelpText("Graph ", " Show next.", 110);
	PrintHelpText("Arrow Keys", " Pan picture.", 120);
	PrintHelpText("Del ", " Delete picture permanently.", 130);
	PrintHelpText("+ ", " Zoom in.", 140);
	PrintHelpText("- ", " Zoom out.", 150);
	PrintHelpText("Zoom ", " Maximum zoom.", 160);
	PrintHelpText("Window", " Default zoom.", 170);

	PrintCenteredX("Press any key to return.", 190);

	gfx_PrintStringXY("Author:", 1, 220);
	gfx_PrintStringXY("TheLastMillennial", 64, 220);
	gfx_PrintStringXY("Tutorial:", 1, 210);
	gfx_PrintStringXY(TUTORIAL_LINK, 64, 210);
	gfx_PrintStringXY("Version:", 1, 230);
	gfx_PrintStringXY(VERSION, 64, 230);
	gfx_PrintStringXY(YEAR, 288, 230);
}

// Draw screen that informs user that no picture were detected.
void drawNoImagesFound() {
	gfx_SetTextBGColor(PALETTE_BLACK);
	gfx_SetTextFGColor(XLIBC_RED);
	PrintCenteredX("No Pictures Detected!", 15);
	gfx_SetTextFGColor(PALETTE_WHITE);
	PrintCenteredX("Convert some images and send them to your", 30);
	PrintCenteredX("calculator using the HD Pic converter!", 40);
	PrintCenteredX("Tutorial: https://youtu.be/uixL9t5ZTJs", 50);

	PrintCenteredX("If you keep getting this error:", 180);
	PrintCenteredX(" Go to home screen.", 190);
	PrintCenteredX(" Press 2nd then + then select 'AppVars'. ", 200);
	PrintCenteredX(" Ensure all picture files are present. ", 210);
	PrintCenteredX("Press any key to quit.", 230);

	gfx_SetColor(PALETTE_WHITE);
	gfx_HorizLine_NoClip(0, 60, 320);
	gfx_HorizLine_NoClip(0, 177, 320);
}