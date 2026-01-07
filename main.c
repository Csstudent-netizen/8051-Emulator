// Testbench File
#include <stdio.h>
#include <string.h>
#include "system.h"

const uint8_t TEST_ROM[] = {
    // --- INITIALIZATION (Address 0x00 - 0x0E) ---
    0x75, 0xD0, 0x08,       // 00: MOV PSW, #0x08     (Bank 1)
    0x78, 0x30,             // 03: MOV R0, #0x30      (Ptr = 0x30)
    0x79, 0x01,             // 05: MOV R1, #0x01      (Val = 1)
    0x7F, 0x05,             // 07: MOV R7, #0x05      (Count = 5)

    // LOOP_INIT:
    0xE9,                   // 09: MOV A, R1
    0xF6,                   // 0A: MOV @R0, A
    0x09,                   // 0B: INC R1
    0x08,                   // 0C: INC R0
    0xDF, 0xFA,             // 0D: DJNZ R7, -6 (Jump to 0x09)

    // --- CALCULATION (Address 0x0F - 0x23) ---
    0x78, 0x30,             // 0F: MOV R0, #0x30      (Reset Ptr)
    0x7F, 0x05,             // 11: MOV R7, #0x05      (Reset Count)
    0x7A, 0x00,             // 13: MOV R2, #0x00      (Sum High)
    0x7B, 0x00,             // 15: MOV R3, #0x00      (Sum Low)
    
    // LOOP_CALC (Target for DJNZ is 0x17):
    0xE6,                   // 17: MOV A, @R0         (Load Val)
    0xF5, 0xF0,             // 18: MOV B, A           (Copy to B)
    0xA4,                   // 1A: MUL AB             (Square it)
    0x2B,                   // 1B: ADD A, R3          (Add Low)
    0xFB,                   // 1C: MOV R3, A          (Store Low)
    0xE5, 0xF0,             // 1D: MOV A, B           (Load High)
    0x3A,                   // 1F: ADDC A, R2         (Add High + C)
    0xFA,                   // 20: MOV R2, A          (Store High)
    0x08,                   // 21: INC R0             (Next Ptr)
    0xDF, 0xF3,             // 22: DJNZ R7, -13       (Jump to 0x17) *** FIXED ***

    // --- LOGIC CHECK (Address 0x24 - 0x2F) ---
    0xC3,                   // 24: CLR C
    0xEB,                   // 25: MOV A, R3          (Load Sum Low: 0x37)
    0x94, 0x0A,             // 26: SUBB A, #0x0A      (0x37 - 10)
    0x40, 0x03,             // 28: JC +3              (Skip if < 10)
    0x75, 0x90, 0xFF,       // 2A: MOV P1, #0xFF      (Turn on LED)
    
    // --- BCD CALL (Address 0x2D - 0x32) ---
    0xEB,                   // 2D: MOV A, R3          (Reload 0x37)
    0x12, 0x00, 0x50,       // 2E: LCALL 0x0050       (Jump to Subroutine at 0x50) *** FIXED ***
    0x80, 0xFE,             // 31: SJMP $             (Infinite Loop)

    // --- PADDING (0x33 - 0x4F) ---
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 

    // --- SUBROUTINE (Address 0x50) ---
    // 0x37 (55) + 0x35 = 0x6C.
    // DA A logic: Low nibble C > 9, Add 6 -> 0x72.
    0x24, 0x35,             // 50: ADD A, #0x35
    0xD4,                   // 52: DA A
    0x22                    // 53: RET
};

// Helper to print debug info
void print_state(system_8051_t *sys) {
 printf("PC: 0x%04X   | A: 0x%02X    | PSW : 0x%02X\n"
        "B: 0x%02X    | SP: 0x%02X   | DPTR: 0x%04X\n"
        "TCON: 0x%02X | TMOD: 0x%02X | TL0: 0x%02X   | TH0: 0x%02X\n"
        "TL1: 0x%02X  | TH1: 0x%02X  | SCON: 0x%02X  | SBUF: 0x%02X\n"
        "R0: 0x%02X   | R1: 0x%02X   | R2: 0x%02X    | R3: 0x%02X    | R4: 0x%02X | R5: 0x%02X   | R6: 0x%02X   | R7: 0x%02X  (Bank0)\n"
        "R0: 0x%02X   | R1: 0x%02X   | R2: 0x%02X    | R3: 0x%02X    | R4: 0x%02X | R5: 0x%02X   | R6: 0x%02X   | R7: 0x%02X  (Bank1)\n"
        "R0: 0x%02X   | R1: 0x%02X   | R2: 0x%02X    | R3: 0x%02X    | R4: 0x%02X | R5: 0x%02X   | R6: 0x%02X   | R7: 0x%02X  (Bank2)\n"
        "R0: 0x%02X   | R1: 0x%02X   | R2: 0x%02X    | R3: 0x%02X    | R4: 0x%02X | R5: 0x%02X   | R6: 0x%02X   | R7: 0x%02X  (Bank3)\n"
        "P1: 0x%02X   | sys->iram[0x30]: 0x%02X  | sys->iram[0x31]: 0x%02X | sys->iram[0x32]: 0x%02X | sys->iram[0x33]: 0x%02X\n"
        "sys->iram[0x34]: 0x%02X | sys->iram[0x00]: 0x%02X | sys->iram[0x08]: 0x%02X\n", 
            sys->cpu.PC, 
            sys->cpu.A, 
            sys->cpu.PSW, 
            sys->cpu.B,
            sys->cpu.SP,
            sys->cpu.DPTR,
            sys->sfr.TCON,
            sys->sfr.TMOD,
            sys->sfr.TL0,
            sys->sfr.TH0,
            sys->sfr.TL1,
            sys->sfr.TH1,
            sys->sfr.SCON,
            sys->sfr.SBUF,
            sys->iram[0], sys->iram[1], sys->iram[2], sys->iram[3], sys->iram[4], sys->iram[5], sys->iram[6], sys->iram[7],
            sys->iram[8], sys->iram[9], sys->iram[10], sys->iram[11], sys->iram[12], sys->iram[13], sys->iram[14], sys->iram[15],
            sys->iram[16], sys->iram[17], sys->iram[18], sys->iram[19], sys->iram[20], sys->iram[21], sys->iram[22], sys->iram[23],
            sys->iram[24], sys->iram[25], sys->iram[26], sys->iram[27], sys->iram[28], sys->iram[29], sys->iram[30], sys->iram[31],
            sys->sfr.P1, sys->iram[0x30], sys->iram[0x31], sys->iram[0x32], sys->iram[0x33], sys->iram[0x34], sys->iram[0x00], sys->iram[0x08]);
}

int main() {
    system_8051_t sys;
    system_reset(&sys);
    
    // 2. Load the ROM into irom
    // Safety check: Don't overflow irom if test is too big
    if (sizeof(TEST_ROM) <= sizeof(sys.irom)) {
        memcpy(sys.irom, TEST_ROM, sizeof(TEST_ROM));
    } else {
        printf("Error: Test ROM too large for irom!\n");
        return 1;
    }

    for(int i=0; i<300; ++i) cpu_step(&sys);
    print_state(&sys);

    return 0;
}