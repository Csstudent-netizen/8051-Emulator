#include "system.h"
#include <stdio.h>

// Count the 1s in Accumulator
static void update_parity(system_8051_t *sys) {
    
    uint8_t count = 0;
    uint8_t a = sys->cpu.A;
    
    for (int i = 0; i < 8; i++) {
        if (a & (1 << i)) {
            count++;
        }
    }
    
    if (count % 2 != 0) sys->cpu.PSW |= PSW_P;
    else sys->cpu.PSW &= ~PSW_P;
}

void cpu_step(system_8051_t *sys) {
    // 1. FETCH
    uint8_t opcode = system_read_code(sys, sys->cpu.PC);
    
    // 2. INCREMENT
    sys->cpu.PC++;
    
    // 3. DECODE & EXECUTE
    switch (opcode) {
        
        case 0x00: //NOP
            sys->cpu.cycles += 12;
            break;
        
        case 0x74: //MOV A, #value
            sys->cpu.A = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            sys->cpu.cycles += 12;
            break;

        case 0x04: //INC A
            sys->cpu.A++;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;

        case 0x14: //DEC A
            sys->cpu.A--;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;

        case 0x24: { //ADD A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            uint8_t old_A = sys->cpu.A;
            uint16_t result = old_A + val;

            //CY
            uint8_t c7 = (result > 0xFF);
            if (c7) sys->cpu.PSW |= PSW_CY;
            else sys->cpu.PSW &= ~PSW_CY;

            //Overflow calc
            uint8_t c6 = ((old_A & 0x7F) + (val & 0x7F)) > 0x7F;
            if (c6 ^ c7) sys->cpu.PSW |= PSW_OV;
            else sys->cpu.PSW &= ~PSW_OV;

            //AC
            if ((old_A & 0x0F) + (val & 0x0F) > 0x0F) sys->cpu.PSW |= PSW_AC;
            else sys->cpu.PSW &= ~PSW_AC;

            sys->cpu.A = (uint8_t)result;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x94: {//SUBB A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            uint8_t carry = (sys->cpu.PSW & PSW_CY) ? 1 : 0;
            uint16_t result = sys->cpu.A - val - carry;

            //CY (Borrow)
            // If result > 255, it means we wrapped around (underflowed)
            // e.g. 5 - 6 = -1 (0xFFFF in uint16). 0xFFFF > 0xFF.
            if (result > 0xFF) sys->cpu.PSW |= PSW_CY;
            else sys->cpu.PSW &= ~PSW_CY;

            //AC (Aux Borrow)
            if ((sys->cpu.A & 0x0F) < (val & 0x0F) + carry) sys->cpu.PSW |= PSW_AC;
            else sys->cpu.PSW &= ~PSW_AC;

            //OV
            uint8_t c7 = (result > 0xFF);
            uint8_t c6 = ((sys->cpu.A & 0x7F) < ((val & 0x7F) + carry));
            if (c6 ^ c7) sys->cpu.PSW |= PSW_OV;
            else sys->cpu.PSW &= ~PSW_OV;

            sys->cpu.A = (uint8_t)result;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xA4: { //MUL AB
            sys->cpu.PSW &= ~PSW_CY;
            uint16_t product = sys->cpu.A * sys->cpu.B;
            sys->cpu.A = (uint8_t)product;
            sys->cpu.B = (uint8_t)(product>>8);

            if(sys->cpu.B) sys->cpu.PSW |= PSW_OV;
            else sys->cpu.PSW &= ~PSW_OV;

            update_parity(sys);
            sys->cpu.cycles += 48;
            break;
        }

        case 0x84: { //DIV AB
            sys->cpu.PSW &= ~PSW_CY;
            if(sys->cpu.B == 0){
                sys->cpu.PSW |= PSW_OV;
                sys->cpu.cycles += 48;
                break;
            }
            else sys->cpu.PSW &= ~PSW_OV;

            uint8_t q = sys->cpu.A/sys->cpu.B;
            uint8_t r = sys->cpu.A%sys->cpu.B;
            sys->cpu.A = q;
            sys->cpu.B = r;

            update_parity(sys);
            sys->cpu.cycles += 48;
            break;
        }

        case 0x54: { //ANL A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.A &= val;

            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x44: { //ORL A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.A |= val;

            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x64: { //XRL A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.A ^= val;

            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x60: { //JZ, label
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.A == 0) sys->cpu.PC += offset;

            sys->cpu.cycles += 24;
            break;
        }

        case 0x70: { //JNZ, label
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.A != 0) sys->cpu.PC += offset;

            sys->cpu.cycles += 24;
            break;
        }
        
        default:
            printf("ERROR: Unknown Opcode 0x%02X at Address 0x%04X\n", opcode, sys->cpu.PC - 1);
            break;
            
    }
}