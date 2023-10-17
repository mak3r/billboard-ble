/*********************************************************************
 Read eeprom using littlfs
 
 Expectations (dependencies):
 1) there will be a file called content.json
 2) content.json contains a list of billboard contents

 Apache 2 license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <StreamUtils.h>
#include <ArduinoJson.h>

using namespace Adafruit_LittleFS_Namespace;

File file(InternalFS);

const uint8_t BILLBOARD_MAX_KEYS = 10;
const char * FILENAME = "content.json";
// Calculate approximate doc size https://arduinojson.org/v6/assistant
StaticJsonDocument<768> doc;
const char * KEYS[BILLBOARD_MAX_KEYS] = {NULL};
uint8_t LAST_ID = 0;

// Used for private method serial input handling
char incomingByte = 0;

struct message {
  uint8_t key_id = 0;
  const char * text = NULL;
  const char * fg = NULL;
  const char * bg = NULL;
  double scroll = 0;
} cur, prev, next;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  // Initialize Internal File System
  InternalFS.begin();

  // Load content into JSON document
  loadContentJSON();

  // configureMessages();
  updateMessages(0);
  _dumpMessage(cur);

  // Print prompt
  Serial.println();
}


void loop() {
  // put your main code here, to run repeatedly:
  while(1) {
    _serialController();
  }
}

void nextMessage() {
  updateMessages(cur.key_id + 1);
}

void prevMessage() {
  updateMessages(cur.key_id - 1);
}

/*
 Assumption: LittleFS contains a file named content.json
*/
void loadContentJSON()
{
  // Open the input folder
  File dir("/", FILE_O_READ, InternalFS);
 
  // File within folder
  File item(InternalFS);

  // Loop through the directory 
  while( (item = dir.openNextFile(FILE_O_READ)) )
  {
    // Look for the file with FILENAME and deserialize it
    // Serial.print("Loading file: ");
    // Serial.println( item.name() );
    if ( strcmp(item.name(), FILENAME) == 0 ) {
      // Buffering the file stream is faster
      // Without this, it reads byte-by-byte
      ReadBufferingStream bufferingStream(file, 64);

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, item);

      // Test if parsing succeeds.
      /* if the docsize is too small error is:
       ****** deserializeJson() failed: NoMemory ******
       */
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      uint8_t i = 0;
      for (JsonPair kv : doc.as<JsonObject>()) {
        //Serial.println(kv.key().c_str());  
        // cur.text = kv.key().c_str();
        KEYS[i] = kv.key().c_str();
        //Serial.println(KEYS[i]);
        i++;
      }
      LAST_ID = i-1;
    }    
    item.close();
  }
  dir.close();
}

/****** 
 Set the cur message with id and content according to the index of KEYS.
 Set next and prev accodingly
 TODO: throw a check in here to insure msg_id is in the range of 0 to the end of KEYS[]
 ******/
void updateMessages(int8_t msg_id) {
  int8_t cur_id = boundId(msg_id);
  int8_t next_id = boundId(cur_id + 1);
  int8_t prev_id = boundId(cur_id - 1);

  _populateMessage(&cur, cur_id);
  _populateMessage(&next, next_id);
  _populateMessage(&prev, prev_id);

}

int8_t boundId(int8_t id) {
  if (id < 0) {
    return LAST_ID;
  }

  if (id > LAST_ID) {
    return 0;
  }

  return id;
}

void _populateMessage(struct message *msg, uint8_t index) {
  msg->key_id = index;
  msg->text = doc[KEYS[index]]["text"];
  msg->bg = doc[KEYS[index]]["bg"];
  msg->fg = doc[KEYS[index]]["fg"];
  msg->scroll = (doc[KEYS[index]]["scroll"] != NULL ? doc[KEYS[index]]["scroll"] : 0);
}

/*
 Internal method to test prev/next capability
 */
void _serialController() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    if (incomingByte == 'n') {
      nextMessage();
      _dumpMessage(cur);
      _serialize(cur);
    } else if (incomingByte == 'p') {
      prevMessage();
      _dumpMessage(cur);
    } else {
      return;
    }

  }
}

/*
  Retaining method for demo code examples.
  No value to the program.
 */
void _configureMessages() {

      Serial.println("KEYS:");
      for (uint8_t i = 0; i < (sizeof(KEYS)/sizeof(KEYS[0])); i++) {
        if (KEYS[i] == NULL) {
          break;
        }
        const char * key = doc[KEYS[i]]["fg"];
        Serial.println(key);
        key = NULL;
      }

      const char * txt = doc[cur.text]["fg"];
      Serial.print("cur.text: ");
      Serial.println(txt);

      Serial.println();
}

void _serialize(message msg) {
  StaticJsonDocument<192> doc;

  JsonObject jo = doc.createNestedObject(KEYS[msg.key_id]);
  jo["text"] = msg.text;
  jo["bg"] = msg.bg;
  jo["fg"] = msg.fg;
  jo["rate"] = msg.scroll;
  
  serializeJson(doc, Serial);
  Serial.println();
}

void _dumpMessage(message msg) {
  Serial.println(msg.key_id);
  Serial.println(msg.text);
  Serial.println(msg.bg);
  Serial.println(msg.fg);
  Serial.println(msg.scroll);
  Serial.println();

}

void _dumpDoc() {
        // serialize the object and send the result to Serial
      serializeJson(doc, Serial);
      Serial.println();
}
