# Billboard on Matrix Portal
This is a simple application which will display a choice of text or images on the Adafruit 32x64 LED matrix driven by a device with an nRF52840 chip.
Currently the board of choice is a feather nRF25840 Express with a featherwing RGB nRF52 Matrix Wing. In addition to the display on the matrix portal, there is 

## Code
`billboard:`
* `display-manager.ino` - the application code which parses billboard content and displays it to the RGB Matrix
* `content.json` 
    - The content which will get displayed on the RGB matrix.
    - See the section [Content Layout](#Content Layout) below for details

`controller:`
* `HID.ino` - code which talks to the RGB Matrix controller

`utility:`
* `json_to_littlefs.ino` - write a blob of json to eeprom via the littlefs file system


## Controller / Billboard Communication
Communication between the billboard controller and the remote control human interface device (HID) will be done using BLE.
The billboard will be configured as a peripheral device and the HID will be the central device. In this way, multiple billboards can be managed by a single remote control.

## Quick Start
### LED Matrix
1. Create a set of billboard messages in json format
1. Write the json formatted content using the `json_to_littlefs.ino`` facility to the RGB Matrix Controller
1. Write the `display_manager.ino` to the RGB Matrix Controller
1. Connect RGB Matrix Controller to the Matrix Display if it's not already connected
1. Power on RGB Matrix Controller - one of the messages should display

### HID Controller
1. Write the HID.ino to the Adafruit Clue
1. Power on
1. Follow prompts to connect to the RGB Matrix Portal Peripheral


## Content Layout
The `content.json` file contains a list of content which can be cycled through on the Matrix Portal. Content could be text or bitmap images. Or a combination of both. Text can be scrolled orizontally and colors can be set for the foreground and background. The content file must be json formatted.

**NOTE:** Each entry in the `content.json` file must have a unique key. The values of the entry are specfied as one fo the 4 content types. This allows for each content type to be repeated in the file in any order. Python reads dictionaries in without regard to order so sequencing according the the file layout is not possible without further development

## Content Generation
* [evaluate size of the StaticJsonDocument](https://arduinojson.org/v6/assistant)

### There are 4 content types:

1. `text` can be a simple word or phrase which is shown statically on the display. `text` has 2 attributes `fg_color` and `bg`. This type can be read from the `content.json` file or from a `url` content type

1. `stext` or scrolling text can be a simple word or phrase which will be scrolled from right to left across the display. `stext` has 3 attributes `fg_color`, `bg` and `rate`. This type can be read from the `content.json` file or from a `url` content type.
<!--
1. `img` is the path to a bitmap image on disk. It is recommended that `img` bitmaps are formatted to the exact size of the display (default 64x32 - WxH). This type can be read from the `content.json` file or from a `url` content type.
"geeko": {"img": "images/geeko.bmp"},
"rancher": {"img": "images/rancher.bmp"},
"file-bg-example": {
  "text" : "GEEKO",
  "fg_color" : "0xFFFFFF",
  "bg" : "images/geeko.bmp"
},

-->


### Attributes:
1. `fg_color` is the foreground color of the text specified as a hex string in RGB - for example 0xFFFFFF is white.
1. `bg` can be either a hex string specified in RGB or a path to a file on disk. 
1. `rate` is the speed at which scrolling text moves from right to left across the screen. Reasonable values appear to be between .04 (fast) and .2 (slow). Any floating point value may be used however the code does not check for 'reasonable' values. 

### Example `content.json`
```
{
	"text1": {
		"text" : "howdy",
		"bg": "0x800020",
		"fg_color": "0x777777"
	},
	"scroll-text": {
		"stext" : "scroll this by me",
		"bg": "0x800020",
		"fg_color": "0xAAAAAA",
		"rate" : ".2"	
	},
	"wrap-example": {
		"text" : "wrap text\nexample",
		"fg_color" : "0x982200",
		"bg" : "0x000000"
	},
"in-meeting": {
		"text": "IN A\nMEETING",
		"fg_color" : "0xFF11BB",
		"bg" : "0x000000"
	}

}

```