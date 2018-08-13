[i]HDPICV
//Comments coming soon

0->APOS
0->TOTALPICS
10->THUMBNAILX
20->THUMBNAILY

Pause 
Begin
While Detect(^^oAPOS,"HDPICV2L"->LPIC
   CloseAll
   Open(LPIC,"r->LPICSLOT
   If LPICSLOT=0
      Goto WRONGPIC
   End
   0->BPOS
   
   Lbl WRONGPIC
   While Detect(^^oBPOS,"HDPICV2R"->RPIC
      Seek(8,0,LPICSLOT   // important!
      Open(RPIC,"r->RPICSLOT
      If RPICSLOT=0
         Goto WRONGPIC
      End
     
      Seek(8,0,RPICSLOT
      0->WRONGPIC
      For(CHAR,0,7
         If GetChar(LPICSLOT)!=GetChar(RPICSLOT
            Close(RPICSLOT
            Goto WRONGPIC
         End
      End
      Goto CORRECTPIC
   End
	Goto STOP
	
   Lbl CORRECTPIC
   3+TOTALPICS->TOTALPICS
   3+TOTALPICS->TOTALPICS
   
   10+120*((remainder(TOTALPICS,12)=0))->LPICX+(*{GetDataPtr(LPICSLOT)})/2->RPICX
   10+60*((TOTALPICS/12)*remainder(TOTALPICS,12)!=0)->LPICY->RPICY

   RotatedScaledSprite_NoClip(GetDataPtr(LPICSLOT),LPICX,LPICY,0,32
   RotatedScaledSprite_NoClip(GetDataPtr(RPICSLOT),RPICX,RPICY,0,32
   Pause 
   
   
End
Lbl STOP
CloseAll
det(1
Disp "End!
Pause 