# avr-clock
AVR interface to LCD screen to make a digital clock.

My thanks to Peter Fleury http://homepage.hispeed.ch/peterfleury/index.html for his lcd.c and lcd.h library files.  My thanks to sniglen and Peter Dannegger for the debounce.c and debounce.h library files, https://www.avrfreaks.net/forum/tutsofthard-button-debouncing-software . 

Required hardware.

One atmega162 AVR, digikey part ATMEGA162-16PU-ND
One Hitachi HD44780 or compatible LCD screen, digikey part 1528-1502-ND
Three push buttons, digikey part 1568-1332-ND
One wall wort 5V DC, digikey part 102-4159-ND
One DC Jack, digikey part EJ503A-ND
One 4MHz clock crystal, digikey part 535-9057-ND
Two 27pf capacitors, digikey part BC1035CT-ND
One 3.2x2" breadboard, digikey part 1528-1143-ND

Optional
One 40 pin socket for integrated circuits, digikey part AE10008-ND

Required software, older versions will probably work, but have not been
tested.
avrdude 6.3
avr-libc 2.0
avr-gcc 7.2.0
avr-binutils 2.30
