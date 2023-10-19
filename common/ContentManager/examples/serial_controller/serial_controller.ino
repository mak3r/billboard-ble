#include <ContentManager.h>

ContentManager cm;
int8_t incomingByte = 0;

void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("ContentManager example.");
  cm.begin();
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    if (incomingByte == 'n') {
      cm.nextMessage();
      cm.dumpMessage(cm.cur);
    } else if (incomingByte == 'p') {
      cm.prevMessage();
      cm.dumpMessage(cm.cur);
    } else if (incomingByte == 'd') {
      cm.dumpDoc();
    } else {
      return;
    }

  }
}
