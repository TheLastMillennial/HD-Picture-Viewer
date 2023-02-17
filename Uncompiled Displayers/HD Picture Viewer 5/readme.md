# High-Definition Picture Viewer
*For the TI-84 Plus CE (-T) (Python) and the TI-83 Premium CE (Python)*

By TheLastMillennial

## How To Use

### Converting Images
Convert an image using the HD Picture Viewer Converter: https://github.com/TheLastMillennial/HDPictureViewerConverter2/releases
Refer to the HD Picture Viewer Converter's readme.md file for instructions on converting an image. 

Alternatively, you can use a web converter created and maintained by Peter Marheine: https://taricorp.gitlab.io/hdpictureconverter-rs/
Please note I cannot guarantee the web converter's compatibility with this program.

### Tutorial:
- Send "HDPICV.8xp" and any converted image(s) to your calculator.
- Run the program on your calculator. 
- Refer to this troubleshooting video if you encounter any issues: https://www.youtube.com/watch?v=-TweNnHuFCQ
- If any images are on the calculator, it will immediately display one.

#### Buttons:
- Use [y=] to view previous image
- Use [graph] to view next image.
- Use arrow keys to pan around image.
- Use [+] to zoom in.
- Use [-] to zoom out.
- Use [zoom] to reset pan and zoom to default.
- Use [del] to immediately delete the current image.
- Use [clear] to exit an image and to quit the program

#### Example Image:
- Sending all the files from the `Example Image` folder is a good start. They are known to work with this program.
- The photo is of a cute puppy! It was photographed by Alex Glanville. You can see more of his work on his instagram: @alex.takes.pics

## Programs 
- [Wine](https://www.winehq.org/) (Only needed if you are on Mac or Linux)

## FAQ

### How do I quit?
Press [clear]. If that's not working, press and hold the reset button on the back of the calculator for at least two seconds.

### How many pictures can I put on the calculator?
This depends on how many files you already have on the calculator, how large each picture is, and how well it can be compressed. In an ideal situation, you could fit 833 pictures at native 320x240 resolution. I tested this by sending over 10,000 files to my calculator. It was physically unable to handle anymore files. A more realistic answer would be about 50 pictures total.

### The picture won't show up!
This is always because not all of the files actually sent to the calculator, or they were accidentally deleted. Try re-sending them. Press [2nd] then [+] then [2] and scroll down to AppVars. Verify all the files are on the calculator.

## Changelog
*Most recent update first*

### v2.0.0-alpha
- Completely re-wrote program in C.
- Images are no longer restricted to 320x240 pixels; there is no restriction.
  - A single image can take up the full storage of the calculator, if it wanted.
- Added delete button.
- Added zooming.
- Added panning.
- Added reset zoom and pan button.
- Shrunk image size significantly so more can fit on the calculator.

### NOTE: Old Version Naming Below!

### v4.0.273
- Fixed bug where images weren't detected when run via Cesium

### v4.0.256
- First full release!
- Added VAT sorting.
- Fixed minor visual bugs.

### v4.0.208
- Redesigned GUI completely.
- Improved image quality drastically.
- Included image converter application.
- Updated ReadMe.
- Updated example images.

### v4.0.101
- Added icon.
- Added description.
- Updated ReadMe.
- Updated example images.

### v4.0.100
- First Beta release.

### v3.40
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

