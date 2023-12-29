/*HD Picture Viewer 
* By TheLastMillennial
* https://github.com/TheLastMillennial/HD-Picture-Viewer
* Build With:    ./make debug --directory="path/to/HD-Picture-Viewer/Uncompiled Displayers/HD Picture Viewer 5"                             
*/

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fileioc.h>
#include <debug.h>
#include <math.h>
#include <compression.h>
#include "gfx/errorgfx.h"


/* globals */
#define BYTES_PER_IMAGE_NAME 9 //8 for image name, 1 for null terminator
//Max images is this because max combinations of appvars goes up to 936.
// Two characters for appvar identifier. 
// First character can be alphabetic (26 options). Second character can be alphanumeric (36 options). 
// 26*36=936 
#define MAX_IMAGES 936 
#define TASKS_TO_FINISH 2
#define X_MARGIN 8
#define Y_MARGIN 38
#define Y_SPACING 25
#define MAX_THUMBNAIL_WIDTH 160
#define MAX_THUMBNAIL_HEIGHT 240
#define THUMBNAIL_ZOOM 0
#define ZOOM_SCALE 2
#define SQUARE_WIDTH_AND_HEIGHT 80
#define MAX_UINT 16777215
//colors
#define XLIBC_GREY 181 //the best grey xlibc has to offer
#define XLIBC_RED 192 //xlibc red
#define PALETTE_BLACK 0 //the xlibc palette and all hdpic generated palettes will have black as 0
#define PALETTE_WHITE 255 //the xlibc palette and all hdpic generated palettes will have white as 255


/* Function Prototyptes */
uint8_t DatabaseReady();
void DisplayHomeScreen(uint24_t pics);
uint8_t DrawImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight, int24_t xOffset, int24_t yOffset);
void DeleteImage(uint24_t picName);
void NoImagesFound();
void PrintCentered(const char *str);
void PrintCenteredX(const char *str, uint8_t y);
void PrintCenteredY(const char *str, uint8_t x);
void PrintNames(uint24_t start, const char *picNames, const uint24_t numOfPics);
void PrintText(const int8_t xpos, const int8_t ypos, const char *text);
uint24_t RebuildDB(uint8_t progress);
void SplashScreen();
void SetLoadingBarProgress(uint24_t progress, uint24_t t);

/* Main function, called first */
int main(void)
{
    uint8_t ready{0}, tasksFinished {0};
    uint24_t picsDetected{0};
    /* Clear the homescreen */
    //os_ClrHome();
    gfx_Begin();
    //ti_CloseAll();
    SplashScreen();

    SetLoadingBarProgress(++tasksFinished, TASKS_TO_FINISH);
    //checks if the database exists and is ready 0 failure; 1 created; 2 exists
    ready = DatabaseReady();

    if (ready==0)
        goto err;


    //returns how many images were found. 0 means no images found so quit.
    picsDetected=RebuildDB(tasksFinished);

    if(picsDetected>0)
    {
        SetLoadingBarProgress(++tasksFinished,TASKS_TO_FINISH);
        //display the list of images
        DisplayHomeScreen(picsDetected);
        //quit
        gfx_End();
        return 0;
    }
    

    //something went wrong. Close all slots and quit.
    err:
    	dbg_sprintf(dbgout,"\nNot Ready");

    /* Waits for a keypress */
    while (!os_GetCSC());
    //ti_CloseAll();
    gfx_End();

    /* Return 0 for success */
    return 0;

}

/* Functions */

void DisplayHomeScreen(uint24_t pics){
    char *picNames { static_cast<char*>(malloc(pics*BYTES_PER_IMAGE_NAME))}; //BYTES_PER_IMAGE_NAME = 9
    ti_var_t database {ti_Open("HDPICDB","r")};
    uint24_t startName{0},
    maxWidth{LCD_WIDTH}, maxHeight{LCD_HEIGHT};
    int24_t xOffset{0}, yOffset{0};
    uint8_t Ypos{10};

    //kb_key_t key = kb_Data[7];
    bool prev, next, 
    deletePic, resetPic, redrawPic,
    zoomIn, zoomOut,
    panUp, panDown, panLeft, panRight;


    //makes the screen black and sets text gray
    gfx_FillScreen(PALETTE_BLACK);
    gfx_SetTextFGColor(XLIBC_GREY);
    gfx_SetTextBGColor(PALETTE_BLACK);
    gfx_SetColor(PALETTE_WHITE);
    gfx_VertLine(140,20,200);


    //seeks to the first image name
    ti_Seek(8,SEEK_SET,database);
    //loops through every picture that was detected and store the image name to picNames
    for(uint24_t i{0};i<=pics;i++){
        ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME],8,1,database);
        picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
        ti_Seek(8,SEEK_CUR,database);
        Ypos+=15;
    }

    //set up variable that checks if DrawImage failed
    uint8_t imageErr{0};
    /* Keypress handler */
    gfx_SetTextXY(10,10);
    PrintNames(startName, picNames, pics);
    DrawImage(startName,maxWidth,maxHeight, xOffset, yOffset);
    do{
        //scans the keys for keypress
        kb_Scan();
        //checks if up or down arrow key were pressed
        //key = kb_Data[7];
        next       = kb_Data[1] & kb_Graph;
        prev       = kb_Data[1] & kb_Yequ;
	    resetPic   = kb_Data[1] & kb_Window;
        deletePic  = kb_Data[1] & kb_Del;
		redrawPic  = kb_Data[6] & kb_Enter;
        zoomIn     = kb_Data[6] & kb_Add;
        zoomOut    = kb_Data[6] & kb_Sub;
        panUp      = kb_Data[7] & kb_Up;
        panDown    = kb_Data[7] & kb_Down;
        panLeft    = kb_Data[7] & kb_Left;
        panRight   = kb_Data[7] & kb_Right;

        //if any key was pressed
        if(kb_AnyKey()){
            //dbg_sprintf(dbgout,"\nKey Pressed");
			
			//image panning
            if (panLeft){
                xOffset++;
                DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
            }
            if (panRight){
                xOffset--;
                DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
            }
            if (panUp){
                yOffset--;
                DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
            }
            if (panDown){
                yOffset++;
                DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
            }
			
	    
			//if Window key was pressed, reset zoom and pan
			if (resetPic){
				maxWidth = LCD_WIDTH;
				maxHeight = LCD_HEIGHT;
				xOffset = 0;
				yOffset = 0;
				imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
				if(imageErr!=0){
					PrintCenteredX("Error resetting picture.",130);
					PrintCenteredX("Press any key to quit.",140);
					while (!os_GetCSC());
					ti_Close(database);
					free(picNames);
					gfx_End();
					return;
				}
			}
			//if Window key was pressed, reset zoom and pan
			if (redrawPic){
				imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
				if(imageErr!=0){
					PrintCenteredX("Error redrawing picture.",130);
					PrintCenteredX("Press any key to quit.",140);
					while (!os_GetCSC());
					ti_Close(database);
					free(picNames);
					gfx_End();
					return;
				}
			}
			
			//if plus key was pressed, zoom in by double
			if (zoomIn){
				if(imageErr!=0){dbg_sprintf(dbgout,"\npre-zoomIn error");}
				//doubles zoom
				maxWidth = maxWidth*ZOOM_SCALE;
				maxHeight = maxHeight*ZOOM_SCALE;
				dbg_sprintf(dbgout,"\n\n--KEYPRESS--\n Zoom In\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
				imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
				//this means we can't zoom in any more. Zoom back out.
				if(imageErr!=0){
					dbg_sprintf(dbgout,"\nCant zoom in trying zooming out...");
					maxWidth = maxWidth/ZOOM_SCALE;
					maxHeight= maxHeight/ZOOM_SCALE;
					dbg_sprintf(dbgout,"\n Zoomed out\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
					imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
					//if zooming back out didn't fix it, abort.
					if(imageErr!=0){
						dbg_sprintf(dbgout,"\nERR: Cant zoom in!!");

						PrintCenteredX("Error zooming in.",130);
						PrintCenteredX("Press any key to quit.",140);
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
			}
            //if subtract key was pressed, zoom out by double.
            if (zoomOut){
				dbg_sprintf(dbgout,"\n\n--KEYPRESS--\n Zoom Out");
				if(imageErr!=0){dbg_sprintf(dbgout,"\npre-zoomOut error");}
				
				//ensure we can zoom out without maxWidth or maxHeight becomeing 0
				if(maxWidth/ZOOM_SCALE!=0 && maxHeight/ZOOM_SCALE!=0){
					//apply the zoom out to the width and height
					maxWidth = maxWidth/ZOOM_SCALE;
					maxHeight = maxHeight/ZOOM_SCALE;
					
					dbg_sprintf(dbgout,"\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
					imageErr= DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
					//this means we can't zoom out any more. Zoom back in.
					if(imageErr!=0){
					dbg_sprintf(dbgout,"\nCant zoom out trying zooming in...");
						maxWidth = maxWidth*ZOOM_SCALE;
						maxHeight= maxHeight*ZOOM_SCALE;
						dbg_sprintf(dbgout,"\n Zoomed in\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);

						imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
						//if zooming back in didn't fix it, abort.
						if(imageErr!=0){
							dbg_sprintf(dbgout,"\nERR: Cant zoom out!!");

							PrintCenteredX("Error zooming out.",130);
							PrintCenteredX("Press any key to quit.",140);
							while (!os_GetCSC());
							ti_Close(database);
							free(picNames);
							gfx_End();
							return;
						}
					}

				}
                else
				{
					dbg_sprintf(dbgout,"\nmaxWidth or maxHeight too small. \n Zoom out aborted.");
					dbg_sprintf(dbgout,"\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
					//redraw the image. If it fails, I dunno why. It should be the exact same image as was previously displayed
					imageErr=DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
						//err handler
						if(imageErr!=0){
							dbg_sprintf(dbgout,"\nERR: Issue displaying same image??");

							PrintCenteredX("Error with zoom.",130);
							PrintCenteredX("Press any key to quit.",140);
							while (!os_GetCSC());
							ti_Close(database);
							free(picNames);
							gfx_End();
							return;
						}
					
					
				}
                
		
            }

            //if delete key pressed, delete all appvars related to current image
            if (deletePic){
                //the current palette is about to be deleted. Set the default palette
                gfx_SetDefaultPalette(gfx_8bpp);
                //we don't want the user seeing the horrors of their image with the wrong palette
                gfx_FillScreen(PALETTE_BLACK);
                gfx_SetTextFGColor(XLIBC_GREY);
                gfx_SetTextBGColor(PALETTE_BLACK);
                PrintCenteredX("Deleting Picture...",120);

                //delete the palette and all squares
                PrintCenteredX("Picture deleted.",130);
                PrintCenteredX("Press any key.",140);
                while (!os_GetCSC());

                //picture names will change. Delete what we currently have
                free(picNames);
                //set color for potential splash screen
                gfx_SetTextFGColor(XLIBC_GREY);
                gfx_SetTextBGColor(PALETTE_BLACK);
                //rebuild the database to account for the deleted image. Plug in 0 temporarily
                uint24_t picsDetected{RebuildDB(0)};
                //check if all images were deleted. If so, just quit.
                if(picsDetected==0){
                    //we pause because RebuildDB will show a warning screen we want the user to see
                    while (!os_GetCSC());
                    gfx_End();
                    return;
                }
		
                //ensure text is readable
                gfx_SetTextFGColor(XLIBC_GREY);
                gfx_SetTextBGColor(PALETTE_BLACK);
                //re-allocate memory for the picture names
                picNames = static_cast<char*>(malloc(picsDetected*BYTES_PER_IMAGE_NAME));

                //re-open the database since we closed everything earlier
                //database = ti_Open("HDPICDB","r");
                //seeks to the first image name
                ti_Seek(8,SEEK_SET,database);
                //loops through every picture that was detected and store the image name to picNames
                for(uint24_t i{0};i<=pics;i++){
                    ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME],8,1,database);
                    picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
                    ti_Seek(8,SEEK_CUR,database);
                    Ypos+=15;
                }

                //re construct the GUI
                gfx_SetTextXY(10,10);
                PrintNames(startName, picNames, pics);
                //display the next non-deleted image
                DrawImage(startName,maxWidth,maxHeight, xOffset, yOffset);

                //todo: change to different image
            }

            /* increases the name to start on and redraws the text */
            if(next){
                startName++;
                //make sure user can't scroll down too far
                if (startName>(pics-1))//If there's more than 4 images, then handle things normally
                startName=pics-1;
                if (startName>pics-1 && pics-1>0) //makes sure user can't scroll too far when there's only 1 image detected
                startName=pics;
                if (startName>pics-2 && pics-2>0) //makes sure user can't scroll too far when there's only 2 images detected
                startName=pics-1;
                if (startName>pics-3 && pics-3>0) //makes sure user can't scroll too far when there's only 3 images detected
                startName=pics-2;
                PrintNames(startName, picNames, pics);
                xOffset = 0;
                yOffset = 0;
                maxWidth = LCD_WIDTH;
                maxHeight = LCD_HEIGHT;
                imageErr= DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
				//if an error is thrown, then we've scrolled bast the safety barrier somehow.
				if(imageErr!=0){
					startName--;
					imageErr= DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
			        //if we still can't display an image, abort
					if(imageErr!=0){
						PrintCenteredX("Error showing next picture.",130);
						PrintCenteredX("Press any key to quit.",140);
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
					
            }

            /* decreases the name to start on and redraws the text */
            if(prev){
                startName--;
                /*checks if startName underflowed from 0 to 16 million or something.
                * Whatever the number, it shouldn't be less than the max number of images possible*/
                if (startName>MAX_IMAGES)
                startName=0;
                PrintNames(startName, picNames, pics);
                xOffset = 0;
                yOffset = 0;
                maxWidth = LCD_WIDTH;
                maxHeight = LCD_HEIGHT;
                imageErr=  DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
				//if an error is thrown, then we've scrolled bast the safety barrier somehow.
				if(imageErr!=0){
					startName++;
					imageErr= DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
			        //if we still can't display an image, abort
					if(imageErr!=0){
						PrintCenteredX("Error showing prev picture.",130);
						PrintCenteredX("Press any key to quit.",140);
						while (!os_GetCSC());
						while (!os_GetCSC());
						ti_Close(database);
						free(picNames);
						gfx_End();
						return;
					}
				}
            }

        }
    }     while(kb_Data[6]!=kb_Clear);

    ti_Close(database);
    free(picNames);
}


 void DeleteImage(uint24_t picName){
    //open the database to figure out what image we're about to delete
    ti_var_t database { ti_Open("HDPICDB","r")};
    char imgWH[6], imgID[2], searchName[9], palName[9];
    

    //seeks past header (8bytes), imgName, and unselected images
    ti_Seek(16+(16*picName),SEEK_CUR,database);
    //reads the image letter ID (2 bytes)
    ti_Read(imgID,2,1,database);
    //reads the image width/height (6 bytes)
    ti_Read(imgWH,6,1,database);
    //closes database
    ti_Close(database);

    /*converts the char numbers from the header appvar into uint numbers
    The header has 6 numbers so the below ? will go from 0-5
    (uint24_t)imgWH[?]-'0')*100 covers the 100's place
    (uint24_t)imgWH[?]-'0')*10 covers the 10's place
    (uint24_t)imgWH[?]-'0' covers the 1's place
    +1 accounts for 0 being the starting number
    */
    uint24_t widthSquares {((static_cast<uint24_t>(imgWH[0])-'0')*100+(static_cast<uint24_t>(imgWH[1])-'0')*10+static_cast<uint24_t>(imgWH[2])-'0')+1};
	uint24_t heightSquares{((static_cast<uint24_t>(imgWH[3])-'0')*100+(static_cast<uint24_t>(imgWH[4])-'0')*10+static_cast<uint24_t>(imgWH[5])-'0')+1};


    //deletes palette
    sprintf(palName, "HP%.2s0000",imgID);
    int delSuccess { ti_Delete(palName)};
    if (delSuccess == 0){
        PrintCenteredX(palName,120);
        dbg_sprintf(dbgout,"\nERR: Issue deleting palette");
    }
    //delete every square
    for(uint24_t xSquare=(widthSquares-1);xSquare<MAX_UINT;xSquare--){
        //dbg_sprintf(dbgout,"\nxS: %d",xSquare);
        for(uint24_t ySquare=(heightSquares-1);ySquare<MAX_UINT;ySquare--){

            //combines the separate parts into one name to search for
            sprintf(searchName, "%.2s%03u%03u", imgID, xSquare, ySquare);

            /*This opens the variable with the name that was just assembled.
            * It then gets the pointer to that and stores it in a graphics variable
            */
            delSuccess = ti_Delete(searchName);
            //checks if the square does not exist
            if (delSuccess==0){
                //square does not exist
                dbg_sprintf(dbgout,"\nERR: Issue deleting square");
                dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgID, xSquare, ySquare);
            }
        }
    }
}

/* Draws the image stored in database at position startName.
* Draws the image at location x,y starting at top left corner.
* If x=-1 then make image horizontally centered in the screen.
* If y=-1 then make image vertically centered on the screen.
* Image will automatically be resized to same aspect ratio so you just set the max width and height (4,3 will fit the screen normally)
* If sucessful, returns 0. Otherwise returns 1
*/
uint8_t DrawImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight, int24_t xOffset, int24_t yOffset){
    dbg_sprintf(dbgout,"\n\n--IMAGE CHANGE--");
    ti_var_t database { ti_Open("HDPICDB","r")};
	
    char imgWH[6], imgID[2], searchName[9], palName[9];
	int24_t scaleNum{1}, scaleDen{1}, newWidthHeight;
    gfx_FillScreen(PALETTE_BLACK);

    //seeks past header (8bytes), imgName, and unselected images
    ti_Seek(16+(16*picName),SEEK_CUR,database);
    //reads the image letter ID (2 bytes)
    ti_Read(imgID,2,1,database);
    //reads the image width/height (6 bytes)
    ti_Read(imgWH,6,1,database);
    //closes database
    ti_Close(database);

    //Converts the width/height from a char array into two integers by converting char into decimal value
    //then subtracting 48 to get the actuall number.
    gfx_SetTextScale(1,1);
    //gfx_PrintStringXY(imgWH,170,10);
    dbg_sprintf(dbgout,"\nimgHeader: %s \n", imgWH);

    /*converts the char numbers from the header appvar into uint numbers
    (uint24_t)imgWH[?]-'0')*100 covers the 100's place
    (uint24_t)imgWH[?]-'0')*10 covers the 10's place
    (uint24_t)imgWH[?]-'0' covers the 1's place
    +1 accounts for 0 being the starting number
    */
    int24_t widthSquares {((static_cast<int24_t>(imgWH[0])-'0')*100+(static_cast<int24_t>(imgWH[1])-'0')*10+static_cast<int24_t>(imgWH[2])-'0')+1};
    int24_t heightSquares{((static_cast<int24_t>(imgWH[3])-'0')*100+(static_cast<int24_t>(imgWH[4])-'0')*10+static_cast<int24_t>(imgWH[5])-'0')+1};
    uint24_t maxWSquares { (maxWidth /80) }; //todo: [jacobly] I'm saying you should use numTilesAcross * 80 rather than maxWidth / 80
    uint24_t maxHSquares { (maxHeight/80) };
    dbg_sprintf(dbgout,"\n maxWS: %d\n widthS: %d\n maxHS: %d\n heightS: %d\n",maxWSquares,widthSquares,maxHSquares,heightSquares);

    //checks if it should scale an image horizontally or vertically.
    if (widthSquares * maxHSquares < heightSquares * maxWSquares) {
        scaleNum = maxWSquares;
        scaleDen = widthSquares;
        dbg_sprintf(dbgout,"\nPath 1 ");
    } else {
        scaleNum = maxHSquares;
        scaleDen = heightSquares;
        dbg_sprintf(dbgout,"\nPath 2 ");
    }
    if(scaleNum == 0 || scaleDen == 0){
        PrintCenteredX("ERR: Cannot zoom out!",130);
        //PrintCenteredX("Press [+] twice to zoom in",145);
        dbg_sprintf(dbgout,"\nERR: Cant zoom out\n scaleNum:%d\n scaleDen:%d",scaleNum,scaleDen);
        //while(!os_GetCSC());
        return 1;
    }
    newWidthHeight = SQUARE_WIDTH_AND_HEIGHT*scaleNum;
	
    /*
    [jacobly] so now whenever we want to compute
    `x * scale`
    we instead want to compute
    `x * (scaleNum / scaleDen)`
    which can now be reordered to use strictly integer math as
    `(x * scaleNum) / scaleDen`


    [jacobly] if you don't know:
    floorDiv(x, y) := x / y;
    roundDiv(x, y) := (x + (y / 2)) / y;
    ceilDiv(x, y) := (x + y - 1) / y;
    [MateoC] huh I didn't know about roundDiv
    */
	
    dbg_sprintf(dbgout,"\n newWH: %d \n ScaleNum: %d \n scaleDen: %d \n xOffset: %d \n yOffset %d",newWidthHeight,scaleNum,scaleDen, xOffset, yOffset);

	//memory where each unsized image will be stored
    gfx_sprite_t *srcImg { gfx_MallocSprite(SQUARE_WIDTH_AND_HEIGHT, SQUARE_WIDTH_AND_HEIGHT)};
    if (!srcImg){
		dbg_sprintf(dbgout,"\nERR: Failed to allocate src memory!");
        PrintCenteredX("ERR: Failed to allocate src memory!",130);
        return 1;
    }
	//scales the suqare width and height to the final output dimensions
	int24_t newSquareDim {newWidthHeight/scaleDen};
	//ensure the resized square will fit within the dimensions of the screen.
	if (newSquareDim>LCD_HEIGHT){
		dbg_sprintf(dbgout,"\nERR: Square will be too large: %d",newSquareDim);
        PrintCenteredX("ERR: Output picture too large!",130);
		free(srcImg);
        return 1;
	}
    //allocates memory for resized image
    gfx_sprite_t *outputImg { gfx_MallocSprite(newSquareDim,newSquareDim)};
    if (!outputImg){
		dbg_sprintf(dbgout,"\nERR: Failed to allocate output memory!");
        PrintCenteredX("ERR: Failed to allocate output memory!",130);
		free(srcImg);
        return 1;
    }

    //sets correct palettes
    sprintf(palName, "HP%.2s0000",imgID);
    ti_var_t palSlot { ti_Open(palName,"r")};
    if (!palSlot){
        PrintCenteredX(palName,110);
        PrintCenteredX("ERR: Palette does not exist!",120);
        PrintCenteredX("Image may have recently been deleted.",130);
        PrintCenteredX("Try restarting the program.",140);
        while(!os_GetCSC());
		free(srcImg);
		free(outputImg);
        return 1;
    }

    //gfx_SetDrawBuffer();
    ti_Seek(24,SEEK_SET,palSlot);
    gfx_SetPalette(ti_GetDataPtr(palSlot),512,0);
    ti_Close(palSlot);

    dbg_sprintf(dbgout,"\n-------------------------");


    PrintCentered("Rendering...");
    //Displays all the images
    dbg_sprintf(dbgout,"\nwS: %d\nxO: %d",widthSquares,xOffset);
    dbg_sprintf(dbgout,"\nhS: %d\nyO: %d",heightSquares,yOffset);
    
	
    //This calculates the number of squares you can fit in the screen horizontally
    //we know the horizontal resolution of the screen is 320px. 
    //We can get the width of each square by doing newWidthHeight/scaleDen
    //the +1 is to account for rounding down errors. We don't want missing squares.
    int24_t rightMostSquare { LCD_WIDTH/(newWidthHeight/scaleDen)+1};
    //leftmost and topmost always starts at 0
    int24_t leftMostSquare{0};
    int24_t topMostSquare={0};
    //This calculates the number of squares you can fit in the screen virtically
    //we know the vertical resolution of the screen is 240px. 
    //We can get the width of each square by doing newWidthHeight/scaleDen
    //the +1 is to account for rounding down errors. We don't want missing squares. (Overflow is compensated for, if neessary, below)
    int24_t bottomMostSquare { LCD_HEIGHT/(newWidthHeight/scaleDen)+1};


    /*applies offsets*/
    //if we're panning horizontally, shift the rightmost and leftmost squares (xOffset is negative in this case)
    rightMostSquare-=xOffset;
    leftMostSquare-=xOffset;
    //if we're panning vertically, shift the topmost and bottomost squares (yOffset is negative in this case)
    bottomMostSquare+=yOffset;
    topMostSquare+=yOffset;
    
    
    /*make sure we don't try to display more squares than exist.*/
    if (rightMostSquare>widthSquares)
	    rightMostSquare=widthSquares;
    if (leftMostSquare<0)
	    leftMostSquare=0;
    if (bottomMostSquare>heightSquares)
	    bottomMostSquare=heightSquares;
    if (topMostSquare<0)
	    topMostSquare=0;

	dbg_sprintf(dbgout,"\nrightMost %d\nLeftMost %d",rightMostSquare,leftMostSquare);
	dbg_sprintf(dbgout,"\ntopMost %d\nbottomMost %d",topMostSquare,bottomMostSquare);
    
    /*Loop to display images*/
    //the -1 is to account for both the
    //the +1 is to prevent underflow which would cause an infinite loop
    //this for loop outputs pic right to left, top to bottom
    int24_t xStart{leftMostSquare-1}, xEnd{rightMostSquare-1};
    int24_t yStart{topMostSquare-1},  yEnd{bottomMostSquare-1};
    
    for(int24_t xSquare{xEnd};xSquare>xStart;--xSquare){
		//dbg_sprintf(dbgout,"\nxS: %d",xSquare);
        //this for loop outputs pic bottom to top
        for(int24_t ySquare{yEnd};ySquare>yStart;--ySquare){
			//dbg_sprintf(dbgout,"\nyS: %d",ySquare);
			//a key interrupted output. Quit immediately
			if(os_GetCSC()){
				//free up source and output memory
				free(srcImg);
				free(outputImg);
				return 0;
			}
            //combines the separate parts into one name to search for
            sprintf(searchName, "%.2s%03u%03u", imgID, xSquare, ySquare);
            //dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgID, xSquare, ySquare);
            /*
			* This opens the variable with the name that was just assembled.
            * It then gets the pointer to that and stores it in a graphics variable
            */
            const ti_var_t squareSlot { ti_Open(searchName,"r")};
            //checks if the square exists
            if (squareSlot){
				//square exists, load it
                //seeks past header
                ti_Seek(16,SEEK_CUR,squareSlot);
                //store the original image into srcImg
                //srcImg = (gfx_sprite_t*)ti_GetDataPtr(squareSlot);
		
				zx0_Decompress(srcImg, ti_GetDataPtr(squareSlot));
                //resizes it to outputImg size
                gfx_ScaleSprite(srcImg,outputImg);

				//displays square
				//if we are displaying an edge image, clip the square. Otherwise don't clip for extra speed.
				if(xSquare==xEnd || ySquare==yEnd){
					gfx_Sprite(outputImg,(xSquare+xOffset)*(newWidthHeight/scaleDen), (ySquare-yOffset)*(newWidthHeight/scaleDen));
				}
				else{
					gfx_Sprite_NoClip(outputImg,(xSquare+xOffset)*(newWidthHeight/scaleDen), (ySquare-yOffset)*(newWidthHeight/scaleDen));
				}
				
			}else{
                //square does not exist, display error image
                dbg_sprintf(dbgout,"\nERR: Square doesn't exist!");
                dbg_sprintf(dbgout,"\n %s",searchName);
                //dbg_sprintf(dbgout,"\nERR: \nxSquare: %d \newWidthHeight: %d \nscaleDen: %d",xSquare,newWidthHeight,scaleDen);
                zx7_Decompress(srcImg, errorTriangle_compressed);
                //resizes it to outputImg size
                gfx_ScaleSprite(srcImg,outputImg);
                //displays the output image
                //dbg_sprintf(dbgout,"\nxSquare: %d \newWidthHeight: %d \nscaleDen: %d\n",xSquare,newWidthHeight,scaleDen);
                gfx_Sprite_NoClip(outputImg,(xSquare+xOffset)*(newWidthHeight/scaleDen), (ySquare-yOffset)*(newWidthHeight/scaleDen));
                //while(!os_GetCSC());
                continue;
            }

            //cleans up
            ti_Close(squareSlot);

        }
    }
    //free up source and output memory
    free(srcImg);
    free(outputImg);
    return 0;
}


/* Rebuilds the database of images on the calculator*/
uint24_t RebuildDB(uint8_t progress){
    char *var_name, *imgInfo[16];// nameBuffer[10];
    void *search_pos = NULL;
    uint24_t imagesFound=0;
    //char myData[8]="HDPALV1" ,names[8];
    ti_var_t database = ti_Open("HDPICDB","w"), palette;
    ti_Write("HDDATV10",8,1,database);//Rewrites the header because w overwrites everything

    //resets splash screen for new loading SetLoadingBarProgress
    SplashScreen();

    /*
    * Searches for palettes. This is a lot easier than searching for every single
    * image square because there's is guarunteed to only be one palette per image.
    * The palette containts all the useful information such as the image size and
    * the two letter ID for each appvar. This makes it easy to find every square via a loop.
    */
    while((var_name = ti_DetectVar(&search_pos, "HDPALV10", OS_TYPE_APPVAR)) != NULL) {
        //sets progress of how many images were found
        SetLoadingBarProgress(++imagesFound,MAX_IMAGES);
        //finds the name, letter ID, and size of entire image this palette belongs to.
        palette = ti_Open(var_name,"r");
        //seeks past useless info
        ti_Seek(8,SEEK_CUR,palette);
        ti_Seek(16,SEEK_CUR,database);
        //reads the important info (16 bytes)
        ti_Read(&imgInfo,16,1,palette);
        //Writes the info to the database
        ti_Write(imgInfo,16,1,database);
        //closes palette for next iteration
        ti_Close(palette);
    }
    //closes the database
    ti_Close(database);
    gfx_End();
    ti_SetArchiveStatus(true,database);
    gfx_Begin();
    SplashScreen();
    gfx_SetTextXY(150,130);
    gfx_PrintUInt(imagesFound,3);
    if (imagesFound==0){
        NoImagesFound();
    }
    SetLoadingBarProgress(++progress,TASKS_TO_FINISH);
    return imagesFound;
}

void NoImagesFound(){
    gfx_SetTextBGColor(PALETTE_WHITE);
    gfx_SetTextFGColor(XLIBC_RED);
    PrintCenteredX("No Pictures Detected!",1);
    gfx_SetTextFGColor(PALETTE_BLACK);
    PrintCenteredX("Convert some images and send them to your",11);
    PrintCenteredX("calculator using the HDpic converter!",21);
    PrintCenteredX("Tutorial: no pre-release tutorial!",31);
    PrintCenteredX("If you keep getting this error:",181);
    PrintCenteredX(" Go to home screen, press 2nd then +",191);
    PrintCenteredX(" then select 'AppVars'. ",201);
    PrintCenteredX(" Ensure the picture is there. ",211);
    PrintCenteredX("Press any key to quit.",231);
    return;
}

//checks if the database is already created. If not, it creates it.
uint8_t DatabaseReady(){
    char *var_name;
    void *search_pos = NULL;
    uint8_t exists{0}, ready {0};
    ti_var_t database;
    char myData[9]{"HDDATV10"}; //remember have one more space than text you're saving for null termiation
	char compare[9]{"HDDATV10"};
    //tries to find database using known header
	//todo: why var_name = ???
    while((var_name = ti_DetectVar(&search_pos, myData, OS_TYPE_APPVAR)) != NULL) {
        exists=1;
    }
    //if file already exists, simply return
    if (exists == 1)
    ready = 2;
    else{
        //if file doesn't already exist, create it.
        //creates the database appvar and writes the header. Checks if wrote successfuly
        database=ti_Open("HDPICDB", "w");
        if(!database)
        ready = 3;
        if(ti_Write(&myData,8,1,database)!=1)
        ready = 4;
        if (ti_Rewind(database) == EOF)
        ready = 5;
        if (ti_Read(&myData,8, 1, database) != 1)
        ready = 6;
        if (strcmp(myData,compare)!=0)
        ready = 7;
        else{
            ready = 1;
        }
	ti_Close(database);
    }
    

    //checks what happened
    if(ready==1){
	dbg_sprintf(dbgout,"\nDatabase Created");
        return 1;
    }else if(ready==2){
	dbg_sprintf(dbgout,"\nDatabase Aready Exists");
        return 2;
    }else{
	dbg_sprintf(dbgout,"\nDatabase Failed to Create: %d\n",ready);
        gfx_SetTextFGColor(XLIBC_RED);
        PrintCenteredX("DB Failure! Please report:",180);
        gfx_SetTextXY(120,200);
        gfx_PrintUInt(ready,1);
        return 0;
    }


}

//makes a loading bar and fills it in depending on progress made / tasks left
void SetLoadingBarProgress(uint24_t progress, const uint24_t tasks){
    progress=((double)progress/(double)tasks)*200.0;
    //ensures loading bar doesn't go past max point
    if (progress>200)
		progress=200;

    gfx_SetColor(128);
    gfx_FillRectangle_NoClip(60,153,static_cast<uint8_t>(progress),7);

}

//creates a simple splash screen when program starts
void SplashScreen(){
    //sets color to grey
    gfx_SetColor(XLIBC_GREY);
    gfx_FillRectangle_NoClip(60,80,LCD_WIDTH-120,LCD_HEIGHT-160);
    gfx_SetTextFGColor(35);
    gfx_SetTextBGColor(XLIBC_GREY);
    /* Print title screen */
    PrintCentered("HD Picture Viewer");
}

/* This UI keeps the user selection in the middle of the screen. */
void PrintNames(uint24_t startName, const char *picNames, const uint24_t numOfPics){
     uint24_t Yoffset{0}, y{0};

    //clears old text and sets prev for new text
    gfx_SetTextScale(2,2);
    gfx_SetColor(PALETTE_BLACK);
    gfx_FillRectangle_NoClip(0,0,140,240);
    gfx_SetColor(PALETTE_WHITE);

    //re-draws UI lines
    gfx_HorizLine_NoClip(0,120,6);
    gfx_HorizLine_NoClip(136,120,5);
    gfx_HorizLine_NoClip(6,110,130);
    gfx_HorizLine_NoClip(6,130,130);
    gfx_VertLine_NoClip(6,110,20);
    gfx_VertLine_NoClip(136,110,21);

    /*if the selected start name is under 4, that means we need to start drawing
    * farther down the screen for the text to go in the right spot */
    if(startName<4){
        Yoffset = 75 - startName * Y_SPACING;
        startName = 0;
    }else{
        startName-=4;
    }
    uint24_t curName{startName};


    /* draw the text on the screen. Starts displaying the name at element start
    * then iterates until out of pics or about to draw off the screen */
    for(uint24_t i{0};i<numOfPics && y<180;i++){
        //calculates where the text should be drawn
        y = i * Y_SPACING + Y_MARGIN + Yoffset;

        //Prints out the correct name
        gfx_PrintStringXY(&picNames[curName++*BYTES_PER_IMAGE_NAME],X_MARGIN,y);
        //while(!os_GetCSC());

    }
    //slows next scrolling speed
    delay(150);
}

/* Prints a screen centered string */
void PrintCentered(const char *str)
{
    gfx_PrintStringXY(str,(LCD_WIDTH - gfx_GetStringWidth(str)) / 2, (LCD_HEIGHT - 8) / 2);
}
/* Prints a X centered string */
void PrintCenteredX(const char *str, const uint8_t y)
{
    gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);
}
/* Prints a Y centered string */
void PrintCenteredY(const char *str, const uint8_t x)
{
    gfx_PrintStringXY(str, x, (LCD_HEIGHT - 8) / 2);
}


/* Draw text on the homescreen at the given X/Y location */
void PrintText(const int8_t xpos, const int8_t ypos, const char *text) {
    os_SetCursorPos(ypos, xpos);
    os_PutStrFull(text);
}
