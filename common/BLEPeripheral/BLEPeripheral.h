#include <bluefruit.h>

#ifndef BLEPeripheral_h
#define BLEPeripheral_h

class BLEPeripheral {

	public:
		static BLEUart bleuart;
		const char * name = "F-nRF52";
	
		void begin(void);
		void startAdv(void);

	private:

		static void _connect_callback(uint16_t conn_handle);
		static void _disconnect_callback(uint16_t conn_handle, uint8_t reason);
};

#endif