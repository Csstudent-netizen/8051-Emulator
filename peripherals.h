#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <stdint.h>

typedef struct {
    // TIMERS
    uint8_t TCON;       // Timer Control (Bit Addressable)
    uint8_t TMOD;       // Timer Mode
    uint8_t TL0;        // Timer 0 Low
    uint8_t TH0;        // Timer 0 High
    uint8_t TL1;        // Timer 1 Low
    uint8_t TH1;        // Timer 1 High

    // SERIAL
    uint8_t SCON;       // Serial Control (Bit Addressable)
    uint8_t SBUF;       // Serial Buffer

    // I/O PORTS (all bit addressable)
    uint8_t P0;
    uint8_t P1;
    uint8_t P2;
    uint8_t P3;         // (Will have alt function bitmasks)

    // INTERRUPTS & POWER
    uint8_t IE;         // Interrupt Enable (Bit Addressable)
    uint8_t IP;         // Interrupt Priority (Bit Addressable)
    uint8_t PCON;       // Power Control
} peripherals_t;

// BIT MASKS

// TCON (Timer Control)
#define TCON_TF1 0x80
#define TCON_TR1 0x40
#define TCON_TF0 0x20
#define TCON_TR0 0x10
#define TCON_IE1 0x08
#define TCON_IT1 0x04
#define TCON_IE0 0x02
#define TCON_IT0 0x01

// IE (Interrupt Enable)
#define IE_EA    0x80
#define IE_ET2   0x20  // Timer 2 Interrupt Enable (Reserved in 8051, ET2 in 8052)
#define IE_ES    0x10
#define IE_ET1   0x08
#define IE_EX1   0x04
#define IE_ET0   0x02
#define IE_EX0   0x01

// IP (Interrupt Priority)
#define IP_PT2   0x20  // Timer 2 Priority (Reserved in 8051, PT2 in 8052)
#define IP_PS    0x10
#define IP_PT1   0x08
#define IP_PX1   0x04
#define IP_PT0   0x02
#define IP_PX0   0x01

// SCON (Serial Control)
#define SCON_SM0 0x80
#define SCON_SM1 0x40
#define SCON_SM2 0x20
#define SCON_REN 0x10
#define SCON_TB8 0x08
#define SCON_RB8 0x04
#define SCON_TI  0x02
#define SCON_RI  0x01

// P3 Alternate Functions
#define P3_RD    0x80  
#define P3_WR    0x40  
#define P3_T1    0x20  
#define P3_T0    0x10  
#define P3_INT1  0x08  
#define P3_INT0  0x04  
#define P3_TXD   0x02  
#define P3_RXD   0x01 

//Interrupt Vectors
#define VECTOR_RESET  0x0000 
#define VECTOR_INT0   0x0003 
#define VECTOR_TIMER0 0x000B 
#define VECTOR_INT1   0x0013 
#define VECTOR_TIMER1 0x001B 
#define VECTOR_SERIAL 0x0023 
#define VECTOR_TIMER2 0x002B

void peripherals_reset(peripherals_t *periph);

#endif