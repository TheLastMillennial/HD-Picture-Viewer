#include <tice.h>
#include <graphx.h>
#include <gfx16.h>
#include "guiUtils.h"
#include "globals.h"

// Simple text that displays program name and how to open help.
void drawWatermark_8bpp()
{
	gfx_SetTextScale(1, 1);
	gfx_SetTextFGColor(PALETTE_WHITE);
	gfx_SetTextBGColor(PALETTE_BLACK);
	gfx_PrintStringXY("HD Picture Viewer", 2, 2);
	gfx_PrintStringXY("[mode] = help", 2, 232);
}

// Simple text that displays program name and how to open help.
void drawWatermark_16bpp()
{
	gfx16_SetTextScale(1, 1);
	gfx16_SetTextFGColor(GFX16_WHITE);
	gfx16_SetTextBGColor(GFX16_BLACK);
	gfx16_PutStringXY("HD Picture Viewer", 2, 2);
	gfx16_PutStringXY("[mode] = help", 2, 232);
}

/* Prints a X centered string */
void gfx16_PrintCenteredX(const char *str, const uint24_t y)
{
	gfx16_PutStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);
}

/* Prints a screen centered string */
void PrintCentered(const char *str)
{
	gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, (LCD_HEIGHT - 8) / 2);
}
/* Prints a X centered string */
void PrintCenteredX(const char *str, const uint24_t y)
{
	gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);
}
/* Prints a Y centered string */
void PrintCenteredY(const char *str, const uint8_t x)
{
	gfx_PrintStringXY(str, x, (LCD_HEIGHT - 8) / 2);
}

/* Draw text on the homescreen at the given X/Y location */
void PrintText(const int8_t xpos, const int8_t ypos, const char *text)
{
	os_SetCursorPos(ypos, xpos);
	os_PutStrFull(text);
}

/* 16bpp Easy way to align help with a horizontal separator */
void PrintHelpText(const char *button, const char *help, uint24_t yPos)
{
	gfx16_PutStringXY(button, 10, yPos);
	gfx16_PutStringXY(help, 120, yPos);
	gfx16_HorizLine_NoClip(10, yPos + 8, 301);
}

// 16bpp creates a simple splash screen when program starts
void drawSplashScreen()
{
	gfx16_FillScreen(GFX16_BLACK);
	gfx16_SetColor(GFX16_BG_BLUE);
	gfx16_FillRectangle_NoClip(40, 80, 240, 80); //size: 2/3 screen width, 1/3 screen height
	
	/* Print title screen */
	gfx16_SetTextBGColor(GFX16_BG_BLUE);
	gfx16_SetTextFGColor(GFX16_TEXT_TITLE);
	gfx16_PrintCenteredX("HD Picture Viewer", 116);
	gfx16_PrintCenteredX(VERSION, 147);
}

/* 16bpp Draw instructions on how to use the program */
void drawHelp()
{
	gfx16_FillScreen(GFX16_BG_0);
	gfx16_SetTextBGColor(GFX16_BG_0);
	gfx16_SetColor(GFX16_TEXT);

	gfx16_SetTextFGColor(GFX16_TEXT_TITLE);
	gfx16_PrintCenteredX("HD Picture Viewer Help", 6);
	gfx16_PutStringXY("Keymap in Menu:", 1, 20);

	gfx16_SetTextFGColor(GFX16_TEXT);
	PrintHelpText("Clear", "Quit program.", 30);
	PrintHelpText("Enter", "Open picture fullscreen.", 40);
	PrintHelpText("Up   ", "Select previous.", 50);
	PrintHelpText("Down ", "Select next.", 60);

	gfx16_SetTextFGColor(GFX16_TEXT_TITLE);
	gfx16_PutStringXY("Keymap in Fullscreen:", 1, 80);
	gfx16_SetTextFGColor(GFX16_TEXT);
	PrintHelpText("Clear ", " Quit to menu.", 90);
	PrintHelpText("Y= ", " Show previous.", 100);
	PrintHelpText("Graph ", " Show next.", 110);
	PrintHelpText("Del ", " Delete picture permanently.", 120);
	//PrintHelpText("Arrow Keys", " Pan picture.", 130);
	//PrintHelpText("+ ", " Zoom in.", 140);
	//PrintHelpText("- ", " Zoom out.", 150);
	//PrintHelpText("Zoom ", " Maximum zoom.", 160);
	//PrintHelpText("Window", " Default zoom.", 170);

	gfx16_SetTextFGColor(GFX16_TEXT_TITLE);
	gfx16_PrintCenteredX("Press any key to return.", 190);

	gfx16_SetTextFGColor(GFX16_TEXT);
	gfx16_PutStringXY("Author:", 1, 220);
	gfx16_PutStringXY("TheLastMillennial", 64, 220);
	gfx16_PutStringXY("Tutorial:", 1, 210);
	gfx16_PutStringXY("Version:", 1, 230);

	gfx16_SetTextFGColor(GFX16_TEXT_BLUE);
	gfx16_PutStringXY(TUTORIAL_LINK, 64, 210);
	gfx16_PutStringXY(VERSION, 64, 230);
	gfx16_PutStringXY(YEAR, 288, 230);
}

// 16bpp Draw screen that informs user that no picture were detected.
void drawNoImagesFound()
{
	gfx16_SetTextBGColor(GFX16_BLACK);
	gfx16_SetTextFGColor(GFX16_TEXT_ERROR);
	gfx16_PrintCenteredX("No Pictures Detected!", 15);
	gfx16_SetTextFGColor(GFX16_TEXT);
	gfx16_PrintCenteredX("Convert some images and send them to", 30);
	gfx16_PrintCenteredX("your calculator using the converter!", 40);
	
	gfx16_SetTextFGColor(0xae1f); //URL blue
	gfx16_PrintCenteredX("https://youtu.be/uixL9t5ZTJs", 50);

	gfx16_SetTextFGColor(GFX16_TEXT);
	gfx16_PrintCenteredX("If you keep getting this error:", 180);
	gfx16_PrintCenteredX(" Go to home screen.", 190);
	gfx16_PrintCenteredX(" Press 2nd then + then select 'AppVars'. ", 200);
	gfx16_PrintCenteredX(" Ensure all picture files are present. ", 210);
	
	gfx16_SetTextFGColor(GFX16_TEXT_ERROR);
	gfx16_PrintCenteredX("Press any key to quit.", 230);

	gfx16_SetColor(GFX16_WHITE);
	gfx16_HorizLine_NoClip(0, 60, LCD_WIDTH);
	gfx16_HorizLine_NoClip(0, 177, LCD_WIDTH);
}