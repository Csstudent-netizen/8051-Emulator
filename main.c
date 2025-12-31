// Testbench File
#include <stdio.h>
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

int main() {
    system_8051_t chip;
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

    // TEST: LOOP (3 Iterations)
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
    
    printf("Final: A=%d PC=%04X (Should be 0005)\n", chip.cpu.A, chip.cpu.PC);

    return 0;
}