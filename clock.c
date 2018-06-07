/*************************************************************************
Title:    Testing output to a HD44780 based LCD display.
Author:   Peter Fleury  <pfleury@gmx.ch>  http://tinyurl.com/peterfleury
File:     $Id: test_lcd.c,v 1.8 2015/01/31 18:04:08 peter Exp $
Software: AVR-GCC 4.x
Hardware: HD44780 compatible LCD text display
          AVR with external SRAM interface if memory-mapped LCD interface is used
          any AVR with 7 free I/O pins if 4-bit IO port mode is used
**************************************************************************/
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"

/*
** function prototypes
*/ 
void wait_until_key_pressed(void);
void lcd_update_clock(void);
void lcd_update_date(void);

/*
 * Add hour minute second
 */
volatile unsigned char hours = 20;
volatile unsigned char minutes = 36;
volatile unsigned char seconds = 02;

/*
 * Interrupt service routine
 */
ISR(TIMER1_COMPA_vect);

void wait_until_key_pressed(void)
{
    	unsigned char temp1, temp2;
    
    	do {
        	temp1 = PIND;                  // read input
        	_delay_ms(5);                  // delay for key debounce
        	temp2 = PIND;                  // read input
        	temp1 = (temp1 & temp2);       // debounce input
    	} while ( temp1 & _BV(PIND2) );
    
    	loop_until_bit_is_set(PIND,PIND2);     /* wait until key is released */
}


int main(void)
{
    	DDRD &=~ (1 << PD2);        /* Pin PD2 input              */
    	PORTD |= (1 << PD2);        /* Pin PD2 pull-up enabled    */

        TCCR1B = (1<<CS12|1<<WGM12);
	OCR1A = 15625-1;
	TIMSK = 1<<OCIE1A;
	sei();

    	/* initialize display, cursor off */
    	lcd_init(LCD_DISP_ON);
        lcd_clrscr();

    	for (;;) {  
       	 	/* put string to display (line 1) with linefeed */
    }
}

void lcd_update_date()
{
	lcd_clrscr();
        lcd_puts("  Jun 6, 2018\n");
}

void lcd_update_clock() 
{
    	char buffer[7];

	lcd_gotoxy(4,1);  
        itoa( hours , buffer, 10);
        lcd_puts(buffer);
        lcd_putc(':');
	if(minutes < 10)
		lcd_putc('0');
	itoa(minutes, buffer, 10);
	lcd_puts(buffer);
	lcd_putc(':');
	if(seconds < 10)
		lcd_putc('0');
	itoa(seconds, buffer, 10);	
	lcd_puts(buffer);
}

ISR(TIMER1_COMPA_vect)
{
        seconds++;

        if(seconds >= 60)
        {
                seconds -= 60;
                minutes++;
        }
        if(minutes >= 60)
        {
                minutes = 60;
                hours++;
        }
        if(hours > 23)
                hours -= 24;
	lcd_update_date();
	lcd_update_clock();
}

