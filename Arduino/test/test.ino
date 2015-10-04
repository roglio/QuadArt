// testshapes demo for Adafruit RGBmatrixPanel library.
// Demonstrates the drawing abilities of the RGBmatrixPanel library.
// For 32x32 RGB LED matrix:
// http://www.adafruit.com/products/607

// Written by Limor Fried/Ladyada & Phil Burgess/PaintYourDragon
// for Adafruit Industries.
// BSD license, all text above must be included in any redistribution.

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

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

#define DELAY 1000

void loop() {

  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0,0,0));
  delay(DELAY);

  // fill the screen with 'white'
  matrix.fillScreen(matrix.Color333(7,7,7));
  delay(DELAY);

  // fill the screen with 'red'
  matrix.fillScreen(matrix.Color333(7,0,0));
  delay(DELAY);

  // fill the screen with 'green'
  matrix.fillScreen(matrix.Color333(0,7,0));
  delay(DELAY);

  // fill the screen with 'blue'
  matrix.fillScreen(matrix.Color333(0,0,7));
  delay(DELAY);

  // colorbars!
  matrix.fillRect( 0,0, 4,16,matrix.Color333(3,3,3));
  matrix.fillRect( 4,0, 7,16,matrix.Color333(0,0,0));
  matrix.fillRect( 8,0,11,16,matrix.Color333(0,7,0));
  matrix.fillRect(12,0,15,16,matrix.Color333(0,0,7));
  matrix.fillRect(16,0,19,16,matrix.Color333(0,7,7));
  matrix.fillRect(20,0,23,16,matrix.Color333(7,0,0));
  matrix.fillRect(24,0,27,16,matrix.Color333(7,0,7));
  matrix.fillRect(28,0,31,16,matrix.Color333(7,7,0));
  matrix.fillRect(0,16,32,31,matrix.Color333(7,0,0));
  delay(DELAY);

  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0,0,0));

  // draw some text!
  matrix.setCursor(1, 0);    // start at top left, with one pixel of spacing
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves

  matrix.setTextColor(matrix.Color333(7,7,7));
  matrix.println(" Ada");
  matrix.println("fruit");
  
  // print each letter with a rainbow color
  matrix.setTextColor(matrix.Color333(7,0,0));
  matrix.print('3');
  matrix.setTextColor(matrix.Color333(7,4,0)); 
  matrix.print('2');
  matrix.setTextColor(matrix.Color333(7,7,0));
  matrix.print('x');
  matrix.setTextColor(matrix.Color333(4,7,0)); 
  matrix.print('3');
  matrix.setTextColor(matrix.Color333(0,7,0));  
  matrix.println('2');

  matrix.setTextColor(matrix.Color333(7,7,7)); 
  matrix.print('*');
  matrix.setTextColor(matrix.Color333(7,0,0)); 
  matrix.print('R');
  matrix.setTextColor(matrix.Color333(0,7,0));
  matrix.print('G');
  matrix.setTextColor(matrix.Color333(0,0,7)); 
  matrix.print("B");
  matrix.setTextColor(matrix.Color333(7,7,7)); 
  matrix.print("*");
  delay(DELAY*2);
  // whew!
}
