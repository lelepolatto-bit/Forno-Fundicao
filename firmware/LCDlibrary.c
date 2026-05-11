#include "LCDPRINCIPALS.h"
#include <avr/pgmspace.h>

#define LCD_LINE_0 0x80U
#define LCD_LINE_1 0xC0U

static void lcdPulseEnable(void);
static void lcdWriteNibble(uint8_t nibble);

static void lcdPulseEnable(void)
{
    setBit(LCD_PORT, LCD_E);
    _delay_us(1);
    clearBit(LCD_PORT, LCD_E);
    _delay_us(100);
}

static void lcdWriteNibble(uint8_t nibble)
{
    /* Envia o nibble para D4..D7 do LCD ligados em PD4..PD7. */
    if (nibble & 0x01U) { setBit(LCD_PORT, LCD_D4); } else { clearBit(LCD_PORT, LCD_D4); }
    if (nibble & 0x02U) { setBit(LCD_PORT, LCD_D5); } else { clearBit(LCD_PORT, LCD_D5); }
    if (nibble & 0x04U) { setBit(LCD_PORT, LCD_D6); } else { clearBit(LCD_PORT, LCD_D6); }
    if (nibble & 0x08U) { setBit(LCD_PORT, LCD_D7); } else { clearBit(LCD_PORT, LCD_D7); }

    lcdPulseEnable();
}

void initLCD(void)
{
    LCD_DDR |= (uint8_t)((1U << LCD_RS) | (1U << LCD_E) |
                         (1U << LCD_D4) | (1U << LCD_D5) |
                         (1U << LCD_D6) | (1U << LCD_D7));

    clearBit(LCD_PORT, LCD_RS);
    clearBit(LCD_PORT, LCD_E);
    _delay_ms(15);

    lcdWriteNibble(0x03U);
    _delay_ms(5);
    lcdWriteNibble(0x03U);
    _delay_us(150);
    lcdWriteNibble(0x03U);
    lcdWriteNibble(0x02U);

    sendData(0x28U, LCD_COMMAND); /* 4 bits, 2 linhas, fonte 5x8. */
    displayON_OFF(1U);
    clearDisplay();
    entryMode();
}

void sendData(unsigned char DATA, uint8_t command)
{
    if (command) {
        setBit(LCD_PORT, LCD_RS);
    } else {
        clearBit(LCD_PORT, LCD_RS);
    }

    lcdWriteNibble((uint8_t)(DATA >> 4));
    lcdWriteNibble((uint8_t)(DATA & 0x0FU));

    if ((DATA == 0x01U) || (DATA == 0x02U)) {
        _delay_ms(2);
    }
}

void sendString(const char *string, uint8_t row)
{
    uint8_t count = 0U;

    setLine(row);
    while ((*string != '\0') && (count < 16U)) {
        sendData((unsigned char)*string, LCD_CHARACTER);
        string++;
        count++;
    }
}

void sendStringFlash(const char *stringFlash, uint8_t row)
{
    uint8_t count = 0U;
    char character;

    setLine(row);
    character = (char)pgm_read_byte(stringFlash);
    while ((character != '\0') && (count < 16U)) {
        sendData((unsigned char)character, LCD_CHARACTER);
        stringFlash++;
        count++;
        character = (char)pgm_read_byte(stringFlash);
    }
}

void lcdPrintFlash(uint8_t row, const char *stringFlash)
{
    uint8_t count = 0U;
    char character;

    setLine(row);
    character = (char)pgm_read_byte(stringFlash);
    while ((character != '\0') && (count < 16U)) {
        sendData((unsigned char)character, LCD_CHARACTER);
        stringFlash++;
        count++;
        character = (char)pgm_read_byte(stringFlash);
    }

    while (count < 16U) {
        sendData(' ', LCD_CHARACTER);
        count++;
    }
}

void clearDisplay(void)
{
    sendData(0x01U, LCD_COMMAND);
}

void homePosition(void)
{
    sendData(0x02U, LCD_COMMAND);
}

void displayON_OFF(uint8_t state)
{
    if (state) {
        sendData(0x0CU, LCD_COMMAND);
    } else {
        sendData(0x08U, LCD_COMMAND);
    }
}

void entryMode(void)
{
    sendData(0x06U, LCD_COMMAND);
}

void setLine(uint8_t row)
{
    if (row == 0U) {
        sendData(LCD_LINE_0, LCD_COMMAND);
    } else {
        sendData(LCD_LINE_1, LCD_COMMAND);
    }
}
