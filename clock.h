#ifndef CLOCK_H
#define CLOCK_H

//
// function prototypes
// 
extern void lcd_display_clock(void);
extern void lcd_display_second(void);
extern void lcd_display_minute(void);
extern void lcd_display_hour(void);
extern void lcd_display_day(void);
extern void lcd_display_weekday(void);
extern void lcd_display_month(void);
extern void lcd_display_year(void);
extern void set_second(void);
extern void set_minute(void);
extern void set_hour(void);
extern void set_day(void);
extern void set_month(void);
extern void set_year(void);
extern char spring_savings(void);
extern char fall_savings(void);
extern char day_of_week(int, char, char);
extern char day_of_month(int, char, char, char);
extern char leap_year(int);

#endif // CLOCK_H
