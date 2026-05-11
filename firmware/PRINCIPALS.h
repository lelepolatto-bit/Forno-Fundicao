#ifndef PRINCIPALS_H
#define PRINCIPALS_H

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define setBit(reg, bit)     ((reg) |= (1 << (bit)))
#define clearBit(reg, bit)   ((reg) &= ~(1 << (bit)))
#define toggleBit(reg, bit)  ((reg) ^= (1 << (bit)))
#define readBit(reg, bit)    ((reg) & (1 << (bit)))

#endif
