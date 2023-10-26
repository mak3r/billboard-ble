/*********************************************************************
 BLE Uart peripheral example for nRF52

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <ArduinoJson.h>
#include <bluefruit.h>
#include <BLEPeripheral.h>
#include <ContentManager.h>
#include <Adafruit_Protomatter.h>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// BLE Service
BLEPeripheral blep; // uart over ble
BLEUart BLEPeripheral::bleuart;
ContentManager cm;
int8_t ch = 0;

// Special nRF52840 FeatherWing pinout
uint8_t rgbPins[]  = {6, A5, A1, A0, A4, 11};
uint8_t addrPins[] = {10, 5, 13, 9};
uint8_t clockPin   = 12;
uint8_t latchPin   = PIN_SERIAL1_RX;
uint8_t oePin      = PIN_SERIAL1_TX;

Adafruit_Protomatter matrix(
  64,          // Width of matrix (or matrix chain) in pixels
  5,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  4, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  true);      // double-buffering here (see "doublebuffer" example)

int16_t  textX;        // Current text position (X)
int16_t  textY;        // Current text position (Y)
int16_t  textMin;      // Text pos. (X) when scrolled off left edge
char     str[64];      // Buffer to hold scrolling message text


void setup()
{
  Serial.begin(115200);

#if CFG_DEBUG
  // Blocking wait for connection when debug mode is enabled in the IDE
  while ( !Serial ) yield();
#endif

  // Initialize the ContentManager
  cm.begin();
  
  // Initialize BLE
  blep.begin();

  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error.
    for(;;);
  }

  // Post the first message
  updateMessage();
}



void loop() {
  //Listen for messages sent from the controller
  while ( BLEPeripheral::bleuart.available() )
  {
    ch = (int8_t) BLEPeripheral::bleuart.read();
    input_control(ch);
    updateMessage();
  }

  //Listen for messages from Serial
  while (Serial.available())
  {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    ch = (int8_t) Serial.read();
    if (ch != '\n') {
      input_control(ch);
      updateMessage();
    }
  }

  //We don't wrap if we are going to scroll
  if (cm.cur.scroll > 0) {
    //Handle scrolling text
    matrix.fillScreen(convertColor(cm.cur.bg));
    matrix.setCursor(textX,textY);
    // Update text position for next frame. If text goes off the
    // left edge, reset its position to be off the right edge.
    if((--textX) < textMin) textX = matrix.width();
    matrix.print(cm.cur.text);
    matrix.show();
    delay(cm.cur.scroll*100); // 20 milliseconds = ~50 frames/second
  }
}

void updateMessage() {

  matrix.fillScreen(convertColor(cm.cur.bg));

  // matrix.fillScreen(0xF8FCFF & strtoul(cm.cur.bg, 0, 16));
  matrix.setTextColor(convertColor(cm.cur.fg));
  matrix.setTextSize(1);

  //Must resolve text wrap before setting the bounds
  if (cm.cur.scroll > 0) {
    matrix.setTextWrap(false);
    matrix.setFont(&FreeSansBoldOblique18pt7b);
  } else {
    matrix.setTextWrap(true);
    matrix.setFont();
  }

  initializeCursor();

  matrix.print(cm.cur.text);
  matrix.show();
}

/*
Convert a hex string color from the format 0xAABBCC
to an RGB565 color
*/
uint16_t convertColor(const char * color) {
  const char red[3] = {color[2], color[3], '\0'};
  const char green[3] = {color[4], color[5], '\0'};
  const char blue[3] = {color[6], color[7], '\0'};
  // Serial.printf("HEX - (bg)red:green:blue(%s)[%x:%x:%x]\n", color, (uint8_t)strtoul(red, 0, 16), strtoul(green, 0, 16), strtoul(blue, 0, 16));
  // Serial.printf("DEC - red:green:blue[%d:%d:%d]\n", (uint8_t)strtoul(red, 0, 16), strtoul(green, 0, 16), strtoul(blue, 0, 16));
  
  return matrix.color565( (uint8_t)strtoul(red,0,16), (uint8_t)strtoul(green,0,16), (uint8_t)strtoul(blue,0,16) );
}

void initializeCursor() {
  // Serial.printf("matrix w:h[%d:%d]\n", matrix.width(), matrix.height());
  int16_t  x1, y1;
  uint16_t w, h;
  matrix.getTextBounds(cm.cur.text, 0, 0, &x1, &y1, &w, &h); // How big is it?
  // Serial.printf("x1:y1[%d:%d], w:h[%d:%d]\n", x1, y1, w, h);
  textMin = -w; // All text is off left edge when it reaches this point

  //Center Y, Center X unless we are scrolling, then start offscreen 
  if (cm.cur.scroll > 0) {
    textX = matrix.width();
  } else {
    textX = matrix.width()/2 - (x1 + w / 2);
  }
  textY = matrix.height() / 2 - (y1 + h / 2); // Center text vertically

  matrix.setCursor(textX,textY);
  // Serial.printf("textMin: [%d], textX: [%d], textY: [%d]\n", textMin, textX, textY);

}

void input_control(int8_t ch) {
  if (ch == 'p') {
    cm.prevMessage();
    cm.dumpMessage(cm.cur);
    serialize(cm.cur);
  }
  if (ch == 'n') {
    cm.nextMessage();
    cm.dumpMessage(cm.cur);
    serialize(cm.cur);
  }
  if (ch == 'd') {
    cm.dumpDoc();
  }
}

void serialize(struct ContentManager::message &msg) {
  StaticJsonDocument<128> doc;

  doc["text"] = msg.text;
  doc["bg"] = msg.bg;
  doc["fg"] = msg.fg;
  doc["rate"] = msg.scroll;
  
  serializeJson(doc, BLEPeripheral::bleuart);
}

