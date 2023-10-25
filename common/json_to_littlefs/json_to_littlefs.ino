/*********************************************************************
 Write billboard content to eeprom using littlfs

 Apache 2 license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <Adafruit_TinyUSB.h> // for Serial

using namespace Adafruit_LittleFS_Namespace;

/* Setup json content that will be used by the billboard.
 *
 * WARNING: this program may overwrite the entire contents of eeprom
 * 
 * Future versions may allow invocation of the WAP of the device
 * to allow reading and writing content over a web interface. 
 */

#define FILENAME    "/content.json"
const char* MESSAGES = R"(
{
	"quiet": {"text": "-", "fg": "0xFFFFFF", "bg": "0x000000"},
	"default": {"text": "billboard", "fg": "0xFFFFFF", "bg": "0x000000"},
	"text1": {
		"text" : "howdy",
		"bg": "0x020202",
		"fg": "0x101010"
	},
	"scroll-text": {
		"text" : "scroll this by me",
		"bg": "0x800020",
		"fg": "0xAAAAAA",
		"rate" : ".2"	
	},
	"wrap-example": {
		"text" : "wrap text\nexample",
		"fg" : "0x982200",
		"bg" : "0x000000"
	},
	"in-meeting": {
		"text": "IN A\nMEETING",
		"fg" : "0xFF11BB",
		"bg" : "0x000000"
	}
}
)";

File file(InternalFS);

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb
  
  Serial.println("Create json formatted content for use in an application");

  // Initialize Internal File System
  InternalFS.begin();
  
  // Format 
  InternalFS.format();

  // Print whole directory tree of root whose level is 0
  printTreeDir("/", 0);

  if( file.open(FILENAME, FILE_O_WRITE) )
  {
    Serial.println("content.json:");
    file.write(MESSAGES, strlen(MESSAGES));
    file.close();
  }else
  {
    Serial.println("Failed writing file" FILENAME "to littlfs!");
  }

  // Print prompt
  Serial.println();
  Serial.println("Enter anything to print directory tree:");
}

// the loop function runs over and over again forever
void loop() 
{
  if ( Serial.available() )
  {
    delay(10); // delay for all input arrived
    while( Serial.available() ) Serial.read();

    printTreeDir("/", 0);
    
    // Print prompt
    Serial.println();
    Serial.println("Enter anything to print directory tree:");
  }
}

/**************************************************************************/
/*!
    @brief  Print out whole directory tree of an folder
            until the level reach MAX_LEVEL

    @note   Recursive call
*/
/**************************************************************************/
void printTreeDir(const char* cwd, uint8_t level)
{
  // Open the input folder
  File dir(cwd, FILE_O_READ, InternalFS);

  // Print root
  if (level == 0) Serial.println("root");
 
  // File within folder
  File item(InternalFS);

  // Loop through the directory 
  while( (item = dir.openNextFile(FILE_O_READ)) )
  {
    // Indentation according to dir level
    for(int i=0; i<level; i++) Serial.print("|  ");

    Serial.print("|_ ");
    Serial.print( item.name() );

    if ( item.isDirectory() )
    {
      Serial.println("/root");
    }else
    {
      // Print file size starting from position 30
      int pos = level*3 + 3 + strlen(item.name());

      // Print padding
      for (int i=pos; i<30; i++) Serial.print(' ');

      // Print at least one extra space in case current position > 50
      Serial.print(' ');
      
      Serial.print( item.size() );
      Serial.println( " Bytes");
      while ( item.available() )
      {
        char buffer[64] = { 0 };
        item.read(buffer, sizeof(buffer)-1);

        Serial.print(buffer);
        delay(10);
      }
      Serial.println("");
    }

    item.close();
  }

  dir.close();
}
