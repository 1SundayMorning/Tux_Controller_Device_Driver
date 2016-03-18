/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

unsigned char LED_BUFFER[6];
unsigned char PACKET_REC[3];
unsigned char opcode[5];
int ack_flag =1;
unsigned long  button_argument[1];
//table of SSD bytes. defined in header
unsigned char segments[17] = {
	SEGMENT0, SEGMENT1, SEGMENT2,
	SEGMENT3, SEGMENT4, SEGMENT5,
	SEGMENT6, SEGMENT7, SEGMENT8,
	SEGMENT9, SEGMENTA, SEGMENTB,
	SEGMENTC, SEGMENTD, SEGMENTE,
	SEGMENTF, SEGMENTOFF
	};

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
	//MAILBOX!!
    //unsigned a, b, c;
    PACKET_REC[0] = packet[0]; /* Avoid printk() sign extending the 8-bit */
    PACKET_REC[1] = packet[1]; /* values when printing them. */
    PACKET_REC[2] = packet[2];

    if(PACKET_REC[0] == MTCP_ACK)
    {
    	ack_flag = 1;
    }
    if(PACKET_REC[0] == MTCP_BIOC_EVENT)
    {
    	tux_buttons(button_argument[0]);
    }
    if(PACKET_REC[0] == MTCP_RESET)
    {
    	opcode[0] = MTCP_BIOC_ON;
		tuxctl_ldisc_put(tty, opcode, 1);
   		tuxctl_ldisc_put(tty, LED_BUFFER, 6);
    }

    //printk("packet : %x %x %x\n", PACKET_REC[0], PACKET_REC[1], PACKET_REC[2]);
}

/* 
 * tux_build_led_buffer
 *   DESCRIPTION: build 6-byte lED buffer to send to TUX ctrl to display LEDs with inputted values
 *
 *   INPUTS: LED_BUFFER: global LED Buffer to be filled with correct vals
 *			 arg: Arg passed in with info of led vals, led on/off spec, and decimal spec
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: changes LED buffer global variable.
 */
void tux_build_led_buffer(unsigned char * LED_BUFFER, unsigned long arg)
{
	unsigned char value0 = ((arg>>VALUE_1_SHIFT) & LOWER_FOUR_MASK);//get first LED value
	unsigned char value1 = ((arg>>VALUE_2_SHIFT) & LOWER_FOUR_MASK);//get second LED value
	unsigned char value2 = ((arg>>VALUE_3_SHIFT) & LOWER_FOUR_MASK);//get third LED vlaue
	unsigned char value3 = ((arg>>VALUE_4_SHIFT) & LOWER_FOUR_MASK);//get fourth LED value
	unsigned char ledspec = ((arg>>LEDSPEC_SHIFT) & LOWER_FOUR_MASK);//get led clear specifications
	unsigned char decimal = ((arg>>DECIMAL_SHIFT) & LOWER_FOUR_MASK);//get decimal specifications

	LED_BUFFER[0] = MTCP_LED_SET;//op-code to set LEDs
	LED_BUFFER[1] = LOWER_FOUR_MASK;//send all LEDs on. This is so our buffer is always fixed size

	LED_BUFFER[2] = segments[(int)value0];//send value as ssd specification, defined in table
	LED_BUFFER[3] = segments[(int)value1];
	LED_BUFFER[4] = segments[(int)value2];
	LED_BUFFER[5] = segments[(int)value3];

	//routine for setting turning on decimal LED where specified
	if((decimal & BIT_ZERO_MASK))
	{
		LED_BUFFER[2] = LED_BUFFER[2] | DECIMAL_MASK;
	}
	if((decimal & BIT_ONE_MASK))
	{
		LED_BUFFER[3] = LED_BUFFER[3] | DECIMAL_MASK;
	}
	if((decimal & BIT_TWO_MASK))
	{
		LED_BUFFER[4] = LED_BUFFER[4] | DECIMAL_MASK;
	}
	if((decimal & BIT_THREE_MASK))
	{
		LED_BUFFER[5] = LED_BUFFER[5] | DECIMAL_MASK;
	}

	//routine for clearing LED where specified.
	if((~(ledspec) & BIT_ZERO_MASK) > 0)
	{
		LED_BUFFER[2] = SEGMENTOFF;//specially defined character to turn LEDs off
	}
	if((~(ledspec) & BIT_ONE_MASK) > 0)
	{
		LED_BUFFER[3] = SEGMENTOFF;
	}
	if ((~(ledspec) & BIT_TWO_MASK) > 0)
	{
		LED_BUFFER[4] = SEGMENTOFF;
	}
	if((~(ledspec) & BIT_THREE_MASK) > 0)
	{
		LED_BUFFER[5] = SEGMENTOFF;
	}

}

/* 
 * tux_buttons
 *   DESCRIPTION: take button buffer from tux, change to correct output format
 *
 *   INPUTS: arg: Arg passed to be written over with correct values
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: button_argument global variable changed to output format. Used in tuxctl case statement
 */
void tux_buttons(unsigned long arg)
{
	unsigned char other_buttons;//buffer for c,b,a,start
	unsigned char directional_buttons;//buffer for u d l r
	unsigned char temp1;//temps for swap
	unsigned char temp2;

	arg = arg & LOWER_EIGHT_CLEAR_MASK;
	other_buttons = (LOWER_FOUR_MASK & PACKET_REC[1]);//get c,b,a,start
	arg = (arg | other_buttons);
	directional_buttons = (LOWER_FOUR_MASK & PACKET_REC[2]);//get u d l r
	//routine to swap down and left
	temp1 = (directional_buttons & BIT_TWO_MASK);
	temp2 = (directional_buttons & BIT_ONE_MASK);
	temp1 = (temp1 >> 1);//right shift 1
	temp2 = (temp2 << 1);//left shift 1
	directional_buttons = (directional_buttons & MIDDLE_TWO_BITS_MASK);//0x09 clears middle bits. (1001)
	directional_buttons = (directional_buttons | temp1);//add down button
	directional_buttons = (directional_buttons | temp2);//add left button
	directional_buttons = (directional_buttons << 4);//shift to high 4 bits

	button_argument[0] = (arg | directional_buttons);//finish args
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		//printk("here");
		opcode[0] = MTCP_BIOC_ON;
		button_argument[0] = 0x00;
		if(tuxctl_ldisc_put(tty, opcode, 1) != 0)//interrupt-driven button input
		{
			return -EINVAL;//catches failed execution
		}
		opcode[0] = MTCP_LED_USR;
		if(tuxctl_ldisc_put(tty, opcode, 1) != 0)//sets LEDs to user mode
		{
			return -EINVAL;//catches failed execution
		}
		return 0;

	case TUX_BUTTONS:

		if(arg == 0)//check arg is value input
		{
			return -EINVAL;
		}
		else
		{
			if(copy_to_user((unsigned long *)arg, button_argument, 4) != 0)//4 specifies four bytes, send button arg to user.
			{
				return -EINVAL;//catches failed execution
			}
			else
			{
				return 0;
			}
		}
		
		return 0;
	case TUX_SET_LED:
		
		if(ack_flag == 1)//check that we recieved acknoledgement that TUX recieved LED values
		{
			tux_build_led_buffer(LED_BUFFER, arg);//function to build LED buffer from passed argument
			ack_flag = 0;
			if(tuxctl_ldisc_put(tty, LED_BUFFER, 6) != 0)//6 specifies six bytes
			{
				return -EINVAL;//catch error
			}
		}
		
		return 0;

	case TUX_LED_ACK://n/a
	case TUX_LED_REQUEST://n/a
	case TUX_READ_LED://n/a
	default:
	    return -EINVAL;
    }

}

