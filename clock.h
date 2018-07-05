#ifndef CLOCK_H
#define CLOCK_H

//
// function prototypes
// 
static void buttons_init(void);
static void timer_init(void);
static void lcd_display_clock(void);
static void lcd_display_day(void);
static void lcd_display_weekday(void);
static void lcd_display_month(void);
static void lcd_display_year(void);
static void set_second(void);
static void set_minute(void);
static void set_hour(void);
static void set_day(void);
static void set_month(void);
static void set_year(void);
static char spring_savings(void);
static char fall_savings(void);
static char day_of_week(int, char, char);
static char day_of_month(int, char, char, char);
static char leap_year(int);
static void lcd_display_time_attribute(uint8_t, uint8_t, uint8_t);

#endif // CLOCK_H
