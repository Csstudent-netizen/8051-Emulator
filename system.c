#include "system.h"
#include <string.h> // for memset

void system_reset(system_8051_t *sys) {
    // 1. Wipe everything to 0 first
    memset(sys, 0, sizeof(system_8051_t));

    // 2. Set CPU Core Defaults (Power On State)
    sys->cpu.PC = 0x0000;
    sys->cpu.SP = 0x07;     // Stack Pointer starts at 0x07
    sys->cpu.cycles = 0;

    // 3. Set Peripheral Defaults
    // Ports start as Input (Logic 1 / High Impedance)
    sys->sfr.P0 = 0xFF;
    sys->sfr.P1 = 0xFF;
    sys->sfr.P2 = 0xFF;
    sys->sfr.P3 = 0xFF;
    
    // 4. Set Hardware Config
    sys->EA = 1;            // Default: Boot from Internal ROM
}

// CODE FETCH (ROM)
uint8_t system_read_code(system_8051_t *sys, uint16_t address) {
    // IF EA Pin is Low (0): Force External Access for ALL addresses
    if (sys->EA == 0) {
        return sys->rom_external[address];
    }
    
    // IF EA Pin is High (1): Use Internal ROM for low addresses
    else {
        if (address < INT_ROM_SIZE) {
            return sys->rom_internal[address];
        } else {
            return sys->rom_external[address];
        }
    }
}

// INTERNAL RAM (IRAM + SFR)
uint8_t system_read_iram(system_8051_t *sys, uint8_t address) {
    if (address < 0x80) {
        return sys->iram[address]; // 0x00-0x7F: Direct RAM
    } else {
        return 0; // TODO: Implement SFR Read Logic (Phase 2)
    }
}

void system_write_iram(system_8051_t *sys, uint8_t address, uint8_t value) {
    if (address < 0x80) {
        sys->iram[address] = value; // 0x00-0x7F: Direct RAM
    } 
    // TODO: Implement SFR Write Logic (Phase 2)
}

// EXTERNAL RAM (XRAM)
uint8_t system_read_xram(system_8051_t *sys, uint16_t address) {
    return sys->xram[address];
}

void system_write_xram(system_8051_t *sys, uint16_t address, uint8_t value) {
    sys->xram[address] = value;
}