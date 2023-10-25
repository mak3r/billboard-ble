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
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  4, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  true);      // No double-buffering here (see "doublebuffer" example)

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
    input_control(ch);
    updateMessage();
  }

  //We don't wrap if we are going to scroll
  if (cm.cur.scroll > 0) {
    Serial.println("Scrolling ...");
    //Handle scrolling text
    // Update text position for next frame. If text goes off the
    // left edge, reset its position to be off the right edge.
    if((--textX) < textMin) textX = matrix.width();
    matrix.show();
    delay(cm.cur.scroll*100); // 20 milliseconds = ~50 frames/second
  }
}

void updateMessage() {
  matrix.fillScreen(strtoul(cm.cur.bg, 0, 16));

  //Must resolve text wrap before setting the bounds
  if (cm.cur.scroll > 0) {
    matrix.setTextWrap(false);
  } else {
    matrix.setTextWrap(true);
  }

  int16_t  x1, y1;
  uint16_t w, h;
  matrix.getTextBounds(cm.cur.text, 0, 0, &x1, &y1, &w, &h); // How big is it?
  textMin = -w; // All text is off left edge when it reaches this point
  textX = matrix.width(); // Start off right edge
  textY = matrix.height() / 2 - (y1 + h / 2); // Center text vertically

  matrix.setCursor(textX,textY);
  matrix.setTextColor(strtoul(cm.cur.fg, 0, 16));
  matrix.setTextSize(1);
  matrix.print(cm.cur.text);
  matrix.show();
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

