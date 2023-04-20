#ifndef _COMMANDS_H
#define _COMMANDS_H

// Byte 0
#define TRANSLATE_FORWARD      0b00000010
#define TRANSLATE_BACKWARD     0b00000001
#define TRANSLATE_NOT          0b00000000
#define TRANSLATE_WITH_LEFT    0b00001000
#define TRANSLATE_WITH_RIGHT   0b00000100
#define TRANSLATE_STRAIGHT     0b00000000
#define TURN_IN_PLACE_LEFT     0b00100000
#define TURN_IN_PLACE_RIGHT    0b00010000
#define TURN_IN_PLACE_DONT     0b00000000
#define CAMERA_TURN_RIGHT      0b01000000
#define CAMERA_TURN_LEFT       0b10000000
// Byte 1
#define OPEN_PEBBLE               0b01000000
#define CLOSE_PEBBLE              0b10000000
#define MOVE_PEBBLE               0b11000000
#define INCREMENT_STRIDE_LENGTH   0b00000001
#define DECREMENT_STRIDE_LENGTH   0b00000010
#define INCREMENT_STRIDE_HEIGHT   0b00000100
#define DECREMENT_STRIDE_HEIGHT   0b00001000
#define INCREMENT_INCLINE         0b00010000
#define DECREMENT_INCLINE         0b00100000

#endif
