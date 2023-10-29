import time
import displayio
import terminalio
import gc
import json

from adafruit_ble import BLERadio
from adafruit_ble import Advertisement
from adafruit_ble.services.nordic import UARTService
from adafruit_clue import clue
from adafruit_display_text import label
from adafruit_display_text import scrolling_label
from adafruit_lsm6ds.lsm6ds33 import LSM6DS33

DEBUG = False
MANUAL_CONNECT = False
# for binding with the matrix portal billboard
BILLBOARD_NAME = "F-nRF52"


# Display Stuff
display = clue.display
disp_group = displayio.Group()
display.show(disp_group)

# ceiling of (clue display dpi / matrix display dpi) [display widths]
LABEL_SCALE = -(-252//64)
IS_SCROLLING = False

DISCONNECTED = "  BILLBOARD\nDISCONNECTED"
# Lower left corner messages for debugging
debug_label = label.Label(terminalio.FONT, text=DISCONNECTED, scale=2, color=0x00FF00)
debug_label.anchor_point = (0,0)
debug_label.anchored_position = (40, 170)
debug_group = displayio.Group()
debug_group.append(debug_label)
disp_group.append(debug_group)
debug_group.hidden = False

# Static content group
static_label = label.Label(terminalio.FONT, text='A'*32, scale=2,
                       color=0xFFFFFF)
static_label.anchor_point = (0, 0)
static_label.anchored_position = (5, 12)
static_group = displayio.Group()
static_group.append(static_label)
disp_group.append(static_group)

# Scrolling content group
scroll_label = scrolling_label.ScrollingLabel(terminalio.FONT, text='A', max_characters = 10)
scroll_label.scale = 2
scroll_label.color = 0xFFFFFF
scroll_label.anchor_point = (0, 0)
scroll_label.anchored_position = (5, 12)
scroll_group = displayio.Group()
scroll_group.append(scroll_label)
disp_group.append(scroll_group)

# This is the bluetooth low energy connection
ble_connection = None
ble = BLERadio()
# print("BLE Radio name:", ble.name)
uart = None
billboard = None
# When we are waiting for a response from sending a prev/next request
await_message = False
# A byte array of json formatted content
response = bytearray('', 'utf-8')


curly_count = 0
def capture_json(data = b''):
    global curly_count
    global await_message
    if DEBUG: print(data)
    for i in data:
        if i == b'{'[0]:
            curly_count += 1
        if i == b'}'[0]:
            curly_count -= 1
    if await_message:
        response.extend(data)
        # print(response)
    if curly_count == 0:
        await_message = False

# Call this method only after the response variable is believed to have 
# valid json formatted content
def parse_data() -> dict:
    parsed = None
    try:
        parsed = json.loads(response)
    except Exception as e:
        if DEBUG: print("failed to parse response:", response)
        return None
    return parsed

def update_label(content=None):
    global IS_SCROLLING

    if content is None:
        content = "Error in\nreturned content\n[A] or [B]\nto continue."
    
    if type(content) is dict:
        try:
            if (len(content.get("text")) > 10 and "\n" not in content.get("text")):
                IS_SCROLLING = True
                config_scrolling(content)
            else:
                IS_SCROLLING = False
                config_static(content)

        except Exception as e:
            c = json.dumps(content)
            err = "Error in dictionary content\n"
            if DEBUG: print(err, c)
            update_label(err + c)
    elif type(content) is str:
        IS_SCROLLING = False
        
        static_label.text = content
        static_label.color = 0xFFFFFF
        static_label.background_color = None
        static_label.scale = 2
    else:
        update_label("ERROR\nCONTENT INVALID\nCHECK BILLBOARD")

    if IS_SCROLLING:
        static_group.hidden = True
        scroll_group.hidden = False
    else:
        scroll_group.hidden = True
        static_group.hidden = False


def config_static(content:dict = None):
    static_label.text = content.get("text")
    static_label.color = int(content.get("fg"),16)
    static_label.background_color = int(content.get("bg"),16)
    static_label.scale = LABEL_SCALE

def config_scrolling(content:dict = None):
    msg = content.get("text")
    # Pad scrolling text with either 1 or 10 spaces
    scroll_label.text = (" "*10) + msg
    scroll_label.color = int(content.get("fg"),16)
    scroll_label.background_color = int(content.get("bg"),16)
    scroll_label.scale = LABEL_SCALE



def clear_connection():
    global uart
    uart = None
    for connection in ble.connections:
        connection.disconnect()

# Scan for advertisements and return the advertisement 
# that matches BILLBOARD_NAME
def scan() -> Advertisement :
    # A completed scan could be from a successful connection
    # or from a scan timeout
    # print("Free memory: %s"%str(gc.mem_free()))
    # print("Allocated memory: %s"%str(gc.mem_alloc()))
    # this will be assigned to the Advertisement for the billboard we want to connect with
    ad = None
    try:
        # Keeping buffer size low seems to reduce the memory amount attempting to be allocated
        #   Default is 512
        for advert in ble.start_scan(buffer_size=128,timeout=2):
            if DEBUG: print(f"{advert=}")
            if advert.complete_name == BILLBOARD_NAME:
                ad = advert
                if MANUAL_CONNECT:
                    update_label("Found {} \n{}".format(ad.complete_name, "[A+B] to connect"))
                break
    except Exception as e:
        if DEBUG: print(e)

    ble.stop_scan()
    gc.collect()
    return ad


def connect(billboard=None):
    global uart
    if billboard:
        try:
            ble.connect(billboard)
            billboard = None
            for connection in ble.connections:
                if not connection.connected:
                    continue
                # print("Check connections for uart service")
                if UARTService not in connection:
                    continue
                # print("Connection has uart service")
                uart = connection[UARTService]
                debug_group.hidden = True
                write_ble(b'c')
                break
        except Exception as e:
            if billboard:
                if MANUAL_CONNECT:
                    update_label("Unable to connect \nto {}.\nPlease rescan[A+B].".format(billboard.complete_name))
            else:
                if MANUAL_CONNECT:
                    update_label("Connection failed.\nTry rescan[A+B].")
            if DEBUG: print(e)

def write_ble(content:bytes = b''):
    global await_message
    global response
    response_delay = 1 # seconds
    response_time = time.monotonic()
    uart.reset_input_buffer()
    uart.write(content)
    try:
        while uart.in_waiting is 0:
            #block until a response is received or the response delay
            # time expires
            if time.monotonic() - response_time > response_delay:
                if DEBUG: print("response delay expired")
                break;
        
        # If there is content returned after the response delay 
        if uart.in_waiting:
            # we need to block until the message is fully received.
            await_message = True
            # Reset the byte array before we begin
            data = b''
            response = bytearray('', 'utf-8')
            while await_message:
                byte_count = uart.in_waiting
                data = uart.read(byte_count)
                if data:
                    capture_json(data) #tests for await_message is True

        msg = parse_data()

        update_label(msg)
    except Exception as e:
        update_label(e)
        print(e)

# Display fade management
display_delay = 10
display_time = time.monotonic()

def reset_display_sleep():
    global display_time
    display_time = time.monotonic()
    display.brightness = 1.0


# Just booting up - set the first instruction
INITIAL_CONNECTION = "No Billboard\nConnected\n[A+B] to scan\nfor billboard"
update_label(INITIAL_CONNECTION)

# Button response management
button_delay = 0.2
button_time = time.monotonic()

reset_display_sleep()
while True:
    if (time.monotonic() - display_time) > display_delay:
        if display.brightness > 0.1:
            display.brightness = display.brightness - 0.1
        else:
            display.brightness = 0
        display_time = time.monotonic()
    
    if clue.proximity > 5:
        display.brightness = 1.0
        display_time = time.monotonic()

    elif ble.connected:
        if (time.monotonic() - button_time) > button_delay:
            if clue.button_a and clue.button_b:
                clue.play_tone(1459, 1)
                clear_connection()
                reset_display_sleep()

            if uart:
                if clue.button_b:
                    debug_label.text = 'B'
                    clue.play_tone(887, .3)
                    clue.play_tone(1024, .3)
                    clue.play_tone(887, .4)
                    write_ble(b'n')
                    reset_display_sleep()

                if clue.button_a:
                    debug_label.text = 'A'
                    clue.play_tone(1024, .3)
                    clue.play_tone(887, .3)
                    clue.play_tone(1024, .4)
                    write_ble(b'p')
                    reset_display_sleep()

            button_time = time.monotonic()
            
        if IS_SCROLLING:
            scroll_label.update()

    else: #BLE not connected
        debug_group.hidden = False
        if MANUAL_CONNECT:
            # Useful for manually handling connections
            if clue.button_a and clue.button_b:
                if not billboard:
                    billboard = scan()
                else:
                    connect(billboard=billboard)
        else:
            # Autoconnect
            scroll_group.hidden = True
            static_group.hidden = True
            debug_label.text = DISCONNECTED
            if not billboard:
                billboard = scan()
            else:
                connect(billboard=billboard)

