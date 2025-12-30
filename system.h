#ifndef SYSTEM_H
#define SYSTEM_H

#include "cpu.h"
#include "peripherals.h"

//  SYSTEM CONFIGURATION
typedef struct {
    uint8_t EA;            // External Access Pin (1=Use Internal ROM, 0=External Only)
    uint16_t int_rom_size; // Size of Internal ROM (Standard 8051 = 4096 bytes)
} system_config_t;

//  THE MOTHERBOARD
typedef struct {
    cpu_core_t      cpu;        
    peripherals_t   sfr;        
    system_config_t cfg;        // The Hardware Config
    
    // Internal RAM
    uint8_t         iram[128];   

    // External RAM
    uint8_t         xram[65536]; 
    
    // CODE MEMORY
    uint8_t         rom_internal[4096]; 

    uint8_t         rom_external[65536]; 

} system_8051_t;

// CONTROL FUNCTIONS
void system_reset(system_8051_t *sys);

// MEMORY BUS FUNCTIONS
// Code Bus (Handles EA Logic)
uint8_t system_read_code(system_8051_t *sys, uint16_t address);

// Internal Data Bus (Handles RAM vs SFR split)
uint8_t system_read_iram(system_8051_t *sys, uint8_t address);
void    system_write_iram(system_8051_t *sys, uint8_t address, uint8_t value);

// External Data Bus (Direct MOVX access)
uint8_t system_read_xram(system_8051_t *sys, uint16_t address);
void    system_write_xram(system_8051_t *sys, uint16_t address, uint8_t value);

#endif