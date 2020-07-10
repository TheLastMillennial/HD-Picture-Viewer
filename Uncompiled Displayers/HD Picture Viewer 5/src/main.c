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

/* globals */

#define BYTES_PER_IMAGE_NAME 9 //8 for image name, 1 for null terminator
#define MAX_IMAGES 936 //Max images is this because max combinations of appvars goes up to that
#define TASKS_TO_FINISH 2
#define X_MARGIN 8
#define Y_MARGIN 43
#define Y_SPACING 25



/* Function Prototyptes */
uint8_t databaseReady();
void DisplayHomeScreen(uint24_t pics);
void noImagesFound();
void PrintCentered(const char *str);
void PrintCenteredX(const char *str, uint8_t y);
void PrintCenteredY(const char *str, uint8_t x);
void printNames(uint24_t start, char *picNames, uint24_t numOfPics);
void printText(int8_t xpos, int8_t ypos, const char *text);
uint24_t rebuildDB(uint8_t p);
void SplashScreen();
void SetLoadingBarProgress(uint8_t p, uint8_t t);

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
  ready = databaseReady();
  if (ready==0)
  goto err;


  picsDetected=rebuildDB(tasksFinished);
  if(picsDetected==0)
  goto err;
  //returns how many images were found. 0 means quit.

  SetLoadingBarProgress(++tasksFinished,TASKS_TO_FINISH);
  DisplayHomeScreen(picsDetected);

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
  uint24_t i,startName=0;
  uint8_t Ypos=10;
  kb_key_t key = kb_Data[7];
  bool up,down,left,right;


  //makes the screen black and text white
  gfx_FillScreen(0);
  gfx_SetTextFGColor(254);
  gfx_SetTextBGColor(0);
  gfx_SetColor(255);
  gfx_VertLine(140,20,200);


  //seeks to the first image name
  ti_Seek(8,SEEK_SET,database);
  //PrintCenteredX("Test1",30);
  //loops through every picture that was detected and store the image name to picNames
  for(i=0;i<=pics;i++){
    ti_Read(&picNames[i * BYTES_PER_IMAGE_NAME],8,1,database);
    picNames[i * BYTES_PER_IMAGE_NAME + BYTES_PER_IMAGE_NAME - 1] = 0;
    ti_Seek(16,SEEK_CUR,database);
    //PrintCenteredX("Test2",40);
    Ypos+=15;
    //PrintCenteredX(&picNames[i*BYTES_PER_IMAGE_NAME],Ypos);

  }

  /* Keypress handler */
  gfx_SetTextXY(10,10);
  printNames(startName, picNames, pics);
  do{
    //scans the keys for keypress
    kb_Scan();
    //checks if up or down arrow key were pressed
    key = kb_Data[7];
    down= key & kb_Down;
    up  = key & kb_Up;
    //if any key was pressed
    if(key){
      /* increases the name to start on and redraws the text */
      if(up){
        //PrintCenteredX("DOWN",10);
        startName++;
        if (startName>(pics-1)) //makes sure user can't scroll too far
        startName=pics-1;
        printNames(startName, picNames, pics);
      }
      //key = kb_Data[3];

      /* decreases the name to start on and redraws the text */
      if(down){
        //PrintCenteredX(" UP ",10);
        startName--;
        if (startName>MAX_IMAGES) //checks if startName underflowed from 0 to 16 million or something
        startName=0;
        printNames(startName, picNames, pics);
      }
    }


  }   while(kb_Data[6]!=kb_Clear);

  free(picNames);
}

/* This UI keeps the user selection in the middle of the screen. */
void printNames(uint24_t startName, char *picNames, uint24_t numOfPics){
  uint24_t i, Yoffset=0, y=0;

  //clears old text and sets up for new text
  gfx_SetTextScale(2,2);
  gfx_SetColor(0);
  gfx_FillRectangle_NoClip(0,0,140,240);
  gfx_SetColor(255);

  //re-draws UI lines
  gfx_HorizLine_NoClip(0,120,6);
  gfx_HorizLine_NoClip(134,120,6);
  gfx_HorizLine_NoClip(6,110,128);
  gfx_HorizLine_NoClip(6,130,128);
  gfx_VertLine_NoClip(6,110,20);
  gfx_VertLine_NoClip(134,110,21);

  /*if the selected start name is under 6, that means we need to start drawing
  * farther down the screen for the text to go in the right spot */
  if(startName<6){
    Yoffset = 70 - startName * Y_SPACING;
    startName = 0;

  }


  /* draw the text on the screen. Starts displaying the name at element start
  * then iterates until out of pics or about to draw off the screen */
  for(i=startName;i<numOfPics && y<180;i++){
    //calculates where the text should be drawn
    y = i * Y_SPACING + Y_MARGIN + Yoffset;

    /* debug stuff
    gfx_SetTextScale(1,1);
    gfx_SetTextXY(200,10);
    gfx_PrintUInt(i,3);
    gfx_SetTextXY(200,20);
    gfx_PrintUInt(y,3);
    gfx_SetTextXY(200,30);
    gfx_PrintUInt(startName,3);
    gfx_SetTextXY(200,40);
    gfx_PrintUInt(numOfPics,3);
    gfx_SetTextScale(2,2);
    */

    gfx_PrintStringXY(&picNames[i*BYTES_PER_IMAGE_NAME],X_MARGIN,y);
    //while(!os_GetCSC());

  }
  //slows down scrolling speed
  delay(250);
}






/* Rebuilds the database of images on the calculator*/

uint24_t rebuildDB(uint8_t p){
  char *var_name, *imgInfo[16], nameBuffer[10];
  uint8_t *search_pos = NULL;
  uint24_t imagesFound=0;
  char myData[8]="HDPALV1",names[8];
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
  gfx_SetTextXY(100,195);
  gfx_PrintUInt(imagesFound,3);
  if (imagesFound==0){
    noImagesFound();
  }
  SetLoadingBarProgress(++p,TASKS_TO_FINISH);



  return imagesFound;
}

void noImagesFound(){
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
uint8_t databaseReady(){
  char *var_name;
  uint8_t *search_pos = NULL, exists=0, ready = 0;
  ti_var_t myAppVar;
  char myData[9]="HDDATV10"; //remember have one more space than text you're saving
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
void SetLoadingBarProgress(uint8_t p, uint8_t t){
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
  /* Print a string */
  PrintCentered("HD Picture Viewer");
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
void printText(int8_t xpos, int8_t ypos, const char *text) {
  os_SetCursorPos(ypos, xpos);
  os_PutStrFull(text);
}
