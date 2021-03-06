// Title:    Clock 
// Author:   Chris Gorman <chrisjohgorman@gmail.com>
// File:     clock.c
//

#include <stdlib.h>
#include <inttypes.h>
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "clock.h"
#include "debounce.h"

//avrfreaks.net thread suggestions
//https://www.avrfreaks.net/forum/avr-project-build-clock-program-atmega162?page=1
// 
#define TICS_PER_SECOND 200
#define DEBOUNCE_TIME 1000

static const PROGMEM uint8_t extended_character_table[]  =
{
    0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C,
    0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F,
    0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F
};
//
// Add hour minute second
//
volatile uint16_t year = 2020;
volatile uint8_t month = 1;
volatile uint8_t hour = 0;
volatile uint8_t minute = 0;
volatile uint8_t day = 1;
volatile uint8_t second = 0;
volatile uint8_t lastdom;
volatile uint8_t daylight_time = 1;
volatile uint8_t set_time = 6;
volatile uint8_t nsubticks = TICS_PER_SECOND;
volatile uint8_t i;


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
    buttons_init();
    timer_init();
    debounce_init();
    // set global interrupts
    sei();
    // initialize display, cursor off
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    lcd_command(_BV(LCD_CGRAM));
    for(i=0; i<64; i++)
    {
        lcd_data(pgm_read_byte_near(&extended_character_table[i]));
    }

    for (;;)
    {

        if (button_down(BUTTON0_MASK))
            set_time++;

        switch (set_time % 9) {
        case 0:
            set_year();
            // if button press up
            //  year++
            //  if year == 2020
            //      year = 0
            if (button_down(BUTTON1_MASK)) {
                year++;
                if (year == 2120)
                    year = 2020;
            }
            // if button press down
            //      year--
            //  if year == -1
            //      year == 2020
            if (button_down(BUTTON2_MASK)) {
                year--;
                if (year == -1)
                    year = 2020;
            }
            break;
        case 1:
            set_month();
            // if button press up
            //  month++
            //  if month == 13
            //      month = 1
            if (button_down(BUTTON1_MASK)) {
                month++;
                if (month == 13)
                    month = 1;
            }
            // if button press down
            //  month--
            //  if month == 0
            //      month = 12
            if (button_down(BUTTON2_MASK)) {
                month--;
                if (month == 0)
                    month = 12;
            }
            break;
        case 2:
            set_day();
            // lastdom is not set until the month is set once
            // so call it here in case someone wants to go
            // backwards through the months, otherwise we get
            // month = 255, 254, etc.
            //
            // if button press up
            //  day++
            //  if day > lastdom
            //      day = 1
            if (!leap_year(year))
                lastdom = daytab[0][month - 1];
            else
                lastdom = daytab[1][month - 1];
            if (button_down(BUTTON1_MASK)) {
                day++;
                if (day > lastdom)
                    day = 1;
            }
            // if button press down
            //  day--
            //  if day < 1
            //      day = lastdom
            if (button_down(BUTTON2_MASK)) {
                day--;
                if (day < 1)
                    day = lastdom;
            }
            break;
        case 3:
            set_hour();
            // if button press up
            //  hour++
            //  if hour == 24
            //      hour = 0
            if (button_down(BUTTON1_MASK)) {
                hour++;
                if (hour > 23)
                    hour = 0;
            }
            // if button press down
            //  hour--
            //  if hour == 255
            //      hour = 23
            if (button_down(BUTTON2_MASK)) {
                hour--;
                if (hour == 255)
                    hour = 23;
            }
            break;
        case 4:
            // if button press up
            //  minute++
            //  if minute == 60
            //      minute = 0
            set_minute();
            if (button_down(BUTTON1_MASK)) {
                minute++;
                if (minute == 60)
                    minute = 0;
            }
            // if button press down
            //  minute--
            //  if minute == 255
            //      minute = 59
            if (button_down(BUTTON2_MASK)) {
                minute--;
                if (minute == 255)
                    minute = 59;
            }
            break;
        case 5:
            set_second();
            // if button press up second = 0;
            if (button_down(BUTTON1_MASK)) {
                second = 0;
            }
            // if button press down second = 0;
            if (button_down(BUTTON2_MASK)) {
                second = 0;
            }
            break;
        case 6:
            lcd_display_clock();
            break;
        case 7:
            lcd_display_time_attribute_big(hour, minute);
            break;
        case 8:
            lcd_clrscr();
            break;
        }
    }
}

static void buttons_init()
{
    DDRD &=~ (1 << PD0) | (1 << PD1) | (1 << PD2); // PD0, PD1, PD2 input
    PORTD = (1 << PD0) | (1 << PD1) | (1 << PD2); // pullup resistors
}

static void timer_init()
{
    // set no clock prescaler and CTC mode
    TCCR1B = (1 << CS10) | (1 << WGM12);
    // set output compare A match
    TIMSK = (1 << OCIE1A);
    // output compare register 1
    OCR1A = 20000 -1;
    // timer counter 1
    TCNT1 = 45536;
}


static void lcd_display_time_attribute(uint8_t attribute,
        uint8_t position, uint8_t line)
{
    char buffer[2];
    lcd_gotoxy(position, line);
    itoa(attribute/10, buffer, 10);
    lcd_puts(buffer);
    itoa(attribute%10, buffer, 10);
    lcd_puts(buffer);
}

static void lcd_display_time_attribute_big(uint8_t hour, uint8_t minute)
{
    switch (hour/10) {
        case 0:
            display_0(0);
            break;
        case 1:
            display_1(0); 
            break;
        case 2:
            display_2(0);
            break;
    }

    switch (hour%10) {
        case 0:
            display_0(4);
            break;
        case 1:
            display_1(4); 
            break;
        case 2:
            display_2(4);
            break;
        case 3:
            display_3(4);
            break;
        case 4:
            display_4(4); 
            break;
        case 5:
            display_5(4);
            break;
        case 6:
            display_6(4); 
            break;
        case 7:
            display_7(4);
            break;
        case 8:
            display_8(4);
            break;
        case 9:
            display_9(4); 
            break;
    }

    switch (minute/10) {
        case 0:
            display_0(8);
            break;
        case 1:
            display_1(8); 
            break;
        case 2:
            display_2(8);
            break;
        case 3:
            display_3(8);
            break;
        case 4:
            display_4(8); 
            break;
        case 5:
            display_5(8);
            break;
    }

    switch (minute%10) {
        case 0:
            display_0(12);
            break;
        case 1:
            display_1(12); 
            break;
        case 2:
            display_2(12);
            break;
        case 3:
            display_3(12);
            break;
        case 4:
            display_4(12); 
            break;
        case 5:
            display_5(12);
            break;
        case 6:
            display_6(12); 
            break;
        case 7:
            display_7(12);
            break;
        case 8:
            display_8(12);
            break;
        case 9:
            display_9(12); 
            break;
    }
}

static void set_year()
{
    if (day < 10)
        lcd_gotoxy(11,0);
    else
        lcd_gotoxy(12,0);
    lcd_display_year();
}

static void set_month()
{
    lcd_gotoxy(4,0);
    lcd_display_month();
}

static void set_day()
{
    lcd_gotoxy(8,0);
    lcd_display_day();
}

static void set_hour()
{
    lcd_gotoxy(4,1);
    lcd_display_time_attribute(hour, 4, 1);
}

static void set_minute()
{
    lcd_gotoxy(7,1);
    lcd_display_time_attribute(minute, 7, 1);
}

static void set_second()
{
    lcd_gotoxy(10,1);
    lcd_display_time_attribute(second, 10, 1);
}

static void lcd_display_day()
{
     char buffer[3];
     lcd_gotoxy(8,0);
     itoa(day,buffer, 10);
     lcd_puts(buffer);
}

static void lcd_display_month()
{
    lcd_gotoxy(4,0);
    lcd_puts(months[month -1]);
}

static void lcd_display_year()
{
    char buffer[5];
    if (day < 10)
        lcd_gotoxy(11,0);
    else 
        lcd_gotoxy(12,0);
    itoa(year,buffer, 10);
    lcd_puts(buffer);
    if (day < 10) 
        lcd_putc(' ');
}

static void lcd_display_weekday()
{
    uint8_t weekday;
    weekday = day_of_week(year, month, day);
    lcd_gotoxy(0,0);
    lcd_puts(weekdays[weekday]);
}

static char day_of_week(int year, char month, char day)
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

static char day_of_month(int year, char month, char dow, char week){
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
static char fall_savings(void) {
    return (day_of_month(year,month,day_of_week(year,month,day), 1));
}

// return the day of the second Sunday in March
static char spring_savings(void) {
    return (day_of_month(year,month,day_of_week(year,month,day), 2));
}

// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
static char leap_year(int year) {
    return (((year%4 == 0 && year%100 != 0) || year%400 ==0));
}

static void lcd_display_clock()
{
    lcd_display_weekday();
    lcd_putc(' ');
    lcd_display_month();
    lcd_putc(' ');
    lcd_display_day();
    lcd_putc(',');
    lcd_putc(' ');
    lcd_display_year();
    lcd_gotoxy(0,1);
    lcd_putc(' ');
    lcd_putc(' ');
    lcd_putc(' ');
    lcd_putc(' ');
    lcd_display_time_attribute(hour, 4, 1);
    lcd_putc(':');
    lcd_display_time_attribute(minute, 7, 1);
    lcd_putc(':');
    lcd_display_time_attribute(second, 10, 1);
    lcd_putc(' ');
    lcd_putc(' ');
    lcd_putc(' ');
    lcd_putc(' ');
}

static void display_0(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(8);
    lcd_putc(1);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(3);
    lcd_putc(4);
    lcd_putc(5);
    lcd_putc(20);
}

static void display_1(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(1);
    lcd_putc(2);
    lcd_putc(20);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(20);
    lcd_putc(255);
    lcd_putc(20);
    lcd_putc(20);
}

static void display_2(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(6);
    lcd_putc(6);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(3);
    lcd_putc(7);
    lcd_putc(7);
    lcd_putc(20);
}

static void display_3(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(6);
    lcd_putc(6);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(7);
    lcd_putc(7);
    lcd_putc(5);
    lcd_putc(20);
}

static void display_4(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(3);
    lcd_putc(4);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(20);
    lcd_putc(20);
    lcd_putc(255);
    lcd_putc(20);
}

static void display_5(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(255);
    lcd_putc(6);
    lcd_putc(6);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(7);
    lcd_putc(7);
    lcd_putc(5);
    lcd_putc(20);
}

static void display_6(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(8);
    lcd_putc(6);
    lcd_putc(6);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(3);
    lcd_putc(7);
    lcd_putc(5);
    lcd_putc(20);
}

static void display_7(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(1);
    lcd_putc(1);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(20);
    lcd_putc(8);
    lcd_putc(20);
    lcd_putc(20);
}

static void display_8(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(8);
    lcd_putc(6);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(3);
    lcd_putc(7);
    lcd_putc(5);
    lcd_putc(20);
}

static void display_9(uint8_t position)
{
    lcd_gotoxy(position,0);
    lcd_putc(8);
    lcd_putc(6);
    lcd_putc(2);
    lcd_putc(20);
    lcd_gotoxy(position,1);
    lcd_putc(20);
    lcd_putc(20);
    lcd_putc(255);
    lcd_putc(20);
}

ISR(TIMER1_COMPA_vect)
{
    nsubticks--;
    if (nsubticks == 0)
    {
        nsubticks = TICS_PER_SECOND;
        second++;
        if(second > 59)
        {
            second = 0;
            minute++;
            if(minute > 59)
            {
                minute = 0;
                // Set at hour 1 plus minutes 60 (2 am)
                //
                // first Sunday in November decrement hour at 2 am
                // second Sunday in March increment hour at 2 am
                // (twice)
                //
                if((daylight_time == 1) && (month == 3) &&
                        (hour == 1) &&
                        (day == spring_savings())) {
                            hour++;
                            daylight_time = 0;
                } else if((daylight_time == 1) && (month == 11) &&
                        (hour == 1) &&
                        (day == fall_savings())) {
                            hour--;
                            daylight_time = 0;
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
                        daylight_time = 1;
                        if (month > 12) {
                            month = 1;
                            year++;
                        }
                    }
                }
            }
        }
    }
    debounce();
}
