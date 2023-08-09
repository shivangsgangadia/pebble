IMAGE_SERVER_PORT = 8090
IMAGE_SERVER_TIMEOUT = 5
COMMAND_SERVER_PORT = 8080

DISPLAY_WIDTH = 1200
DISPLAY_HEIGHT = 800

ORIGIN = (DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2)

SENSITIVITY_X = 30
SENSITIVITY_Y = 30

# Byte 0
TRANSLATE_FORWARD =     0b00000010
TRANSLATE_BACKWARD =    0b00000001
TRANSLATE_NOT =         0b00000000
TRANSLATE_WITH_LEFT =   0b00001000
TRANSLATE_WITH_RIGHT =  0b00000100
TRANSLATE_STRAIGHT =    0b00000000
TURN_IN_PLACE_LEFT =    0b00100000
TURN_IN_PLACE_RIGHT =   0b00010000
TURN_IN_PLACE_DONT =    0b00000000
CAMERA_TURN_RIGHT  =    0b01000000
CAMERA_TURN_LEFT   =    0b10000000
# Byte 1
OPEN_PEBBLE	   =	        0b01000000
CLOSE_PEBBLE   =	        0b10000000
MOVE_PEBBLE	   =            0b11000000
INCREMENT_STRIDE_LENGTH =   0b00000001
DECREMENT_STRIDE_LENGTH =   0b00000010
INCREMENT_STRIDE_HEIGHT =   0b00000100
DECREMENT_STRIDE_HEIGHT =   0b00001000
INCREMENT_INCLINE = 0b00010000
DECREMENT_INCLINE = 0b00100000

Y_AXIS_FORWARD = 1
Y_AXIS_STOP = 2
Y_AXIS_BACKWARD = 3
Y_AXIS_STATE = Y_AXIS_STOP

X_AXIS_LEFT = 1
X_AXIS_STOP = 2
X_AXIS_RIGHT = 3
X_AXIS_STATE = X_AXIS_STOP

ROTATION_CLOCKWISE = 1
ROTATION_ANTICLOCKWISE = 2
ROTATION_STOP = 3
ROTATION_STATE = 1

FONT_SIZE = 15
FONT_SIZE_BUTTON = 15
PADDING_BUTTON = 10

TEXT_DISPLAY_COLOR = (100, 100, 0)
TEXT_DISPLAY_COLOR_BUTTON_1 = (255, 255, 255)
TEXT_DISPLAY_COLOR_BUTTON_2 = (100, 100, 0)
BG_COLOR_BUTTON_1 = (100, 0, 0)
BG_COLOR_BUTTON_2 = (0, 100, 0)
IP_TEXT_DISPLAY_LOCATION = (DISPLAY_WIDTH * (1/3) - 300, 20)
CYCLE_TIME_DISPLAY_LOCATION = (DISPLAY_WIDTH * (2/3) - 300, 20)
PACKET_LOSS_DISPLAY_LOCATION = (DISPLAY_WIDTH * (3/3) - 300, 20)
IMAGE_SIZE = 500
IMAGE_DISPLAY_LOCATION = ((DISPLAY_WIDTH / 2) - (IMAGE_SIZE / 2) - 100, 60)


UI_ELEMENTS_COUNT = 4
PACKET_LOSS_DISPLAY_INDEX = 0
IP_TEXT_DISPLAY_INDEX = 1
CYCLE_TIME_DISPLAY_INDEX = 2
IMAGE_DISPLAY_INDEX = 3

BUTTON_COUNT = 9
MOVE_BUTTON_INDEX = 0
OPEN_BUTTON_INDEX = 1
CLOSE_BUTTON_INDEX = 2
INCREMENT_STRIDE_LENGTH_INDEX = 3
DECREMENT_STRIDE_LENGTH_INDEX = 4
INCREMENT_STRIDE_HEIGHT_INDEX = 5
DECREMENT_STRIDE_HEIGHT_INDEX = 6
INCREMENT_INCLINE_INDEX = 7
DECREMENT_INCLINE_INDEX = 8

OPEN_BUTTON_LOCATION = (DISPLAY_WIDTH - 200, (DISPLAY_HEIGHT / 2) - 50)
MOVE_BUTTON_LOCATION = (DISPLAY_WIDTH - 200, (DISPLAY_HEIGHT / 2))
CLOSE_BUTTON_LOCATION = (DISPLAY_WIDTH - 200, (DISPLAY_HEIGHT / 2) + 50)
INCREMENT_BUTTONS_LOCATION_X = CLOSE_BUTTON_LOCATION[0] - 100
INCREMENT_BUTTONS_LOCATION_Y = CLOSE_BUTTON_LOCATION[1] + 50