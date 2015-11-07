// SPI slave example Written by Nick Gammon on February 2011

#include <EEPROM.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <RGBmatrixPanel.h>
#include "image444.h"

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE  9
#define LAT A4 // 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

byte qatile_id;

/*
  0x30 0x31 0x32 0x33 0x34
  0x35 0x36 0x37 0x38 0x39
  0x3A 0x3B 0x3C 0x3D 0x3E
  0x3F 0x40 0x41 0x42 0x43
*/

uint8_t *ptr;

void setup() {
  //turn on SPI in slave mode
  SPCR |= bit (SPE);

  //initialize the matrix library (i.e. interrupts)
  matrix.begin();

  // have to send on master in, *slave out*
  // set SS as input pullup to avoid floating values
  pinMode(MISO, OUTPUT);
  pinMode(SS, INPUT_PULLUP);

  //clear the matrix and get the framebuffer pointer
  matrix.fillScreen(matrix.Color333(0,0,0));
  ptr = matrix.backBuffer();

  //call the welcome show
  welcome();

  //cleanup before showcase
  matrix.fillScreen(matrix.Color333(0,0,0));

  //read it's own identity from EEPROM's address 0
  qatile_id = EEPROM.read(0);
  //if id is not in range, then listen for all frames
  if(qatile_id<0x30 || qatile_id>0x43) qatile_id=0;

  //show tile id for easy placement on the table
  matrix.setCursor(1, 1);    // start at top left, with one pixel of spacing
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves
  matrix.setTextColor(matrix.Color333(1,1,1));
  matrix.println((qatile_id!=0 ?(qatile_id-0x30+1) :0),DEC);
}

//this function will be developed by each individual
void welcome() {
  int frames = sizeof(image)/sizeof(image[0]);
  unsigned long startup = millis();

  while(millis()-startup < 3000) {
    for(int i=0;i<frames;i++){
      memcpy_P(ptr, image[i], 1536);
      delay(100);
    }
  }
}

bool ignore = false;

void loop() {
  bool tx = !digitalRead(SS); //SS = 10
  unsigned char c;
  uint8_t *p = ptr;
  unsigned int i;

  if(ignore && !tx) ignore=false;

  if(tx && !ignore) {
    //stop the matrix refresh to avoid interferences with SPI
    noInterrupts();
    //get the message's tile_id
    c = SPI.transfer(0);
    //if it is mine then get the framebuffer
    //otherwise ignore the message until SS go up again
    //but get all frames if my id is zero or
    //sent id is 0xFF
    if(c==qatile_id || qatile_id==0 || c==0xFF) {
      for (i = 0; i < 1536; i++)
        *p++ = SPI.transfer(0);
    } else {
      ignore = true;
    }
    //restore interrupts to keep the matrix updated
    interrupts();
  }
}






