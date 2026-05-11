#ifndef LCDPRINCIPALS_H
#define LCDPRINCIPALS_H

#include "PRINCIPALS.h"
#include <avr/pgmspace.h>

/* Mapeamento do LCD 16x2 em 4 bits no PORTD do ATmega328P. */
#define LCD_PORT PORTD
#define LCD_DDR DDRD
#define LCD_PIN PIND

#define LCD_RS PD2
#define LCD_E PD3
#define LCD_D4 PD4
#define LCD_D5 PD5
#define LCD_D6 PD6
#define LCD_D7 PD7

#define LCD_COMMAND 0U
#define LCD_CHARACTER 1U
#define LCD_COLUMNS 16U

void initLCD(void);
void sendData(unsigned char DATA, uint8_t command);
void sendString(const char *string, uint8_t row);
void sendStringFlash(const char *stringFlash, uint8_t row);
void lcdPrintFlash(uint8_t row, const char *stringFlash);
void clearDisplay(void);
void homePosition(void);
void displayON_OFF(uint8_t state);
void entryMode(void);
void setLine(uint8_t row);

#endif
