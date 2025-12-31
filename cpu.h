#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// CORE REGISTERS
typedef struct {
    uint8_t A;          // Accumulator
    uint8_t B;          // B Register
    uint8_t PSW;        // Flags
    uint8_t SP;         // Stack Pointer
    uint16_t PC;        // Program Counter
    uint16_t DPTR;      // Data Pointer (16-bit)
    
    uint64_t cycles;    // Clock Cycle Counter
} cpu_core_t;

// PSW MASKS
#define PSW_CY   0x80
#define PSW_AC   0x40
#define PSW_F0   0x20
#define PSW_RS1  0x10
#define PSW_RS0  0x08
#define PSW_OV   0x04
#define PSW_P    0x01

#endif