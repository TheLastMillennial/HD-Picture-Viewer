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
#define MAX_IMAGES 936 //Max images is this because max combinations of appvars goes up to that
#define TASKS_TO_FINISH 2
#define X_MARGIN 8
#define Y_MARGIN 38
#define Y_SPACING 25
#define MAX_THUMBNAIL_WIDTH 160
#define MAX_THUMBNAIL_HEIGHT 240
#define THUMBNAIL_ZOOM 0
#define SQUARE_WIDTH_AND_HEIGHT 80
#define MAX_UINT 16777215


/* Function Prototyptes */
uint8_t DatabaseReady();
void DisplayHomeScreen(uint24_t pics);
void DrawImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight, int24_t xOffset, int24_t yOffset);
void DeleteImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight);
void NoImagesFound();
void PrintCentered(const char *str);
void PrintCenteredX(const char *str, uint8_t y);
void PrintCenteredY(const char *str, uint8_t x);
void PrintNames(uint24_t start, char *picNames, uint24_t numOfPics);
void PrintText(int8_t xpos, int8_t ypos, const char *text);
uint24_t RebuildDB(uint8_t p);
void SplashScreen();
void SetLoadingBarProgress(uint24_t p, uint24_t t);
bool HorizOverflow(uint24_t Sq, int24_t off, uint24_t newWH, uint24_t scaleD);
bool VertOverflow(uint24_t Sq, int24_t off, uint24_t newWH, uint24_t scaleD);

/* Main function, called first */
int main(void)
{
  uint8_t ready=0, tasksFinished = 0;
  uint24_t picsDetected=0;
  /* Clear the homescreen */
  //os_ClrHome();

  gfx_Begin();
  ti_CloseAll();
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
    ti_CloseAll();
    gfx_End();
    return 0;
  }

  //something went wrong. Close all slots and quit.
  err:
  /* Waits for a keypress */
  while (!os_GetCSC());
  ti_CloseAll();
  gfx_End();

  /* Return 0 for success */
  return 0;

}

/* Functions */

void DisplayHomeScreen(uint24_t pics){
  char *picNames = malloc(pics*BYTES_PER_IMAGE_NAME); //BYTES_PER_IMAGE_NAME = 9
  ti_var_t database = ti_Open("HDPICDB","r");
  uint24_t i,startName=0,
  maxWidth=320, maxHeight=240;
  int24_t xOffset=0, yOffset=0;
  uint8_t Ypos=10;

  //kb_key_t key = kb_Data[7];
  bool prev, next, deletePic,
  zoomIn, zoomOut,
  panUp, panDown, panLeft, panRight;


  //makes the screen black and sets text gray
  gfx_FillScreen(0);
  gfx_SetTextFGColor(181);
  gfx_SetTextBGColor(0);
  gfx_SetColor(255);
  gfx_VertLine(140,20,200);


  //seeks to the first image name
  ti_Seek(8,SEEK_SET,database);
  //loops through every picture that was detected and store the image name to picNames
  for(i=0;i<=pics;i++){
    ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME],8,1,database);
    picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
    ti_Seek(8,SEEK_CUR,database);
    Ypos+=15;
  }

  /* Keypress handler */
  gfx_SetTextXY(10,10);
  PrintNames(startName, picNames, pics);
  DrawImage(startName,maxWidth,maxHeight, xOffset, yOffset);
  do{
    //scans the keys for keypress
    kb_Scan();
    //checks if up or down arrow key were pressed
    //key = kb_Data[7];
    next     = kb_Data[1] & kb_Graph;
    prev     = kb_Data[1] & kb_Yequ;
    deletePic= kb_Data[1] & kb_Del;
    zoomIn   = kb_Data[6] & kb_Add;
    zoomOut  = kb_Data[6] & kb_Sub;
    panUp    = kb_Data[7] & kb_Up;
    panDown  = kb_Data[7] & kb_Down;
    panLeft  = kb_Data[7] & kb_Left;
    panRight = kb_Data[7] & kb_Right;

    //if any key was pressed
    if(kb_AnyKey()){
      //dbg_sprintf(dbgout,"\nKey Pressed");

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


      //if plus key was pressed, zoom in by double
      if (zoomIn){
        //doubles zoom
        maxWidth = maxWidth*2;
        maxHeight = maxHeight*2;
        dbg_sprintf(dbgout,"\n\nKEYPRESS: Zoom In\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
        DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
      }
      //if subtract key was pressed, zoom out by double.
      if (zoomOut){
        maxWidth = maxWidth/2;
        maxHeight = maxHeight/2;
        dbg_sprintf(dbgout,"\n\nKEYPRESS: Zoom Out\n maxWidth: %d\n maxHeight: %d ", maxWidth, maxHeight);
        DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);

      }

      //if delete key pressed, delete all appvars related to current image
      if (deletePic){
        //the current palette is about to be deleted. Set the default palette
        gfx_SetDefaultPalette(gfx_8bpp);
        //we don't want the user seeing the horrors of their image with the wrong palette

        gfx_FillScreen(0);
        gfx_SetTextFGColor(181);
        gfx_SetTextBGColor(0);
        PrintCenteredX("Deleting Picture...",120);
        //close all open slots. We don't want any leaks.
        ti_CloseAll();
        //delete the palette and all squares
        DeleteImage(startName, maxWidth, maxHeight);
        PrintCenteredX("Picture deleted.",130);
        PrintCenteredX("Press any key.",140);
        while (!os_GetCSC());

        //picture names will change. Delete what we currently have
        free(picNames);
        //set color for potential splash screen
        gfx_SetTextFGColor(181);
        gfx_SetTextBGColor(0);
        //rebuild the database to account for the deleted image. Plug in 0 temporarily
        int picsDetected=RebuildDB(0);
        //check if all images were deleted. If so, just quit.
        if(picsDetected==0){
          //we pause because RebuildDB will show a warning screen we want the user to see
          while (!os_GetCSC());
          ti_CloseAll();
          gfx_End();
          return;
        }
        //ensure text is readable
        gfx_SetTextFGColor(191);
        gfx_SetTextBGColor(0);
        //re-allocate memory for the picture names
        picNames = malloc(picsDetected*BYTES_PER_IMAGE_NAME);

        //re-open the database since we closed everything earlier
        database = ti_Open("HDPICDB","r");
        //seeks to the first image name
        ti_Seek(8,SEEK_SET,database);
        //loops through every picture that was detected and store the image name to picNames
        for(i=0;i<=pics;i++){
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
        maxWidth = 320;
        maxHeight = 240;
        DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
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
        maxWidth = 320;
        maxHeight = 240;
        DrawImage(startName, maxWidth, maxHeight, xOffset, yOffset);
      }

    }
  }   while(kb_Data[6]!=kb_Clear);

  free(picNames);
}


void DeleteImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight){
  //open the database to figure out what image we're about to delete
  ti_var_t database = ti_Open("HDPICDB","r");
  char imgWH[6], imgID[2], searchName[9], palName[9];
  uint24_t widthSquares=0, heightSquares=0,
  maxWSquares=0, maxHSquares=0,
  xSquare=0, ySquare=0;
  uint8_t delSuccess;

  //seeks past header (8bytes), imgName, and unselected images
  ti_Seek(16+(16*picName),SEEK_CUR,database);
  //reads the image letter ID (2 bytes)
  ti_Read(imgID,2,1,database);
  //reads the image width/height (6 bytes)
  ti_Read(imgWH,6,1,database);
  //closes database
  ti_Close(database);

  ti_CloseAll();

  /*converts the char numbers from the header appvar into uint numbers
  (uint24_t)imgWH[?]-'0')*100 covers the 100's place
  (uint24_t)imgWH[?]-'0')*10 covers the 10's place
  (uint24_t)imgWH[?]-'0' covers the 1's place
  +1 accounts for 0 being the starting number
  */
  widthSquares= (((uint24_t)imgWH[0]-'0')*100+((uint24_t)imgWH[1]-'0')*10+(uint24_t)imgWH[2]-'0')+1;
  heightSquares=(((uint24_t)imgWH[3]-'0')*100+((uint24_t)imgWH[4]-'0')*10+(uint24_t)imgWH[5]-'0')+1;
  maxWSquares = (maxWidth/80); //todo: [jacobly] I'm saying you should use numTilesAcross * 80 rather than maxWidth / 80
  maxHSquares = (maxHeight/80);

  //deletes palette
  sprintf(palName, "HP%.2s0000",imgID);
  delSuccess = ti_Delete(palName);
  if (delSuccess == 0){
    PrintCenteredX(palName,120);
    dbg_sprintf(dbgout,"\nERR: Issue deleting palette");
  }
  //delete every square
  for(xSquare=(widthSquares-1);xSquare<MAX_UINT;xSquare--){
    dbg_sprintf(dbgout,"\nxS: %d",xSquare);
    for(ySquare=(heightSquares-1);ySquare<MAX_UINT;ySquare--){

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
*
*/
void DrawImage(uint24_t picName, uint24_t maxWidth, uint24_t maxHeight, int24_t xOffset, int24_t yOffset){
  dbg_sprintf(dbgout,"\n\n--IMAGE CHANGE--");
  ti_var_t database = ti_Open("HDPICDB","r"), squareSlot, palSlot;
  char imgWH[6], imgID[2], searchName[9], palName[9];
  uint24_t widthSquares=0, heightSquares=0,
  maxWSquares=0, maxHSquares=0,
  xSquare=0, ySquare=0;
  uint16_t *palPtr[256];
  gfx_sprite_t *outputImg, *srcImg;
  uint24_t scaleNum=1, scaleDen=1, newWidthHeight;
  gfx_FillScreen(0);

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
  widthSquares =(((uint24_t)imgWH[0]-'0')*100+((uint24_t)imgWH[1]-'0')*10+(uint24_t)imgWH[2]-'0')+1;
  heightSquares=(((uint24_t)imgWH[3]-'0')*100+((uint24_t)imgWH[4]-'0')*10+(uint24_t)imgWH[5]-'0')+1;
  maxWSquares = (maxWidth/80); //todo: [jacobly] I'm saying you should use numTilesAcross * 80 rather than maxWidth / 80
  maxHSquares = (maxHeight/80);
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
    PrintCenteredX("Press [+] twice to zoom in",145);
    dbg_sprintf(dbgout,"\nERR:\n scaleNum:%d\n scaleDen:%d",scaleNum,scaleDen);
    while(!os_GetCSC());
    ti_CloseAll();
    return;
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

  //allocates memory for outputImg according to scale
  outputImg = gfx_MallocSprite(newWidthHeight/scaleDen,newWidthHeight/scaleDen);
  if (!outputImg){
    PrintCenteredX("ERR: Failed to allocate memory!",130);
    while(!os_GetCSC());
    ti_CloseAll();
    return;
  }


  //sets correct palettes
  sprintf(palName, "HP%.2s0000",imgID);
  palSlot = ti_Open(palName,"r");
  if (!palSlot){
    PrintCenteredX(palName,110);
    PrintCenteredX("ERR: Palette does not exist!",120);
    PrintCenteredX("Image may have recently been deleted.",130);
    PrintCenteredX("Try restarting the program.",140);
    while(!os_GetCSC());
    ti_CloseAll();
    return;
  }

  //gfx_SetDrawBuffer();
  ti_Seek(24,SEEK_SET,palSlot);
  *palPtr = ti_GetDataPtr(palSlot);
  gfx_SetPalette(*palPtr,512,0);
  ti_Close(palSlot);

  dbg_sprintf(dbgout,"\n-------------------------");


  PrintCentered("Rendering...");
  //Displays all the images
  dbg_sprintf(dbgout,"\nwS: %d\nxO: %d",widthSquares,xOffset);
  dbg_sprintf(dbgout,"\nhS: %d\nyO: %d",heightSquares,yOffset);
  
	
  int24_t rightMostSquare=(widthSquares*(scaleNum/scaleDen));
  //This calculates the number of squares you can fit in the screen horizontally
  //we know the horizontal resolution of the screen is 320px. 
  //We can get the width of each square by doing newWidthHeight/scaleDen
  //the +1 is to account for rounding down errors. We don't want missing squares.
  rightMostSquare = 320/(newWidthHeight/scaleDen)+1;
  //leftmost and topmost always starts at 0
  int24_t leftMostSquare=0;
  int24_t topMostSquare=0;
  //This calculates the number of squares you can fit in the screen virtically
  //we know the vertical resolution of the screen is 240px. 
  //We can get the width of each square by doing newWidthHeight/scaleDen
  //the +1 is to account for rounding down errors. We don't want missing squares.
  int24_t bottomMostSquare = 240/(newWidthHeight/scaleDen)+1;


  //applies offsets
  //if we're panning horizontally, shift the rightmost and leftmost squares (xOffset is negative in this case)
  if(xOffset<0){
      rightMostSquare-=xOffset;
	  leftMostSquare-=xOffset;
  }
  //if we're panning vertically, shift the topmost and bottomost squares (yOffset is negative in this case)
  if(yOffset<0)
	  bottomMostSquare+=yOffset;
  if(yOffset>0)
	  topMostSquare+=yOffset;
  
  
  //make sure we don't try to display more squares than exist.
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

  //the -1 is to account for both the >= 
  //the +1 is to prevent underflow which would cause an infinite loop
  //this for loop outputs pic right to left
  for(xSquare=rightMostSquare-1;xSquare+1>=leftMostSquare+1;xSquare--){
  dbg_sprintf(dbgout,"\nxS: %d",xSquare);
    //this for loop outputs pic bottom to top
    for(ySquare=bottomMostSquare-1;ySquare+1>=topMostSquare+1;ySquare--){
		if(ySquare == MAX_UINT){
			
			dbg_sprintf(dbgout,"ERR: ySquare too high: %d",ySquare);
			return;
		}
		if(xSquare == MAX_UINT){
			dbg_sprintf(dbgout,"ERR: xSquare too high: %d",xSquare);
			return;
		}

        /*
      //This will eventually not draw squares that are out of frame
      if(HorizOverflow(xSquare, xOffset, newWidthHeight,scaleDen) || VertOverflow(ySquare+1, yOffset, newWidthHeight,scaleDen))
      {
        dbg_sprintf(dbgout,"\n-OOB- xLoc: %d\nyLoc: %d\ntest: %d",(xSquare+xOffset)*(newWidthHeight/scaleDen),(ySquare-yOffset)*(newWidthHeight/scaleDen), (xSquare+xOffset)*(newWidthHeight/scaleDen)-newWidthHeight/scaleDen);

        //dbg_sprintf(dbgout,"\nERR: Square would go out of screen bounds! \n %.2s%03u%03u\n x location: %d \n y location: %d",
        //imgID, xSquare, ySquare,(xSquare)*(newWidthHeight/scaleDen),(ySquare+1)*(newWidthHeight/scaleDen));
        continue;
      }*/
      //combines the separate parts into one name to search for
      sprintf(searchName, "%.2s%03u%03u", imgID, xSquare, ySquare);
      //dbg_sprintf(dbgout, "\n%.2s%03u%03u", imgID, xSquare, ySquare);
      /*This opens the variable with the name that was just assembled.
      * It then gets the pointer to that and stores it in a graphics variable
      */
      squareSlot = ti_Open(searchName,"r");
      //checks if the square does not exist
      if (!squareSlot){
        //square does not exist
        dbg_sprintf(dbgout,"\nERR: Square doesn't exist!");
        dbg_sprintf(dbgout,"\n %s",searchName);
        //dbg_sprintf(dbgout,"\nERR: \nxSquare: %d \newWidthHeight: %d \nscaleDen: %d",xSquare,newWidthHeight,scaleDen);
        gfx_sprite_t *errorImg;
        //uncompresses the error image
        errorImg = gfx_MallocSprite(SQUARE_WIDTH_AND_HEIGHT, SQUARE_WIDTH_AND_HEIGHT);
        zx7_Decompress(errorImg, errorTriangle_compressed);
        //resizes it to outputImg size
        gfx_ScaleSprite(errorImg,outputImg);
        //displays the output image
        //dbg_sprintf(dbgout,"\nxSquare: %d \newWidthHeight: %d \nscaleDen: %d\n",xSquare,newWidthHeight,scaleDen);
        gfx_Sprite(outputImg,(xSquare+xOffset)*(newWidthHeight/scaleDen), (ySquare-yOffset)*(newWidthHeight/scaleDen));
        free(errorImg);
        //while(!os_GetCSC());
        continue;
      }
      else
      {
        //if the square was detected, load it
        //seeks past header
        ti_Seek(16,SEEK_CUR,squareSlot);
        //store the original image into srcImg
        srcImg = (gfx_sprite_t*)ti_GetDataPtr(squareSlot);
        //resizes it to outputImg size
        gfx_ScaleSprite(srcImg,outputImg);
        //displays the output image
        //dbg_sprintf(dbgout,"\nxSquare: %d \newWidthHeight: %d \nscaleDen: %d\n",xSquare,newWidthHeight,scaleDen);
        //dbg_sprintf(dbgout,"\nxLoc: %d\nyLoc: %d",(xSquare+xOffset)*(newWidthHeight/scaleDen),(ySquare-yOffset)*(newWidthHeight/scaleDen));

        gfx_Sprite(outputImg,(xSquare+xOffset)*(newWidthHeight/scaleDen), (ySquare-yOffset)*(newWidthHeight/scaleDen));
      }
      //cleans up
      ti_Close(squareSlot);

    }
  }
  free(outputImg);
}
//checks if graphics will overflow horizontally
bool HorizOverflow(uint24_t Sq, int24_t off, uint24_t newWH, uint24_t scaleD){
  return false; //temporary
  if( ((Sq-off)*(newWH/scaleD)) - newWH/scaleD > 320){
    //dbg_sprintf(dbgout,"\nLoc: %d\nnewWH %d",(Sq+Off)*(newWH/scaleD),newWH);
    return true;
  }

  else{
    //dbg_sprintf(dbgout,"\nOOB xLoc: %d\nyLoc: %d\nnewWH %d",(xSquare+xOffset)*(newWidthHeight/scaleDen),(ySquare-yOffset)*(newWidthHeight/scaleDen),newWH);

    return false;
  }
}
//checks if graphics will overflow vertically
bool VertOverflow(uint24_t Sq, int24_t off, uint24_t newWH, uint24_t scaleD){
  return false; //temporary
  if((Sq-off)*(newWH/scaleD) > 240)
    return true;
  else
    return false;

}

/* Rebuilds the database of images on the calculator*/
uint24_t RebuildDB(uint8_t p){
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
  while((var_name = ti_DetectVar(&search_pos, "HDPALV10", TI_APPVAR_TYPE)) != NULL) {
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
  gfx_SetTextXY(150,195);
  gfx_PrintUInt(imagesFound,3);
  if (imagesFound==0){
    NoImagesFound();
  }
  SetLoadingBarProgress(++p,TASKS_TO_FINISH);
  return imagesFound;
}

void NoImagesFound(){
  gfx_SetTextBGColor(255);
  gfx_SetTextFGColor(192);
  PrintCenteredX("No Pictures Detected!",1);
  gfx_SetTextFGColor(0);
  PrintCenteredX("Convert some images and send them to your",11);
  PrintCenteredX("calculator using the HDpic converter!",21);
  PrintCenteredX("Tutorial:  https://youtu.be/s1-g8oSueQg",31);
  PrintCenteredX("Press any key to quit",41);
  return;
}

//checks if the database is already created. If not, it creates it.
uint8_t DatabaseReady(){
  char *var_name;
  void *search_pos = NULL;
  uint8_t exists=0, ready = 0;
  ti_var_t myAppVar;
  char myData[9]="HDDATV10"; //remember have one more space than text you're saving for null termiation
  char compare[9]="HDDATV10";
  //tries to find database using known header
  while((var_name = ti_DetectVar(&search_pos, myData, TI_APPVAR_TYPE)) != NULL) {
    exists=1;
  }
  //if file already exists, simply return
  if (exists == 1)
  ready = 2;
  else{
    //if file doesn't already exist, create it.
    //creates the database appvar and writes the header. Checks if wrote successfuly
    myAppVar=ti_Open("HDPICDB", "w");
    if(!myAppVar)
    ready = 3;
    if(ti_Write(&myData,8,1,myAppVar)!=1)
    ready = 4;
    if (ti_Rewind(myAppVar) == EOF)
    ready = 5;
    if (ti_Read(&myData,8, 1, myAppVar) != 1)
    ready = 6;
    if (strcmp(myData,compare)!=0)
    ready = 7;
    else{
      ready = 1;
    }
  }
  ti_CloseAll();

  //checks what happened
  if(ready==1){
    gfx_SetTextFGColor(195);
    PrintCenteredX("created",180);
    return 1;
  }else if(ready==2){
    gfx_SetTextFGColor(004);
    PrintCenteredX("exists",180);
    return 2;
  }else{
    gfx_SetTextFGColor(224);
    PrintCenteredX("failure",180);
    gfx_SetTextXY(120,190);
    gfx_PrintUInt(ready,1);
    return 0;
  }


}

//makes a loading bar and fills it in depending on progress made (p) / tasks left (t)
void SetLoadingBarProgress(uint24_t p, uint24_t t){
  p=((double)p/(double)t)*200.0;
  //ensures loading bar doesn't go past max point
  if (p>200)
  p=200;

  gfx_SetColor(128);
  gfx_FillRectangle_NoClip(60,153,(uint8_t)p,7);

}

//creates a simple splash screen when program starts
void SplashScreen(){
  //sets color to grey
  gfx_SetColor(181);
  gfx_FillRectangle_NoClip(60,80,LCD_WIDTH-120,LCD_HEIGHT-160);
  gfx_SetTextFGColor(35);
  gfx_SetTextBGColor(181);
  /* Print title screen */
  PrintCentered("HD Picture Viewer");
}

/* This UI keeps the user selection in the middle of the screen. */
void PrintNames(uint24_t startName, char *picNames, uint24_t numOfPics){
  uint24_t i, Yoffset=0, y=0, curName=0;

  //clears old text and sets prev for new text
  gfx_SetTextScale(2,2);
  gfx_SetColor(0);
  gfx_FillRectangle_NoClip(0,0,140,240);
  gfx_SetColor(255);

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
  curName=startName;


  /* draw the text on the screen. Starts displaying the name at element start
  * then iterates until out of pics or about to draw off the screen */
  for(i=0;i<numOfPics && y<180;i++){
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
void PrintCenteredX(const char *str, uint8_t y)
{
  gfx_PrintStringXY(str, (LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);
}
/* Prints a Y centered string */
void PrintCenteredY(const char *str, uint8_t x)
{
  gfx_PrintStringXY(str, x, (LCD_HEIGHT - 8) / 2);
}


/* Draw text on the homescreen at the given X/Y location */
void PrintText(int8_t xpos, int8_t ypos, const char *text) {
  os_SetCursorPos(ypos, xpos);
  os_PutStrFull(text);
}
