#ifndef SYSTEM_H
#define SYSTEM_H

#include "cpu.h"
#include "peripherals.h"

#define INT_ROM_SIZE 4096


//  THE MOTHERBOARD
typedef struct {
    cpu_core_t cpu;        
    peripherals_t sfr;        
    uint8_t EA; //External access  
    
    // Internal RAM
    uint8_t iram[128 + 128]; //to handle indirect addressing (case where Rx contains value greater than 0x7F)

    // External RAM
    uint8_t xram[65536]; 
    
    // CODE MEMORY
    uint8_t irom[INT_ROM_SIZE]; 

    uint8_t xrom[65536]; 

} system_8051_t;

void system_reset(system_8051_t *sys);

uint8_t system_read_code(system_8051_t *sys, uint16_t address);

uint8_t system_read_iram(system_8051_t *sys, uint8_t address);
void system_write_iram(system_8051_t *sys, uint8_t address, uint8_t value);

uint8_t system_read_xram(system_8051_t *sys, uint16_t address);
void system_write_xram(system_8051_t *sys, uint16_t address, uint8_t value);

void cpu_step(system_8051_t *sys);

#endif