// Testbench File
#include <stdio.h>
#include "system.h"

int main() {
    // Instantiate the Chip
    system_8051_t chip;
    system_reset(&chip);

    printf("8051 SYSTEM CHECK \n");

    // TEST 1: Internal RAM
    system_write_iram(&chip, 0x05, 0x42);
    if (system_read_iram(&chip, 0x05) == 0x42) {
        printf("[PASS] Internal RAM R/W (Address 0x05)\n");
    } else {
        printf("[FAIL] Internal RAM Error\n");
    }

    // TEST 2: External RAM
    system_write_xram(&chip, 0x0005, 0x99);
    // Ensure writing to XRAM didn't overwrite IRAM
    if (system_read_xram(&chip, 0x0005) == 0x99 && system_read_iram(&chip, 0x05) == 0x42) {
        printf("[PASS] External RAM R/W (Address 0x0005) - Separated from IRAM\n");
    } else {
        printf("[FAIL] Memory Overlap Error! IRAM and XRAM should be separate.\n");
    }

    // TEST 3: ROM Mapping (EA Pin Logic)
    chip.rom_internal[0x0000] = 0xAA; // Internal Marker
    chip.rom_external[0x0000] = 0xBB; // External Marker

    // Case A: EA = 1 (Default) -> Should read Internal
    chip.cfg.EA = 1;
    uint8_t val1 = system_read_code(&chip, 0x0000);
    
    // Case B: EA = 0 -> Should read External
    chip.cfg.EA = 0;
    uint8_t val2 = system_read_code(&chip, 0x0000);

    if (val1 == 0xAA && val2 == 0xBB) {
        printf("[PASS] ROM Banking / EA Pin Logic works.\n");
    } else {
        printf("[FAIL] ROM Banking Error. Read: %02X / %02X\n", val1, val2);
    }

    return 0;
}