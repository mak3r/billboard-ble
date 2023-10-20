/*********************************************************************
 Read JSON doc from eeprom using littlfs
 Enable cycling through billboard messages in the JSON doc

 Assumptions (dependencies):
 1) there will be a file called content.json in littlfs
 2) content.json contains a list of billboard messages

 Apache 2 license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <ContentManager.h>

using namespace Adafruit_LittleFS_Namespace;

void ContentManager::begin() {
    // Initialize Internal File System
  InternalFS.begin();

  // Load content into JSON document
  _loadContentJSON();

  // configureMessages();
  _updateMessages(0);
}

void ContentManager::nextMessage() {
  _updateMessages(cur.key_id + 1);
}

void ContentManager::prevMessage() {
  _updateMessages(cur.key_id - 1);
}

/*
 Assumption: LittleFS contains a file named content.json
*/
void ContentManager::_loadContentJSON()
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
      ReadBufferingStream bufferingStream(item, 64);

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
void ContentManager::_updateMessages(int8_t msg_id) {
  int8_t cur_id = _boundId(msg_id);
  int8_t next_id = _boundId(cur_id + 1);
  int8_t prev_id = _boundId(cur_id - 1);

  _populateMessage(cur, cur_id);
  _populateMessage(next, next_id);
  _populateMessage(prev, prev_id);

}

int8_t ContentManager::_boundId(int8_t id) {
  if (id < 0) {
    return LAST_ID;
  }

  if (id > LAST_ID) {
    return 0;
  }

  return id;
}

void ContentManager::_populateMessage(struct ContentManager::message &msg, uint8_t index) {
  msg.key_id = index;
  msg.text = doc[KEYS[index]]["text"];
  msg.bg = doc[KEYS[index]]["bg"];
  msg.fg = doc[KEYS[index]]["fg"];
  msg.scroll = (doc[KEYS[index]]["scroll"] != NULL ? doc[KEYS[index]]["scroll"] : 0);
}

void ContentManager::dumpMessage(struct ContentManager::message &msg) {
  StaticJsonDocument<192> doc;

  JsonObject jo = doc.createNestedObject(KEYS[msg.key_id]);
  jo["text"] = msg.text;
  jo["bg"] = msg.bg;
  jo["fg"] = msg.fg;
  jo["rate"] = msg.scroll;
  
  serializeJson(doc, Serial);
  Serial.println();
}

void ContentManager::dumpDoc() {
        // serialize the object and send the result to Serial
      serializeJson(doc, Serial);
      Serial.println();
}
