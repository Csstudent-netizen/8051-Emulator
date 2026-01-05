// Testbench File
#include <stdio.h>
#include <string.h>
#include "system.h"


void run_alu_test(system_8051_t *chip, char* name, uint8_t opcode, uint8_t operand, uint8_t initial_A, uint8_t initial_PSW) {
    // 1. Reset & Inject State directly
    system_reset(chip);
    chip->cpu.A = initial_A;
    chip->cpu.PSW = initial_PSW;

    // 2. Inject Code (Opcode + Operand)
    chip->irom[0] = opcode;
    chip->irom[1] = operand;

    // 3. Execute one step
    cpu_step(chip);

    // 4. Print Result
    // We print PSW in binary-like format (CY, AC, OV) for easier reading
    printf("[%s] %02X op %02X -> A:%02X | Flags: ", name, initial_A, operand, chip->cpu.A);
    
    if (chip->cpu.PSW & PSW_CY) printf("CY ");
    if (chip->cpu.PSW & PSW_AC) printf("AC ");
    if (chip->cpu.PSW & PSW_OV) printf("OV ");
    if (chip->cpu.PSW & PSW_P)  printf("P ");
    printf("\n");
}

// Helper to print debug info
void print_state(system_8051_t *sys) {
    printf("PC: 0x%04X | A: 0x%02X | PSW: 0x%02X | CY: %d\n", 
           sys->cpu.PC, 
           sys->cpu.A, 
           sys->cpu.PSW, 
           (sys->cpu.PSW >> 7) & 1);
}

int main() {
    //system_8051_t chip;
    /*
    printf("=== ALU RAPID FIRE TESTS ===\n\n");

    // ----------------------------------------
    // GROUP 1: ADD (Opcode 0x24)
    // ----------------------------------------
    
    // 1. Simple Math
    // 10 (0x0A) + 5 (0x05) = 15 (0x0F)
    run_alu_test(&chip, "ADD Simple   ", 0x24, 0x05, 0x0A, 0x00);

    // 2. Carry Test
    // 255 (0xFF) + 1 = 0 (with Carry)
    run_alu_test(&chip, "ADD Carry    ", 0x24, 0x01, 0xFF, 0x00);

    // 3. Aux Carry Test
    // 0x0F + 1 = 0x10. (Bit 3 overflowed to Bit 4, so AC should set)
    run_alu_test(&chip, "ADD AuxCarry ", 0x24, 0x01, 0x0F, 0x00);

    // 4. Signed Overflow (The "Disaster" Test)
    // +127 (0x7F) + 1 = +128? NO, 0x80 is -128.
    // This flips the sign from Pos to Neg. OV should set.
    run_alu_test(&chip, "ADD Overflow ", 0x24, 0x01, 0x7F, 0x00);


    printf("\n");
    // ----------------------------------------
    // GROUP 2: SUBB (Opcode 0x94)
    // ----------------------------------------

    // 1. Simple Subtraction
    // 5 - 3 = 2
    run_alu_test(&chip, "SUBB Simple  ", 0x94, 0x03, 0x05, 0x00);

    // 2. Subtraction WITH Previous Carry
    // 5 - 3 - 1(Carry) = 1
    run_alu_test(&chip, "SUBB w/Carry ", 0x94, 0x03, 0x05, PSW_CY);

    // 3. Underflow (Generates Carry/Borrow)
    // 0 - 1 = 255 (0xFF) with Borrow (CY)
    run_alu_test(&chip, "SUBB Borrow  ", 0x94, 0x01, 0x00, 0x00);

    // 4. Signed Overflow (Negative - Positive = Positive)
    // -128 (0x80) - 1 = +127 (0x7F)? 
    // Sign flipped purely due to math error. OV should set.
    run_alu_test(&chip, "SUBB Overflow", 0x94, 0x01, 0x80, 0x00); */

   /* // TEST: LOOP (3 Iterations)
    printf("\n--- LOOP TEST ---\n");
    system_reset(&chip);
    
    // Program:
    // 0x00: MOV A, #3 (0x74, 0x03)
    // 0x02: DEC A     (0x14)     <-- Label "LOOP" is here
    // 0x03: JNZ -3    (0x70, 0xFD)
    
    chip.irom[0] = 0x74; chip.irom[1] = 0x03;
    chip.irom[2] = 0x14;
    chip.irom[3] = 0x70; chip.irom[4] = 0xFD; // -3 signed

    // Step 1: Execute MOV (A=3)
    cpu_step(&chip);
    printf("Step 1 (MOV): A=%d PC=%04X\n", chip.cpu.A, chip.cpu.PC);

    // Step 2: Execute DEC (A=2)
    cpu_step(&chip);
    printf("Step 2 (DEC): A=%d PC=%04X\n", chip.cpu.A, chip.cpu.PC);

    // Step 3: Execute JNZ (Jump back to 0x02)
    cpu_step(&chip);
    printf("Step 3 (JNZ): A=%d PC=%04X (Should be 0002)\n", chip.cpu.A, chip.cpu.PC);

    // Step 4: Execute DEC again (A=1)
    cpu_step(&chip);
    printf("Step 4 (DEC): A=%d\n", chip.cpu.A);

    // ... Run until A=0
    cpu_step(&chip); // JNZ
    cpu_step(&chip); // DEC (A=0)
    cpu_step(&chip); // JNZ (Should NOT jump now)
    
    printf("Final: A=%d PC=%04X (Should be 0005)\n", chip.cpu.A, chip.cpu.PC); */

    /* // TEST: LCALL and RET
    printf("\n--- FUNCTION CALL TEST ---\n");
    system_reset(&chip);

    // 0x0000: LCALL 0x0010 (Opcode 12, High 00, Low 10)
    chip.irom[0] = 0x12; 
    chip.irom[1] = 0x00; 
    chip.irom[2] = 0x10; 

    // 0x0003: MOV A, #55 (Opcode 74, Val 37) - Return Point
    chip.irom[3] = 0x74;
    chip.irom[4] = 0x37;

    // 0x0010: MOV A, #99 (Opcode 74, Val 63) - The Subroutine
    chip.irom[0x10] = 0x74; 
    chip.irom[0x11] = 0x63;

    // 0x0012: RET (Opcode 22)
    chip.irom[0x12] = 0x22;

    // Step 1: Execute LCALL
    // PC should jump to 0010. SP should go from 07 -> 09.
    cpu_step(&chip);
    printf("Step 1 (Post-Call): PC=%04X SP=%02X\n", chip.cpu.PC, chip.cpu.SP);
    
    // Check Stack content:
    // SP (09) should hold High Byte (00)
    // SP-1 (08) should hold Low Byte (03) because next instr was at 0x0003
    printf("       Stack Top [09]: %02X (Expect 00)\n", chip.iram[chip.cpu.SP]);
    printf("       Stack - 1 [08]: %02X (Expect 03)\n", chip.iram[chip.cpu.SP-1]);

    // Step 2: Execute Function Body (MOV A, #99)
    cpu_step(&chip);
    printf("Step 2 (In Function): A=%d\n", chip.cpu.A);

    // Step 3: Execute RET
    // PC should go back to 0003. SP should drop back to 07.
    cpu_step(&chip);
    printf("Step 3 (Post-RET): PC=%04X SP=%02X\n", chip.cpu.PC, chip.cpu.SP);

    // Step 4: Execute final MOV A, #55
    cpu_step(&chip);
    printf("Step 4 (Back Home): A=%d\n", chip.cpu.A); */

    /* // TEST: Register Bank Switching
    printf("\n--- REGISTER BANK TEST ---\n");
    system_reset(&chip);

    // 1. MOV R0, #0x11 (Opcode 78, Val 11) -> Default Bank 0
    chip.irom[0] = 0x78;
    chip.irom[1] = 0x11;

    // 2. MOV PSW, #0x08 (Opcode 75, Addr D0, Val 08) -> Switch to Bank 1
    chip.irom[2] = 0x75;
    chip.irom[3] = 0xD0; 
    chip.irom[4] = 0x08; 

    // 3. MOV R0, #0x22 (Opcode 78, Val 22) -> Now in Bank 1
    chip.irom[5] = 0x78;
    chip.irom[6] = 0x22;

    // Run 3 instructions
    cpu_step(&chip); // Step 1
    cpu_step(&chip); // Step 2 (Switch Bank)
    cpu_step(&chip); // Step 3

    printf("Bank 0 R0 (Addr 0x00): %02X (Expect 11)\n", chip.iram[0x00]);
    printf("Bank 1 R0 (Addr 0x08): %02X (Expect 22)\n", chip.iram[0x08]);
    
    // Check internal CPU state
    printf("Final PSW: %02X (Expect 08)\n", chip.cpu.PSW); */

    /* // TEST: Register Arithmetic & Logic
    printf("\n--- REGISTER MATH & LOGIC TEST ---\n");
    system_reset(&chip);

    // 1. INC R0 (Overflow Test)
    // Initialize R0 = 255 (0xFF). 
    // Opcode 0x78 (MOV R0, #imm)
    chip.irom[0] = 0x78; 
    chip.irom[1] = 0xFF;

    // Execute INC R0. 
    // Opcode 0x08 (INC R0)
    // Expectation: R0 becomes 0x00. PSW (Carry) remains 0.
    chip.irom[2] = 0x08; 

    // 2. XRL A, R1 (Invert Test)
    // Initialize A = 0x55 (Binary 0101 0101)
    chip.irom[3] = 0x74; chip.irom[4] = 0x55; 

    // Initialize R1 = 0xFF (Binary 1111 1111)
    chip.irom[5] = 0x79; chip.irom[6] = 0xFF; 

    // Execute XRL A, R1 (A = A ^ R1)
    // 0x55 ^ 0xFF should be 0xAA (1010 1010) - Bitwise NOT
    // Opcode 0x69 (XRL A, R1)
    chip.irom[7] = 0x69; 

    // --- RUN & VERIFY ---

    // Step 1: MOV R0, #FF
    cpu_step(&chip); 
    
    // Step 2: INC R0
    cpu_step(&chip);
    printf("INC R0 (255->0): Val=%02X (Expect 00) | PSW=%02X (Expect 00 - No Carry!)\n", 
           chip.iram[0x00], chip.cpu.PSW);

    // Step 3: MOV A, #55
    cpu_step(&chip);
    
    // Step 4: MOV R1, #FF
    cpu_step(&chip);

    // Step 5: XRL A, R1
    cpu_step(&chip);
    printf("XRL A, R1: A=%02X (Expect AA)\n", chip.cpu.A); */

    system_8051_t sys;
    system_reset(&sys);

    printf("--- 8051 Emulator Test: Boolean Gauntlet ---\n");

    // 2. Load the Test Program into Internal ROM
    // This program tests SETB, CLR, CPL, JBC, JC, JNB
    
    uint8_t program[] = {
        // INIT: Clear A
        0x74, 0x00,       // 0x00: MOV A, #0x00

        // TEST 1: SETB C (Carry) and Jump if Carry (JC)
        0xD3,             // 0x02: SETB C           (C should be 1)
        0x40, 0x04,       // 0x03: JC +4            (Jump to 0x09)
        0x74, 0xEE,       // 0x05: MOV A, #0xEE     (FAIL CODE 1)
        0x80, 0x18,       // 0x07: SJMP to END      (Skip to halt)

        // TEST 2: CLR bit and Jump if Not Bit (JNB)
        0xC2, 0x00,       // 0x09: CLR 0x00         (Clear Bit 0 of RAM 0x20)
        0x30, 0x00, 0x04, // 0x0B: JNB 0x00, +4     (Jump to 0x12)
        0x74, 0xDD,       // 0x0E: MOV A, #0xDD     (FAIL CODE 2)
        0x80, 0x0F,       // 0x10: SJMP to END

        // TEST 3: CPL bit and Jump if Bit (JB)
        0xB2, 0x00,       // 0x12: CPL 0x00         (Flip Bit 0 -> becomes 1)
        0x20, 0x00, 0x04, // 0x14: JB 0x00, +4      (Jump to 0x1B)
        0x74, 0xCC,       // 0x17: MOV A, #0xCC     (FAIL CODE 3)
        0x80, 0x06,       // 0x19: SJMP to END

        // TEST 4: JBC (Jump if Bit set, THEN Clear bit)
        0x10, 0x00, 0x04, // 0x1B: JBC 0x00, +4     (Jump to 0x22 AND clear bit)
        0x74, 0xBB,       // 0x1E: MOV A, #0xBB     (FAIL CODE 4 - Didn't Jump)
        0x80, 0x02,       // 0x20: SJMP to END

        // CHECK 4: Ensure Bit was actually cleared by JBC
        0x30, 0x00, 0x04, // 0x22: JNB 0x00, +4     (Jump to Success)
        0x74, 0xAA,       // 0x25: MOV A, #0xAA     (FAIL CODE 5 - Bit wasn't cleared)
        0x80, 0xFE,       // 0x27: SJMP -2 (Halt)

        // SUCCESS
        0x74, 0xFF,       // 0x29: MOV A, #0xFF     (Success Code)
        0x80, 0xFE        // 0x2B: SJMP -2 (Infinite Loop / Halt)
    };

    // Load program into system memory
    memcpy(sys.irom, program, sizeof(program));

    // 3. Run the CPU
    // We run for enough cycles to complete the logic (approx 50 steps is plenty)
    int steps = 0;
    while (steps < 50) {
        cpu_step(&sys);
        steps++;

        // Stop if we hit the infinite loop at the end (Instruction 0x80)
        // We check if the PC is stuck at the SUCCESS or FAIL trap
        uint8_t current_opcode = system_read_code(&sys, sys.cpu.PC);
        if (current_opcode == 0x80) { // SJMP detected
             // Run one last step to execute the jump and settle the PC
             cpu_step(&sys);
             break;
        }
    }

    // 4. Validate Results
    printf("\n--- Test Results ---\n");
    print_state(&sys);

    if (sys.cpu.A == 0xFF) {
        printf("\n[SUCCESS] Boolean Logic & Jumps working perfectly!\n");
    } else {
        printf("\n[FAILURE] Test failed with Error Code: 0x%02X\n", sys.cpu.A);
        printf("Error Codes Key:\n");
        printf("  0xEE: JC failed (Carry Flag logic)\n");
        printf("  0xDD: JNB or CLR failed\n");
        printf("  0xCC: JB or CPL failed\n");
        printf("  0xBB: JBC failed to jump\n");
        printf("  0xAA: JBC jumped but failed to clear the bit\n");
    }

    return 0;
}