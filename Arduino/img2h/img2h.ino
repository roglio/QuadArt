/* QuadArt 2.0 - cooperative tiled led matrix display project       */
/* this is a companion scketch for the img2h.pde converter          */
/* the .h file created by the Processing counterpart will be loaded */
/* and shown on an attached adafuit style 32x32 led matrix display  */
/* credits for the original version goes to: Markus Lipp            */
/* http://forums.adafruit.com/viewtopic.php?f=47&t=50115            */
/* starting from the original version I added the ability show a    */
/* sequence of preloaded frames                                     */
/* 2015/10/05 leandro.zaccaria@gmail.com CC BY-SA 3.0 for AUGC      */
/* https://www.facebook.com/groups/1376364222668338/                */

#include <Adafruit_GFX.h>
#include <RGBmatrixPanel.h>
#include "image444.h"

// If your 32x32 matrix has the SINGLE HEADER input,
// use this pinout:
#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3
// If your matrix has the DOUBLE HEADER input, use:
//#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
//#define LAT 9
//#define OE  10
//#define A   A3
//#define B   A2
//#define C   A1
//#define D   A0
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

void setup() {
  matrix.begin();
}

void loop() {
  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0,0,0));
  delay(500);

  uint8_t *ptr = matrix.backBuffer(); // Get address of matrix data

  int frames = sizeof(image)/sizeof(image[0]);

  for(;;) {
    for(int i=0;i<frames;i++){
      memcpy_P(ptr, image[i], 1536);
      delay(100);
    }
  }
}
