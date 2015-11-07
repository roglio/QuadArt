/*
  SDWebServer - Example WebServer with SD Card backend for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.htm is the default index (works on subfolders as well)

  upload the contents of SdRoot to the root of the SDcard and access the editor by going to http://esp8266sd.local/edit

*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

//#include "image444.h"
#include "gamma_table.h"

#define DBG_OUTPUT_PORT Serial

/*
 * ESP8266-12        HY-1.8 SPI
 * GPIO5             Pin 06 (RESET)
 * GPIO2             Pin 07 (A0)
 * GPIO13 (HSPID)    Pin 08 (SDA)
 * GPIO14 (HSPICLK)  Pin 09 (SCK)
 * GPIO15 (HSPICS)   Pin 10 (CS)
 */

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
#define TFT_CS  15  // Chip select line for TFT display
#define TFT_RST  5  // Reset line for TFT (or see below...)
#define TFT_DC   2  // Data/command line for TFT
#define SD_CS    4  // Chip select line for SD card
#define TILE_CS 16

const char* ssid = "sardaricambi";
const char* password = "retedomestica";
const char* host = "esp8266sd";

ESP8266WebServer server(80);

static bool hasSD = false;
File uploadFile;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void returnOK() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSdCard(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if(dataFile.isDirectory()){
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    if(SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.println(upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void deleteRecursive(String path){
  File file = SD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if(path.indexOf('.') > 0){
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if(!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if(path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;

    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
 }
 server.sendContent("]");
 dir.close();
}

void handleNotFound(){
  if(hasSD && loadFromSdCard(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  DBG_OUTPUT_PORT.print(message);
}

void setup(void){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  //init the TFT screen and clear it
  tft.initR(INITR_BLACKTAB); // Use this initializer if you're using a 1.8" TFT
  tft.setRotation(1); //portrait
  tft.fillScreen(ST7735_BLACK); //clear all

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if(i == 21){
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while(1) delay(500);
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  if(MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }

  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, [](){ returnOK(); });
  server.onNotFound(handleNotFound);
  server.onFileUpload(handleFileUpload);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  if(SD.begin(SD_CS)){
     DBG_OUTPUT_PORT.println("SD Card initialized.");
     hasSD = true;
  }

  //SPI master setup. Slave select is active-low.
  pinMode(TILE_CS,OUTPUT);
  digitalWrite(TILE_CS,HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
/*
  bmpDraw("pac-man.bmp", 0, 0);
  delay(500);
  bmpDraw("kybertht.bmp", 0, 0);
  delay(500);
  bmpDraw("aragona.bmp", 0, 0);
  delay(500);
  bmpDraw("comune.bmp", 0, 0);
  delay(500);
  bmpDraw("sella.bmp", 0, 0);
  delay(500);
  bmpDraw("stemmaco.bmp", 0, 0);
  delay(500);
  bmpDraw("stemmabw.bmp", 0, 0);
  delay(500);
  bmpDraw("pac-man.bmp", 0, 0);
  delay(500);
  bmpDraw("kybertht.bmp", 0, 0);
  delay(500);
  bmpDraw("aragona.bmp", 0, 0);
  delay(500);
  bmpDraw("comune.bmp", 0, 0);
  delay(500);
  bmpDraw("sella.bmp", 0, 0);
  delay(500);
  bmpDraw("stemmaco.bmp", 0, 0);
  delay(500);
  bmpDraw("stemmabw.bmp", 0, 0);
  delay(500);
*/
}
/*
enum SM_STATUSES {SM_INITSLIDES, SM_LOADNEXT, SM_WAIT};

void SM(){
  static byte status = SM_INITSLIDES;
  static byte current_slide, current_delay, total_slides;
  static int start_time;

  switch(status) {
    case SM_LOADNEXT:
Serial.println("SM_LOADNEXT:");
Serial.print("slide : "); Serial.println(current_slide,DEC);
      if(current_slide >= total_slides) status=SM_INITSLIDES;
      else status=SM_WAIT;
      //get current filename and delay
      //call bmpDraw to load and display the image
      //increase counter
      //restart the loop
Serial.print("delay = "); Serial.println(current_delay);
      current_slide++;
      start_time = millis();
      break;
    case SM_WAIT:
Serial.println("SM_WAIT:");
      if((millis()-start_time)>=current_delay) status=SM_LOADNEXT;
      break;
    case SM_INITSLIDES:
Serial.println("SM_INITSLIDES:");
      current_slide=0; current_delay=0;
      //load slideshow list, count slides
      //alloc needed memory
      //populate the list
      status=SM_LOADNEXT;
      break;
    default:
Serial.println("SM_DELIRIOOOOOO:");
      break;
  }
}
*/
File img_folder;

void loop() {
//  SM();

  img_folder = SD.open("/images/");
  if(!img_folder)
    Serial.println("Cannot open /images/ folder");

  processDir(img_folder);

  server.handleClient();
}

void processDir(File dir) {
   while(true) {
     File entry =  dir.openNextFile();

     if(!entry) break; // no more files

     if (entry.isDirectory()) {
       processDir(entry);
     } else {
      String filename = entry.name();
      if(filename.endsWith(".bmp") || filename.endsWith(".BMP")) {
        Serial.print("processing file: ");
        Serial.println(filename);
        bmpDraw(entry,0,0);
        delay(500);
      }
     }
     entry.close();
   }
}


// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(File bmpFile, uint8_t x, uint8_t y) {

//  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;
/*
  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print("File not found");
    return;
  }
*/
  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);
  
        //split each tile, fill the framebuffer and send while loading
        for(int tr=0;tr<4;tr++){
          for(int tc=0;tc<5;tc++){
            tft.setAddrWindow(tc*32, tr*32, tc*32+32-1, tr*32+32-1);
              for (row=tr*32; row<tr*32+32; row++) { // For each scanline...
                // Seek to start of scan line
                if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
                  pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
                else     // Bitmap is stored top-to-bottom
                  pos = bmpImageoffset + row * rowSize;

                //skip previous tiles
                pos+=tc*32*3;

                if(bmpFile.position() != pos) { // Need seek?
                  bmpFile.seek(pos);
                  buffidx = sizeof(sdbuffer); // Force buffer reload
                }

                for (col=0; col<32; col++) { // For each pixel...
                  // Time to read more pixel data?
                  if (buffidx >= sizeof(sdbuffer)) { // Indeed
                    bmpFile.read(sdbuffer, sizeof(sdbuffer));
                    buffidx = 0; // Set index to beginning
                  }
      
                  // Convert pixel from BMP to TFT format, push to display
                  b = sdbuffer[buffidx++];
                  g = sdbuffer[buffidx++];
                  r = sdbuffer[buffidx++];
                  tft.pushColor(tft.Color565(r,g,b));
                  //convert pixel from rgb888 to rgb444 and write it into framebuffer
                  RGBmatrixPanelDrawPixel(col,row%32,RGBmatrixPanelColor888(r,g,b));
                } // end pixel
              } // end scanline
            //send framebuffer to each tile from 0 to 19
            send2tile(0x30+(tc+tr*5));
          } //end tile's columns loop
        } //end tile's rows loop
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

//  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


#define nPlanes 4
#define nRows 16
#define WIDTH 32
#define HEIGHT 32

uint8_t matrixbuff[1536];

void RGBmatrixPanelDrawPixel(int16_t x, int16_t y, uint16_t c) {
  uint8_t r, g, b, bit, limit, *ptr;

  // Adafruit_GFX uses 16-bit color in 5/6/5 format, while matrix needs
  // 4/4/4.  Pluck out relevant bits while separating into R,G,B:
  r =  c >> 12;        // RRRRrggggggbbbbb
  g = (c >>  7) & 0xF; // rrrrrGGGGggbbbbb
  b = (c >>  1) & 0xF; // rrrrrggggggBBBBb

  // Loop counter stuff
  bit   = 2;
  limit = 1 << nPlanes;

  if(y < nRows) {
    // Data for the upper half of the display is stored in the lower
    // bits of each byte.
    ptr = &matrixbuff[y * WIDTH * (nPlanes - 1) + x]; // Base addr
    // Plane 0 is a tricky case -- its data is spread about,
    // stored in least two bits not used by the other planes.
    ptr[WIDTH*2] &= ~B00000011;           // Plane 0 R,G mask out in one op
    if(r & 1) ptr[WIDTH*2] |=  B00000001; // Plane 0 R: 64 bytes ahead, bit 0
    if(g & 1) ptr[WIDTH*2] |=  B00000010; // Plane 0 G: 64 bytes ahead, bit 1
    if(b & 1) ptr[WIDTH]   |=  B00000001; // Plane 0 B: 32 bytes ahead, bit 0
    else      ptr[WIDTH]   &= ~B00000001; // Plane 0 B unset; mask out
    // The remaining three image planes are more normal-ish.
    // Data is stored in the high 6 bits so it can be quickly
    // copied to the DATAPORT register w/6 output lines.
    for(; bit < limit; bit <<= 1) {
      *ptr &= ~B00011100;            // Mask out R,G,B in one op
      if(r & bit) *ptr |= B00000100; // Plane N R: bit 2
      if(g & bit) *ptr |= B00001000; // Plane N G: bit 3
      if(b & bit) *ptr |= B00010000; // Plane N B: bit 4
      ptr  += WIDTH;                 // Advance to next bit plane
    }
  } else {
    // Data for the lower half of the display is stored in the upper
    // bits, except for the plane 0 stuff, using 2 least bits.
    ptr = &matrixbuff[(y - nRows) * WIDTH * (nPlanes - 1) + x];
    *ptr &= ~B00000011;                  // Plane 0 G,B mask out in one op
    if(r & 1)  ptr[WIDTH] |=  B00000010; // Plane 0 R: 32 bytes ahead, bit 1
    else       ptr[WIDTH] &= ~B00000010; // Plane 0 R unset; mask out
    if(g & 1) *ptr        |=  B00000001; // Plane 0 G: bit 0
    if(b & 1) *ptr        |=  B00000010; // Plane 0 B: bit 0
    for(; bit < limit; bit <<= 1) {
      *ptr &= ~B11100000;            // Mask out R,G,B in one op
      if(r & bit) *ptr |= B00100000; // Plane N R: bit 5
      if(g & bit) *ptr |= B01000000; // Plane N G: bit 6
      if(b & bit) *ptr |= B10000000; // Plane N B: bit 7
      ptr  += WIDTH;                 // Advance to next bit plane
    }
  }
}

uint16_t RGBmatrixPanelColor888(uint8_t r, uint8_t g, uint8_t b) {
  r = pgm_read_byte(&gamma_table[r]); // Gamma correction table maps
  g = pgm_read_byte(&gamma_table[g]); // 8-bit input to 4-bit output
  b = pgm_read_byte(&gamma_table[b]);
  return ((uint16_t)r << 12) | ((uint16_t)(r & 0x8) << 8) | // 4/4/4->5/6/5
         ((uint16_t)g <<  7) | ((uint16_t)(g & 0xC) << 3) |
         (          b <<  1) | (           b        >> 3);
}

//this function targets each tile by name and send out the framebuffer
void send2tile(byte tile) {
  int p;

  digitalWrite(TILE_CS, LOW);
  SPI.transfer(tile);
  delay(10);
  for(p=0;p<1536;p++)
    SPI.transfer(matrixbuff[p]);
  digitalWrite(TILE_CS, HIGH);
  delay(50);
}

/*
void playAnim(byte tile) {
  int i, p, frames = sizeof(image)/sizeof(image[0]);
  unsigned char c;

  for(i=0;i<frames;i++) {
    digitalWrite(TILE_CS, LOW);
    SPI.transfer(tile);
    delay(10);
    //SPI.transfer(&image[i],1536);
    for(p=0;p<1536;p++) {
      c = pgm_read_byte(&image[i][p]);
      SPI.transfer(c);
    }
    digitalWrite(TILE_CS, HIGH);
    delay(100);
  }
}
*/
