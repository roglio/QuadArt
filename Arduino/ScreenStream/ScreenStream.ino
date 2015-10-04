    // Fast Serial Screen Streaming, written by Markus Lipp, based on examples from AdaFruit
    // BSD license, all text above must be included in any redistribution.

    #include <Adafruit_GFX.h>   // Core graphics library
    #include <RGBmatrixPanel.h> // Hardware-specific library

    #define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
    #define OE  9
    #define LAT 10
    #define A   A0
    #define B   A1
    #define C   A2
    #define D   A3
    RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

    int pos=0;

    uint8_t* buffer;
    int bufferLength;

    void setup() { 
      matrix.begin();
      Serial.begin(115200);
     
      pos=0;
      bufferLength = 1536;
      buffer = matrix.backBuffer();   
    }

    byte prevVal,val;
    void loop() {   
       if (Serial.available())
       {     
          prevVal = val;
          val = Serial.read();         
          //prepare RGB-Matrix buffer (including gamma correction) directly on PC and stream
          if ( (prevVal==0x21 && val==0x8) //magic numbers     
              || pos>=bufferLength)
            {
              pos=0;
            } else
            {
              buffer[pos++]=val;
            }           
       }
    }
