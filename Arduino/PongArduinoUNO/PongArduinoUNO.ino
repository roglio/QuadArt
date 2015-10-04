// PONG demo for 16X32 RGB LED matrix
// (Arduino UNO Version)
// Pong classic game on the 16x32 RGB LED matrix:


// Written by Lluis Toyos (ltoyos@gmail.com) @tolbier
// Based on TFT Pong Arduino Example 
// http://arduino.cc/en/Tutorial/TFTPong


// and RGBmatrixPanel examples
// https://github.com/adafruit/RGB-matrix-Panel
// and Adafruit-GFX-Library
// https://github.com/adafruit/Adafruit-GFX-Library

// The Wiring setup is the same as in RGB-matrix-Panel examples, 
// described at 
// http://learn.adafruit.com/32x16-32x32-rgb-led-matrix/wiring-the-16x32-matrix

// also it has to connect 2 10k potentiometers at A4 and A5 as described in 
// http://arduino.cc/en/Tutorial/TFTPong

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

#define CLK  8 // MUST be on PORTB!
#define LAT 10
#define OE  9
#define A   A0
#define B   A1
#define C   A2
#define D   A3
// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);
uint16_t black = matrix.Color444(0, 0, 0);
uint16_t white = matrix.Color444(15, 15, 15);
uint16_t yellow = matrix.Color444(15, 15, 0);

int paddleWidth=5;
int paddleHeight=1;

int ballDiameter=1;
int paddleX = 0;
int paddleY = 0;
int oldPaddleX, oldPaddleY;
int ballDirectionX = 1;
int ballDirectionY = 1;

int ballSpeed = 200; //lower numbers are faster

int ballX, ballY, oldBallX, oldBallY;

void setup() {
  matrix.begin();
  clear();
}

void clear() {
  matrix.fillScreen(black);
}

void loop() {
  int myWidth = matrix.width();
  int myHeight = matrix.height();
  int a0= analogRead(A4);
  int a1= analogRead(A5);
  
  paddleX = map(a0, 0, 1023, 0, myWidth)- paddleWidth/2 ;
  paddleY = map(a1, 0, 1023, 0, myHeight)-paddleHeight/2 ;

  if (oldPaddleX != paddleX || oldPaddleY != paddleY) {
    matrix.fillRect(oldPaddleX, oldPaddleY, paddleWidth, paddleHeight,black);
  }
  
  matrix.fillRect(paddleX, paddleY, paddleWidth, paddleHeight,white);
  
  oldPaddleX = paddleX;
  oldPaddleY = paddleY;
  if (millis() % (ballSpeed/2) < 2) {
    moveBall();
  }
}

void quicker(){
    if (ballSpeed>20) ballSpeed--;
}

void moveBall() {
  // if the ball goes offscreen, reverse the direction:
  if (ballX > matrix.width()-1 || ballX < 0) {
    ballDirectionX = -ballDirectionX;
    quicker();
  }
 
  if (ballY > matrix.height()-1 || ballY < 0) {
    ballDirectionY = -ballDirectionY;
    quicker();
  }  
 
  // check if the ball and the paddle occupy the same space on screen
  if (inPaddle(ballX, ballY, paddleX, paddleY, paddleWidth, paddleHeight)) {
    //    ballDirectionX = -ballDirectionX;
    ballDirectionY = -ballDirectionY;
    quicker();
  }
 
  // update the ball's position
  ballX += ballDirectionX;
  ballY += ballDirectionY;
 
  // erase the ball's previous position
  if (oldBallX != ballX || oldBallY != ballY) {
    matrix.fillRect(oldBallX, oldBallY, ballDiameter, ballDiameter,black);
  }
 
  // draw the ball's current position
  matrix.fillRect(ballX, ballY, ballDiameter, ballDiameter,white);
 
  oldBallX = ballX;
  oldBallY = ballY;
}

boolean inPaddle(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight) {
  boolean result = false;
   
  if ((x >= rectX && x <= (rectX + rectWidth)) &&
      (y >= rectY && y <= (rectY + rectHeight))) {
       result = true;
  }
 
  return result;  
}
