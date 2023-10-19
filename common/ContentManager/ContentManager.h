#ifndef ContentManager_h
#define ContentManager_h

#include <Arduino.h>

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <StreamUtils.h>
#include <ArduinoJson.h>

class ContentManager {
	public:
		static const uint8_t BILLBOARD_MAX_KEYS = 10;
		const char * FILENAME = "content.json";
		// Calculate approximate doc size https://arduinojson.org/v6/assistant
		StaticJsonDocument<768> doc;

		struct message {
			uint8_t key_id = 0;
			const char * text = NULL;
			const char * fg = NULL;
			const char * bg = NULL;
			double scroll = 0;
		} cur, prev, next;

		void begin();
		void nextMessage();
		void prevMessage();
		void dumpMessage(struct message msg);
		void dumpDoc();

	private:
		const char * KEYS[BILLBOARD_MAX_KEYS] = {NULL};
		uint8_t LAST_ID = 0;

		void _loadContentJSON();
		void _updateMessages(int8_t msg_id);
		int8_t _boundId(int8_t id);
		void _populateMessage(struct message *msg, uint8_t index);
};

#endif