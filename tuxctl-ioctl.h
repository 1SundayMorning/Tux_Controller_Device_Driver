// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)

//bytes for encoding characters to ssd byte format.
#define SEGMENT0 	0xE7
#define SEGMENT1 	0x06
#define SEGMENT2 	0xCB
#define SEGMENT3 	0x8F
#define SEGMENT4 	0x2E
#define SEGMENT5 	0xAD
#define SEGMENT6 	0xED
#define SEGMENT7 	0x86
#define SEGMENT8 	0xEF
#define SEGMENT9 	0xAE
#define SEGMENTA 	0xEE
#define SEGMENTB 	0x6D
#define SEGMENTC 	0xE1
#define SEGMENTD 	0x4F
#define SEGMENTE 	0xE9
#define SEGMENTF 	0xE8
#define SEGMENTOFF 	0x00

#define BIT_ZERO_MASK 0x01
#define BIT_ONE_MASK 0X02
#define BIT_TWO_MASK 0x04
#define BIT_THREE_MASK 0x08
#define MIDDLE_TWO_BITS_MASK 0x09
#define LOWER_FOUR_MASK 0x0F
#define DECIMAL_MASK 0x10
#define LOWER_EIGHT_CLEAR_MASK 0xFFFFFF00
#define VALUE_1_SHIFT 0
#define VALUE_2_SHIFT 4
#define VALUE_3_SHIFT 8
#define VALUE_4_SHIFT 12
#define LEDSPEC_SHIFT 16
#define DECIMAL_SHIFT 24

//function to build LED buffer to send to tux contorller and wrrite LEDs
void tux_build_led_buffer(unsigned char * LED_BUFFER, unsigned long arg);
//function to create button press output. Send to user.
void tux_buttons(unsigned long arg);

#endif

