#ifndef CLOCK_H
#define CLOCK_H

//
// function prototypes
// 
static inline void lcd_display_clock(void);
static inline void lcd_display_day(void);
static inline void lcd_display_weekday(void);
static inline void lcd_display_month(void);
static inline void lcd_display_year(void);
static inline void set_second(void);
static inline void set_minute(void);
static inline void set_hour(void);
static inline void set_day(void);
static inline void set_month(void);
static inline void set_year(void);
static inline char spring_savings(void);
static inline char fall_savings(void);
static inline char day_of_week(int, char, char);
static inline char day_of_month(int, char, char, char);
static inline char leap_year(int);
static inline void lcd_display_time_attribute(uint8_t, uint8_t, uint8_t);

#endif // CLOCK_H
