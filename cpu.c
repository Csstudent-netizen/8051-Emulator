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

//handling direct addressing for SFRs
uint8_t iram_read(system_8051_t *sys, uint8_t address) {
    if(address < 0x80) return sys->iram[address];
    else {
        switch (address) {
            case 0xE0: return sys->cpu.A;
            
            case 0xF0: return sys->cpu.B;

            case 0x81: return sys->cpu.SP;

            case 0xD0: return sys->cpu.PSW;

            case 0x88: return sys->sfr.TCON;
            
            case 0x89: return sys->sfr.TMOD;
            
            case 0x8A: return sys->sfr.TL0;

            case 0x8B: return sys->sfr.TL1;

            case 0x8C: return sys->sfr.TH0;

            case 0x8D: return sys->sfr.TH1;

            //more to come
            
            default:
                printf("Unknown SFR address: 0x%02X\n", address);
                return 0;
        }
    }
}

void iram_write(system_8051_t *sys, uint8_t address, uint8_t value) {
    if(address < 0x80) sys->iram[address] = value;
    else {
        switch (address) {
            case 0xE0:
                sys->cpu.A = value;
                update_parity(sys);
                break;
            
            case 0xF0: sys->cpu.B = value; break;

            case 0x81: sys->cpu.SP = value; break;

            case 0xD0: sys->cpu.PSW = value; break;

            case 0x88: sys->sfr.TCON = value; break; 

            case 0x89: sys->sfr.TMOD = value; break; 

            case 0x8A: sys->sfr.TL0 = value; break;

            case 0x8B: sys->sfr.TL1 = value; break;

            case 0x8C: sys->sfr.TH0 = value; break;

            case 0x8D: sys->sfr.TH1 = value; break;

            //more to come
            
            default:
                printf("Unknown SFR address: 0x%02X\n", address);
                break;
        }
    }
}

uint8_t get_rx_addr(system_8051_t *sys, uint8_t reg_index) {
    if(reg_index < 0 || reg_index > 7) {
        printf("Invalid Rx index\n");
        return 0x00;
    }

    uint8_t bank = ((sys->cpu.PSW & PSW_RS1) + (sys->cpu.PSW & PSW_RS0)) >> 3;

    switch (bank) {
        case 0x00: return 0x00 + reg_index;


        case 0x01: return 0x08 + reg_index;

        case 0x02: return 0x10 + reg_index;

        case 0x03: return 0x18 + reg_index;

        default: printf("Invalid bank\n"); return 0x00;
    }

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

        case 0xC0: { //PUSH addr
            uint8_t target = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.SP++;
            uint8_t val = iram_read(sys, target);
            sys->iram[sys->cpu.SP] = val;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xD0: { //POP addr
            uint8_t target = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            uint8_t val = sys->iram[sys->cpu.SP];
            iram_write(sys, target, val);
            sys->cpu.SP--;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x12: { //LCALL ladd
            uint16_t highadd = (uint16_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t lowadd = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)sys->cpu.PC;
            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)(sys->cpu.PC >> 8);
            sys->cpu.PC = (uint16_t)((highadd << 8) + lowadd);

            sys->cpu.cycles += 24;
            break;
        }

        case 0x22: { //RET
            uint16_t highadd = (uint16_t)sys->iram[sys->cpu.SP];
            sys->cpu.SP--;
            uint8_t lowadd = sys->iram[sys->cpu.SP];
            sys->cpu.SP--;
            sys->cpu.PC = (uint16_t)((highadd << 8) + lowadd);

            sys->cpu.cycles += 24;
            break;
        }

        //MOV A, Rx
        case 0xE8: case 0xE9: case 0xEA: case 0xEB: 
        case 0xEC: case 0xED: case 0xEE: case 0xEF: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);

            sys->cpu.A = sys->iram[rx_addr];
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //MOV Rx, A
        case 0xF8: case 0xF9: case 0xFA: case 0xFB: 
        case 0xFC: case 0xFD: case 0xFE: case 0xFF: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);

            sys->iram[rx_addr] = sys->cpu.A;
            sys->cpu.cycles += 12;
            break;
        }
        
        default:
            printf("ERROR: Unknown Opcode 0x%02X at Address 0x%04X\n", opcode, sys->cpu.PC - 1);
            break;
            
    }
}