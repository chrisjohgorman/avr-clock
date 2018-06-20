// Title:    Clock 
// Author:   Chris Gorman <chrisjohgorman@gmail.com>
// File:     clock.c
//

#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "lcd.h"
#include "clock.h"


//
// Add hour minute second
//
volatile unsigned int year = 2018;
volatile unsigned char month = 6;
volatile unsigned char day = 11;
volatile unsigned char hour = 15;
volatile unsigned char minute = 13;
volatile unsigned char second = 0;
volatile unsigned char lastdom;
volatile unsigned char daylight_time = 0;
volatile unsigned char set_time = 0;

const char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec" };
const char daytab[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};  

//
// Interrupt service routine
//

ISR(TIMER1_COMPA_vect);

int main(void)
{
	DDRD &=~ (1 << PD0) | (1 << PD1) | (1 << PD2); // PD0, PD1, PD2 input
	PORTD = (1 << PD0) | (1 << PD1) | (1 << PD2); // pullup resistors 

	TCCR1B = (1 << CS12) | (1 << WGM12);
	TIMSK = (1 << OCIE1A);
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	sei();
	//OCR1A = 32768 - 1; // set CTC compare value to 1Hz at 32.768KHz 
        		     //	with a prescaler of 1 (hex 0x7FFF) 
	OCR1A = 16525-1;
	//TCCR1B |= (1 << CS10); // set to clk_io/1 (No prescaling) 
	//TCCR1B |= ((1 << CS10) | (1 << CS11));

	// initialize display, cursor off
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();

	for (;;)
	{
		switch (set_time % 7)
		{
		case 0:
			set_year();
			// if button press up
			// 	year++
			//	if year == 10000
			// 		year = 0
			if(bit_is_clear(PIND, PD3))
			{
				year++;
				if (year == 10000)
					year = 0;
			}
			// if button press down
			//  	year--
			//	if year == -1
			// 		year == 9999
			if(bit_is_clear(PIND, PD4))
			{
				year--;
				if (year == -1)
					year = 9999;
			}
			break;
		case 1:
			set_month();
			// if button press up
			// 	month++
			//	if month == 13
			//		month = 1
			if(bit_is_clear(PIND, PD3))
			{
				month++;
				if (month == 13)
					month = 1;
			}
			// if button press down
			//	month--
			//	if month == 0
			//		month = 12
			if(bit_is_clear(PIND, PD4))
			{
				month--;
				if (month == 0)
					month = 12;
			}
			break;
		case 2:
			set_day();
			// FIXME lastdom is not set until the month is set once
			// so call it here in case someone wants to go
			// backwards through the months, otherwise we get 
			// month = 255, 254, etc.
			//
			// if button press up
			// 	day++
			// 	if day > lastdom
			// 		day = 1
			if(!leap_year(year))
				lastdom = daytab[0][month -1];
			else
				lastdom = daytab[1][month -1];
			if(bit_is_clear(PIND, PD3))
			{
				day++;
				if (day > lastdom)
					day = 1;
			}
			// if button press down 
			// 	day--
			// 	if day < 1
			// 		day = lastdom
			if(bit_is_clear(PIND, PD4))
			{
				day--;
				if (day < 1)
					day = lastdom;
			}
			break;
		case 3:
			set_hour();
			// if button press up 
			// 	hour++
			// 	if hour == 24
			// 		hour = 0
			if(bit_is_clear(PIND, PD3))
			{
				hour++;
				if (hour > 23)
					hour = 0;
			}
			// if button press down 
			// 	hour--
			// 	if hour == -1
			// 		hour = 23
			if(bit_is_clear(PIND, PD4))
			{
				hour--;
				if (hour < 0)
					hour = 23;
			}
			break;
		case 4:
			// if button press up 
			// 	minute++
			// 	if minute == 60
			// 		minute = 1
			set_minute();
			if(bit_is_clear(PIND, PD3))
			{
				minute++;
				if (minute == 60)
					minute = 1;
			}
			// if button press down 
			// 	minute--
			// 	if minute == -1
			//		minute = 59
			if(bit_is_clear(PIND, PD4))
			{
				minute--;
				if (minute < 0)
					minute = 59;
			}
			break;
		case 5:
			set_second();
			// if button press up second = 0;
			if(bit_is_clear(PIND, PD3))
			{
				second = 0;
			}
			// if button press down second = 0;
			if(bit_is_clear(PIND, PD4))
			{
				second = 0;
			}
			break;
		case 6:
			//sleep_mode();
			break;
		}
	}
}

static void inline lcd_display_date_attribute(uint8_t attribute, 
		uint8_t position, uint8_t line)
{
	char buffer[5];
	lcd_gotoxy(position, line);
	itoa(attribute, buffer, 10);
	lcd_puts(buffer);
}

static void inline lcd_display_time_attribute(uint8_t attribute, 
		uint8_t position, uint8_t line)
{
	char buffer[2];
	lcd_gotoxy(position, line);
	itoa(attribute/10, buffer, 10);
	lcd_puts(buffer);
	itoa(attribute%10, buffer, 10);
	lcd_puts(buffer);
}

void set_year()
{
	lcd_gotoxy(12,0);
	lcd_puts("    ");
	_delay_ms(500);
	if (day < 10)
		lcd_display_date_attribute(year, 11, 0);
	else
		lcd_display_date_attribute(year, 12, 0);
	_delay_ms(500);
}

void set_month()
{
	lcd_gotoxy(4,0);
	lcd_puts("   ");
	_delay_ms(500);
	lcd_display_date_attribute((uint16_t) months[month -1], 12, 0);
	_delay_ms(500);
}

void set_day()
{
	lcd_gotoxy(8,0);
	lcd_puts("  ");
	_delay_ms(500);
	lcd_display_date_attribute(day, 12, 0);
	_delay_ms(500);
}

void set_hour()
{
	lcd_gotoxy(4,1);
	lcd_puts("  ");
	_delay_ms(500);
	lcd_display_date_attribute(hour, 12, 0);
	_delay_ms(500);
}

void set_minute()
{
	lcd_gotoxy(7,1);
	lcd_puts("  ");
	_delay_ms(500);
	lcd_display_date_attribute(minute, 12, 0);
	_delay_ms(500);
}

void set_second()
{
	lcd_gotoxy(10,1);
	lcd_puts("  ");
	_delay_ms(500);
	lcd_display_date_attribute(second, 12, 0);
	_delay_ms(500);
}

//void lcd_display_month() 
//{
//	lcd_gotoxy(4,0);
//	lcd_puts(months[month -1]);
//}
//
//void lcd_display_day() 
//{
//	char buffer[7];
//	lcd_gotoxy(8,0);
//	itoa(day,buffer, 10);
//	lcd_puts(buffer);
//}
//
//void lcd_display_year() 
//{
//	char buffer[7];
//	lcd_gotoxy(12,0);
//	itoa(year,buffer, 10);
//	lcd_puts(buffer);
//}
//
//void lcd_display_weekday()
//{
//	unsigned char weekday;
//	weekday = day_of_week(year, month, day);
//	lcd_gotoxy(0,0);
//	lcd_puts(weekdays[weekday]);
//}
//
//void lcd_display_hour()
//{
//	char buffer[7];
//	lcd_gotoxy(4,1);
//	itoa(hour/10, buffer, 10);
//	lcd_puts(buffer);
//	itoa(hour%10 , buffer, 10);
//	lcd_puts(buffer);
//}
//
//void lcd_display_minute()
//{
//	char buffer[7];
//	lcd_gotoxy(7,1);
//	itoa(minute/10, buffer, 10);
//	lcd_puts(buffer);
//	itoa(minute%10, buffer, 10);
//	lcd_puts(buffer);
//}
//
//void lcd_display_second()
//{
//	char buffer[7];
//	lcd_gotoxy(10,1);
//	itoa(second/10, buffer, 10);	
//	lcd_puts(buffer);
//	itoa(second%10, buffer, 10);	 
//	lcd_puts(buffer);
//}
//
char day_of_week(int year, char month, char day)
{
	static char table[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	year -= month < 3;
	return (year + year/4 - year/100 + year/400 + table[month-1] + day) % 7;
}

// The following function used to be called NthDate and was authored by
// Mike Szczys.  It can be found at GitHub and has the following License.
//
// https://github.com/szczys/Automatic-Daylight-Savings-Time
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute 
// this software, either in source code form or as a compiled binary, for any 
// purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of 
// this software dedicate any and all copyright interest in the software to the 
// public domain. We make this dedication for the benefit of the public at large
// and to the detriment of our heirs and successors. We intend this dedication 
// to be an overt act of relinquishment in perpetuity of all present and future 
// rights to this software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For more information, please refer to https://unlicense.org 

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

// return the day of the first Sunday in November
char fall_savings(void) {
	return (day_of_month(year,month,day_of_week(year,month,day), 0));
}

// return the day of the second Sunday in March
char spring_savings(void) {
	return (day_of_month(year,month,day_of_week(year,month,day), 1));
}

// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
char leap_year(int year) {
	return (((year%4 == 0 && year%100 != 0) || year%400 ==0));
}

void lcd_display_clock() 
{
	lcd_display_date_attribute((uint16_t) day_of_week(year,month,day),
			0, 0);
	//lcd_display_weekday();
	lcd_putc(' ');
	lcd_display_date_attribute((uint16_t) months[month -1], 4, 0);
	//lcd_display_month();
	lcd_putc(' ');
	lcd_display_date_attribute(day, 8, 0);
	//lcd_display_day();
	lcd_putc(',');
	lcd_putc(' ');
	//lcd_display_year();
	if (day < 10)
		lcd_display_date_attribute(year, 11, 0);
	else
		lcd_display_date_attribute(year, 12, 0);
	lcd_putc('\n');
	//lcd_display_hour();
	//lcd_display_time_attribute(hour, 4, 1);
	lcd_putc(':');
	//lcd_display_time_attribute(minute, 7, 1);
	//lcd_display_minute();
	lcd_putc(':');
	//lcd_display_time_attribute(second, 10, 1);
	//lcd_display_second();
	lcd_putc('\n');
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
			// FIXME - should be hour == 2, emperical tests
			// show that it hour == 1 works?
			//
			// first Sunday in November decrement hour at 2 am
			// second Sunday in March increment hour at 2 am
			// (twice)
			//
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
	lcd_display_clock();
}
