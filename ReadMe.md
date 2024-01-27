# High-Definition Picture Viewer
*For the TI-84 Plus CE (-T) (Python) and the TI-83 Premium CE (Python)*

By TheLastMillennial

## How To Use

### Full Video Tutorial:
https://youtu.be/uixL9t5ZTJs

### Converting Pictures:
Convert an picture using the HD Picture Viewer Converter: https://github.com/TheLastMillennial/HDPictureViewerConverter2/releases
Refer to the HD Picture Viewer Converter's readme.md file for instructions on converting an picture. 

Alternatively, you can use a web converter created and maintained by Peter Marheine: https://taricorp.gitlab.io/hdpictureconverter-rs/
Please note I cannot guarantee the web converter's compatibility with this program.

### Written tutorial:
- Send "HDPICV.8xp" to the *archive memory* of your calculator.
- Send all converted picture files to the *archive memory* of your calculator.
- Refer to this troubleshooting video if you encounter any issues sending files to the calculator.
  - https://www.youtube.com/watch?v=-TweNnHuFCQ
- New OS version will require the HDPICV program to be run through the Artifice Jailbreak:
  - https://www.youtube.com/watch?v=abB0ZEdQ1rs
- When the program is run, if any pictures are on the calculator, you will see a list of picture names on the main menu.
- Press Enter to open an image in full screen.
- Press Clear to exit.

#### Buttons:
Keymap in Main Menu:
Clear ______ Quit program.
Mode _______ Open help.
Enter ______ Open picture fullscreen.
Up _________ Select previous picture.
Down _______ Select next picture.

Keymap in Fullscreen:
Clear ______ Quit to menu.
Mode _______ Open help.
Y= _________ Show previous.
Graph ______ Show next.    
Arrow Keys _ Pan picture.    
Del ________ Permenantly delete picture."
+ __________ Zoom in. 
- __________ Zoom out.
Zoom _______ Maximum zoom.
Window _____ Reset picture zoom.

#### Example picture:
- Sending all the files from one of the `Example` folders is a good start. They are known to work with this program.
- The example photo is of a cute puppy! It was photographed by Alex Glanville. Used with permission. You can see more of his work on his instagram: @alex.takes.pics

## FAQ

## Is there a Mac or Linux converter?
The first-party converter is only designed for Windows. You can try using the alternative converter mentioned in the [Converting Pictures] section above.

### How do I quit?
Press [clear]. If that's not working, press and hold the reset button on the back of the calculator for at least two seconds.

### How many pictures can I put on the calculator?
This greatly depends on many factors: how many files you already have on the calculator, how large each picture is, and how well the pictures can be compressed. In an ideal situation, you could fit 833 pictures at native 320x240 resolution. I tested this by sending over 10,000 files to my calculator. It was physically unable to handle anymore files. A more realistic answer would be about 50 pictures total.

### The picture won't show up!
This is because not all of the files actually sent to the calculator, or they were accidentally deleted. Try re-sending them. Press [2nd] then [+] then [2] and scroll down to AppVars. Verify all the files are on the calculator.

## Changelog
*Most recent update first*

### v2.0.1
- Updated help
- Fixed versioning

### v2.0.0
- Added tutorial
- Added icon
- Updated help
- Fixed bug where button presses were detected too quickly

### v2.0.0-beta
- Converted to C++
- Added maximum zoom button
- Added GUI
- Added help
- Added picture converter to release
- Added loading bar when deleting images
- Reduced change in zoom level from 100% to 10%
- Fixed images not being scaled correctly
- Fixed inconsistent GUI
- Fixed not being able to scroll past first few pictures

### v2.0.0-alpha
- Completely re-wrote program in C.
- pictures are no longer restricted to 320x240 pixels; there is no restriction.
  - A single picture can take up the full storage of the calculator, if it wanted.
- Added delete button.
- Added zooming.
- Added panning.
- Added reset zoom and pan button.
- Shrunk picture size significantly so more can fit on the calculator.

### v1.0.1 (originally: v4.0.273)
- Fixed bug where pictures weren't detected when run via Cesium

### v1.0.0 (originally: v4.0.256)
- First full release!
- Added VAT sorting.
- Fixed minor visual bugs.

### v1.0.0-beta.3 (originally: v4.0.208)
- Redesigned GUI completely.
- Improved picture quality drastically.
- Included picture converter application.
- Updated ReadMe.
- Updated example pictures.

### v1.0.0-beta.2 (originally: v4.0.101)
- Added icon.
- Added description.
- Updated ReadMe.
- Updated example pictures.

### v1.0.0-beta.1 (originally: v4.0.100)
- First Beta release.

### v0.0.1 (originally: v3.40)
- Initial GitHub upload.

## Credits
Thank you to everyone who has helped me fix code and report bugs! 

- Matt "MateoConLechuga" Waltz
- Peter "PT_" Tillema
- Beckadamtheinventor
- commandblockguy
- calclover2514
- _iPhoenix_
- Hooloovoo
- Epsilon5
- Runer112
- Jacobly
- Iambian
- SahilS
- SM84CE
