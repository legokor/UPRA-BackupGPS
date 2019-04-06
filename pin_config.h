#ifndef _PINCONFIG_H
#define _PINCONFIG_H

// LEDs
sbit RED_LED     at PORTC.B5;
sbit RED_LED_dir at TRISC.B5;
sbit YELLOW_LED     at PORTC.B4;
sbit YELLOW_LED_dir at TRISC.B4;

// GSM module connections
sbit GSM_DTR     at LATA.B2;
sbit GSM_DTR_dir at TRISA.B2;
sbit GSM_PWRKEY     at LATC.B0;
sbit GSM_PWRKEY_dir at TRISC.B0;

// GSM module UART (TRIS bits and PPS settings)
sbit RXPIN_dir at TRISC.B1;
sbit TXPIN_dir at TRISC.B2;
#define RXPIN 0b10001
#define TXPIN RC2PPS

// Diag port
#define _DIAGPORT &PORTA
#define _DIAG_RXPIN 5
#define _DIAG_TXPIN 4
#define _DIAG_BAUD 9600

#endif
