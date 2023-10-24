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

// BLE Service
BLEPeripheral blep; // uart over ble
BLEUart BLEPeripheral::bleuart;
ContentManager cm;
int8_t ch = 0;
    
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

}



void loop() {
  //Listen for messages sent from the controller
  while ( BLEPeripheral::bleuart.available() )
  {
    ch = (int8_t) BLEPeripheral::bleuart.read();
    input_control(ch);
  }

  //Listen for messages from Serial
  while (Serial.available())
  {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    ch = (int8_t) Serial.read();
    input_control(ch);
  }
}

void input_control(int8_t ch) {
  if (ch == 'p') {
    cm.prevMessage();
    cm.dumpMessage(cm.cur);
    // bleuart.write(cm.cur.text);
    serialize(cm.cur);
  }
  if (ch == 'n') {
    cm.nextMessage();
    cm.dumpMessage(cm.cur);
    // bleuart.write(cm.cur.text);
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

