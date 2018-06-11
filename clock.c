/*************************************************************************
Title:    Clock routine
Author:   Chris Gorman <chrisjohgorman@gmail.com>
File:     clock.c
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
#include <avr/sleep.h>
#include "lcd.h"

/*
** function prototypes
*/ 
void wait_until_key_pressed(void);
void lcd_update_clock(void);
void lcd_update_date(void);
void lcd_update_second(void);
void lcd_update_minute(void);
void lcd_update_hour(void);
void lcd_update_day(void);
void lcd_update_weekday(void);
void lcd_update_month(void);
void lcd_update_year(void);
char spring_savings(void);
char fall_savings(void);
char day_of_week(int, char, char);
char day_of_month(int, char, char, char);
char leap_year(int);

/*
 * Add hour minute second
 */
volatile unsigned int year = 2018;
volatile unsigned char month = 6;
volatile unsigned char day = 11;
volatile unsigned char hour = 15;
volatile unsigned char minute = 13;
volatile unsigned char second = 00;
volatile unsigned char lastdom;
volatile unsigned char daylight_time = 0;

const char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec" };
const char daytab[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};  

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

       	TCCR1B |= (1 << WGM12);
	TIMSK |= (1 << OCIE1A);
	sei();
	//OCR1A = 32768 - 1; /* set CTC compare value to 1Hz at 32.768KHz 
	//			with a prescaler of 1 (hex 0x7FFF) */
	OCR1A = 16525-1;
	//TCCR1B |= (1 << CS10) /* set to clk_io/1 (No prescaling) */
       	TCCR1B |= ((1 << CS10) | (1 << CS11)); 

    	/* initialize display, cursor off */
    	lcd_init(LCD_DISP_ON);
        lcd_clrscr();
	
	//set_sleep_mode(SLEEP_MODE_IDLE);
    	for (;;) {  
		//sleep_mode();
       	 	/* put string to display (line 1) with linefeed */
    }
}

void lcd_update_date()
{	
	lcd_update_weekday();
	lcd_putc(' ');
	lcd_update_month();
	lcd_putc(' ');
	lcd_update_day();
	lcd_putc(',');
	lcd_putc(' ');
	lcd_update_year();
	lcd_putc('\n');
}

void lcd_update_month() 
{
	lcd_gotoxy(4,0);
	lcd_puts(months[month -1]);
}

void lcd_update_day() 
{
	char buffer[7];
	lcd_gotoxy(8,0);
	itoa(day,buffer, 10);
	lcd_puts(buffer);
}

void lcd_update_year() 
{
	char buffer[7];
	lcd_gotoxy(12,0);
	itoa(year,buffer, 10);
	lcd_puts(buffer);
}

void lcd_update_weekday()
{
	unsigned char weekday;
	weekday = day_of_week(year, month, day);
	lcd_gotoxy(0,0);
	lcd_puts(weekdays[weekday]);
}

void lcd_update_hour()
{
	char buffer[7];
	lcd_gotoxy(4,1);
        itoa(hour/10, buffer, 10);
        lcd_puts(buffer);
        itoa(hour%10 , buffer, 10);
        lcd_puts(buffer);
}

void lcd_update_minute()
{
	char buffer[7];
	lcd_gotoxy(7,1);
	itoa(minute/10, buffer, 10);
	lcd_puts(buffer);
	itoa(minute%10, buffer, 10);
	lcd_puts(buffer);
}

void lcd_update_second()
{
	char buffer[7];
	lcd_gotoxy(10,1);
	itoa(second/10, buffer, 10);	
	lcd_puts(buffer);
	itoa(second%10, buffer, 10);	 
	lcd_puts(buffer);
}

char day_of_week(int year, char month, char day)
{
       static char table[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
       year -= month < 3;
       return (year + year/4 - year/100 + year/400 + table[month-1] + day) % 7;
}

/* The following function used to be called NthDate and was authored by
Mike Szczys.  It can be found at GitHub and has the following License.

https://github.com/szczys/Automatic-Daylight-Savings-Time

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute 
this software, either in source code form or as a compiled binary, for any 
purpose, commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of 
this software dedicate any and all copyright interest in the software to the 
public domain. We make this dedication for the benefit of the public at large 
and to the detriment of our heirs and successors. We intend this dedication 
to be an overt act of relinquishment in perpetuity of all present and future 
rights to this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to https://unlicense.org  */

char day_of_month(int year, char month, char dow, char week){
  	char target_date = 1;
  	char first_dow = day_of_week(year, month, target_date);
  	while (first_dow != dow){
    		first_dow = (first_dow+1)%7;
    		target_date++;
  	}

  	target_date += (week-1)*7;
  	return target_date;
}

char fall_savings(void) {
	/* return the day of the first sunday in november */
	return (day_of_month(year,month,day_of_week(year,month,day), 0));
}

char spring_savings(void) {
	/* return the day of the second sunday in march */
	return (day_of_month(year,month,day_of_week(year,month,day), 1));
}

char leap_year(int year) {
	return (((year%4 == 0 && year%100 != 0) || year%400 ==0));
}
/*
 * https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
 */

void lcd_update_clock() 
{
	lcd_update_hour();
        lcd_putc(':');
	lcd_update_minute();
	lcd_putc(':');
	lcd_update_second();
}

ISR(TIMER1_COMPA_vect)
{
        second++;

        if(second > 59)
        {
                second = 0;
                minute++;
                if(minute > 59)
                {
                        minute = 0;
			/*
			 * first sunday in november decrement hour at 2 am
			 * second sunday in march increment hour at 2 am
			 * (twice)
			 */
			if((daylight_time == 0) && (month == 3) && 
					(hour == 1) && 
					(day == spring_savings())) {
				hour++;
				daylight_time = 1;
			} else if((daylight_time == 0) && (month == 11) && 
					(hour == 1) && 
					(day == fall_savings())) {
				hour--;
				daylight_time = 1;
			}
                        hour++;
                        if(hour > 23) {
                                hour = 0;
                                day++;
                                if(!leap_year(year))
                                        lastdom = daytab[0][month -1];
                                else
                                        lastdom = daytab[1][month -1];
                                if(day > lastdom) {
                                        day = 1;
                                        month++;
                                        if (month > 12) {
                                                month = 1;
                                                year++;
                                        }
                                }
                        }
                }
        }
	lcd_update_date();
	lcd_update_clock();
}

