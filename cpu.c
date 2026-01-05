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

static void alu_add(system_8051_t *sys, uint8_t val) {
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
}

static void alu_subb(system_8051_t *sys, uint8_t val) {
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
}

//handling direct addressing for SFRs
static uint8_t iram_read(system_8051_t *sys, uint8_t address) {
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

static void iram_write(system_8051_t *sys, uint8_t address, uint8_t value) {
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

static uint8_t get_rx_addr(system_8051_t *sys, uint8_t reg_index) {
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

static uint8_t get_indirect_addr(system_8051_t *sys, uint8_t reg_index) {
    if(reg_index != 0 && reg_index != 1) {
        printf("Invalid register pointer\n");
        return 0x00;
    }

    return sys->iram[get_rx_addr(sys, reg_index)];
}

static uint8_t bit_read(system_8051_t * sys, uint8_t bit_addr) {
    if(bit_addr < 0x80) { //iram
        uint8_t byte_addr = 0x20 + (bit_addr >> 3);
        uint8_t bit_index = bit_addr & 0x07;
        if(iram_read(sys, byte_addr) & (0x01 << bit_index)) return 0x01;
        else return 0x00;
    }
    //SFRs
    uint8_t byte_addr = bit_addr & 0xF8;
    uint8_t bit_index = bit_addr & 0x07;
    if(iram_read(sys, byte_addr) & (0x01 << bit_index)) return 0x01;
    else return 0x00;
}

static void bit_write(system_8051_t *sys, uint8_t bit_addr, uint8_t val) {
    if(bit_addr < 0x80) { //iram
        uint8_t byte_addr = 0x20 + (bit_addr >> 3);
        uint8_t bit_index = bit_addr & 0x07;
        if(val) sys->iram[byte_addr] |= (0x01 << bit_index);
        else sys->iram[byte_addr] &= ~(0x01 << bit_index);
        return;
    }
    //SFRs
    uint8_t byte_addr = bit_addr & 0xF8;
    uint8_t bit_index = bit_addr & 0x07;
    if(val) {
        uint8_t send = iram_read(sys, byte_addr) | (0x01 << bit_index);
        iram_write(sys, byte_addr, send);
    }
    else {
        uint8_t send = iram_read(sys, byte_addr) & ~(0x01 << bit_index);
        iram_write(sys, byte_addr, send);
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

            alu_add(sys, val);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x94: {//SUBB A, #value
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            alu_subb(sys, val);
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

        case 0x60: { //JZ label
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.A == 0) sys->cpu.PC += offset;

            sys->cpu.cycles += 24;
            break;
        }

        case 0x70: { //JNZ label
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.A != 0) sys->cpu.PC += offset;

            sys->cpu.cycles += 24;
            break;
        }

        case 0x80: { //SJMP sadd
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x02: { //LJMP ladd
            uint16_t highaddr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t lowaddr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.PC = (uint16_t)((highaddr << 8) + lowaddr);
            sys->cpu.cycles += 24;
            break;
        }

        //DJNZ Rx, label
        case 0xD8: case 0xD9: case 0xDA: case 0xDB:
        case 0xDC: case 0xDD: case 0xDE: case 0xDF: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->iram[rx_addr]--;
            if(sys->iram[rx_addr] != 0) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xD5: { //DJNZ addr, label
            uint8_t target = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            uint8_t val = iram_read(sys, target);
            val--;
            iram_write(sys, target, val);
            if(val != 0) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xB4: { //CJNE A, #value, label
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            if(sys->cpu.A < val) {
                sys->cpu.PSW |= PSW_CY;
                sys->cpu.PC += offset;
            }
            else if(sys->cpu.A > val) {
                sys->cpu.PSW &= ~PSW_CY;
                sys->cpu.PC += offset;
            }
            else {
                sys->cpu.PSW &= ~PSW_CY;
            }

            sys->cpu.cycles += 24;
            break;
        }

        case 0xB5: { //CJNE A, addr, label
            uint8_t target = system_read_code(sys, sys->cpu.PC);
            uint8_t val = iram_read(sys, target);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            if(sys->cpu.A < val) {
                sys->cpu.PSW |= PSW_CY;
                sys->cpu.PC += offset;
            }
            else if(sys->cpu.A > val) {
                sys->cpu.PSW &= ~PSW_CY;
                sys->cpu.PC += offset;
            }
            else {
                sys->cpu.PSW &= ~PSW_CY;
            }

            sys->cpu.cycles += 24;
            break;
        }

        //CJNE Rx, #value, label
        case 0xB8: case 0xB9: case 0xBA: case 0xBB:
        case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            if(sys->iram[rx_addr] < val) {
                sys->cpu.PSW |= PSW_CY;
                sys->cpu.PC += offset;
            }
            else if(sys->iram[rx_addr] > val) {
                sys->cpu.PSW &= ~PSW_CY;
                sys->cpu.PC += offset;
            }
            else {
                sys->cpu.PSW &= ~PSW_CY;
            }

            sys->cpu.cycles += 24;
            break;
        }

        //CJNE @Rx, #value, label
        case 0xB6: case 0xB7: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            if(sys->iram[target] < val) {
                sys->cpu.PSW |= PSW_CY;
                sys->cpu.PC += offset;
            }
            else if(sys->iram[target] > val) {
                sys->cpu.PSW &= ~PSW_CY;
                sys->cpu.PC += offset;
            }
            else {
                sys->cpu.PSW &= ~PSW_CY;
            }

            sys->cpu.cycles += 24;
            break;
        }

        //AJMP lower8 
        case 0x01: case 0x21: case 0x41: case 0x61:
        case 0x81: case 0xA1: case 0xC1: case 0xE1: {
            uint16_t mid3 = (uint16_t)((opcode & 0xE0) << 3); //3 bits after top 5
            uint16_t low8 = (uint16_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint16_t high5 = sys->cpu.PC & 0xF800; //top 5 bits

            sys->cpu.PC = high5 + mid3 + low8;
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
            uint16_t highaddr = (uint16_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t lowaddr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)sys->cpu.PC;
            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)(sys->cpu.PC >> 8);
            sys->cpu.PC = (uint16_t)((highaddr << 8) + lowaddr);

            sys->cpu.cycles += 24;
            break;
        }

        //ACALL lower8
        case 0x11: case 0x31: case 0x51: case 0x71:
        case 0x91: case 0xB1: case 0xD1: case 0xF1: {
            uint16_t mid3 = (uint16_t)((opcode & 0xE0) << 3); //3 bits after top 5
            uint16_t low8 = (uint16_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint16_t high5 = sys->cpu.PC & 0xF800; //top 5 bits

            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)sys->cpu.PC;
            sys->cpu.SP++;
            sys->iram[sys->cpu.SP] = (uint8_t)(sys->cpu.PC >> 8);
            sys->cpu.PC = high5 + mid3 + low8;

            sys->cpu.cycles += 24;
            break;
        }

        case 0x22: { //RET
            uint16_t highaddr = (uint16_t)sys->iram[sys->cpu.SP];
            sys->cpu.SP--;
            uint8_t lowaddr = sys->iram[sys->cpu.SP];
            sys->cpu.SP--;
            sys->cpu.PC = (uint16_t)((highaddr << 8) + lowaddr);

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

        //MOV Rx, #value
        case 0x78: case 0x79: case 0x7A: case 0x7B:
        case 0x7C: case 0x7D: case 0x7E: case 0x7F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->iram[rx_addr] = val;
            sys->cpu.cycles += 12;
            break;
        }

        //ADD A, Rx
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);
            uint8_t val = sys->iram[rx_addr];

            alu_add(sys, val);
            sys->cpu.cycles += 12;
            break;
        }

        //SUBB A, Rx
        case 0x98: case 0x99: case 0x9A: case 0x9B:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);
            uint8_t val = sys->iram[rx_addr];

            alu_subb(sys, val);
            sys->cpu.cycles += 12;
            break;
        }

        //INC Rx
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);

            sys->iram[rx_addr]++;
            sys->cpu.cycles += 12;
            break;
        }

        //DEC Rx
        case 0x18: case 0x19: case 0x1A: case 0x1B:
        case 0x1C: case 0x1D: case 0x1E: case 0x1F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t rx_addr = get_rx_addr(sys, reg_index);

            sys->iram[rx_addr]--;
            sys->cpu.cycles += 12;
            break;
        }

        //ANL A, Rx
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t addr = get_rx_addr(sys, reg_index);
            
            sys->cpu.A &= sys->iram[addr];
            
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //ORL A, Rx
        case 0x48: case 0x49: case 0x4A: case 0x4B:
        case 0x4C: case 0x4D: case 0x4E: case 0x4F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t addr = get_rx_addr(sys, reg_index);
            
            sys->cpu.A |= sys->iram[addr];
            
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //XRL A, Rx
        case 0x68: case 0x69: case 0x6A: case 0x6B:
        case 0x6C: case 0x6D: case 0x6E: case 0x6F: {
            uint8_t reg_index = opcode & 0x07;
            uint8_t addr = get_rx_addr(sys, reg_index);
            
            sys->cpu.A ^= sys->iram[addr];
            
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x75: { //MOV addr, #value
            uint8_t target = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            iram_write(sys, target, val);
            sys->cpu.cycles += 24;
            break;
        }

        //MOV A, @Rx
        case 0xE6: case 0xE7: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);

            sys->cpu.A = sys->iram[target];
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //MOV @Rx, A
        case 0xF6: case 0xF7: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);

            sys->iram[target] = sys->cpu.A;
            sys->cpu.cycles += 12;
            break;
        }

        //MOV @Rx, #value
        case 0x76: case 0x77: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            uint8_t val = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->iram[target] = val;
            sys->cpu.cycles += 12;
            break;
        }

        //ADD A, @Rx
        case 0x26: case 0x27: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            uint8_t val = sys->iram[target];

            alu_add(sys, val);
            sys->cpu.cycles += 12;
            break;
        }

        //SUBB A, @Rx
        case 0x96: case 0x97: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            uint8_t val = sys->iram[target];

            alu_subb(sys, val);
            sys->cpu.cycles += 12;
            break;
        }

        //ANL A, @Rx
        case 0x56: case 0x57: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            
            sys->cpu.A &= sys->iram[target];
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //ORL A, @Rx
        case 0x46: case 0x47: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            
            sys->cpu.A |= sys->iram[target];
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //XRL A, @Rx
        case 0x66: case 0x67: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);
            
            sys->cpu.A ^= sys->iram[target];
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        //INC @Rx
        case 0x06: case 0x07: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);

            sys->iram[target]++;
            sys->cpu.cycles += 12;
            break;
        }

        //DEC @Rx
        case 0x16: case 0x17: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t target = get_indirect_addr(sys, reg_index);

            sys->iram[target]--;
            sys->cpu.cycles += 12;
            break;
        }

        //MOV addr, addr
        case 0x85: {
            uint8_t src = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t dest = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            uint8_t val = iram_read(sys, src);
            iram_write(sys, dest, val);
            sys->cpu.cycles += 24;
            break;
        }

        case 0x23: { //RL A
            uint8_t temp = (sys->cpu.A & 0x80) ? 0x01 : 0x00;
            sys->cpu.A <<= 1;
            sys->cpu.A |= temp;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x03: { //RR A
            uint8_t temp = (sys->cpu.A & 0x01) ? 0x80 : 0x00;
            sys->cpu.A >>= 1;
            sys->cpu.A |= temp;
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xC4: { //SWAP A
            sys->cpu.A = (sys->cpu.A << 4) + (sys->cpu.A >> 4);
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x33: { //RLC A
            uint8_t old_cy = (sys->cpu.PSW & PSW_CY) ? 0x01 : 0x00;
            uint8_t new_cy = sys->cpu.A & 0x80; 
            sys->cpu.A <<= 1;
            sys->cpu.A |= old_cy;
            sys->cpu.PSW = (new_cy) ? sys->cpu.PSW | PSW_CY : sys->cpu.PSW & ~PSW_CY;
            
            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0x13: { //RRC A
            uint8_t old_cy = (sys->cpu.PSW & PSW_CY) ? 0x80 : 0x00;
            uint8_t new_cy = sys->cpu.A & 0x01; 
            sys->cpu.A >>= 1;
            sys->cpu.A |= old_cy;
            sys->cpu.PSW = (new_cy) ? sys->cpu.PSW | PSW_CY : sys->cpu.PSW & ~PSW_CY;

            update_parity(sys);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xC3: { //CLR C
            sys->cpu.PSW &= ~PSW_CY;
            sys->cpu.cycles += 12;
            break;
        }

        case 0xD3: { //SETB C
            sys->cpu.PSW |= PSW_CY;
            sys->cpu.cycles += 12;
            break;
        }

        case 0xB3: { //CPL C
            sys->cpu.PSW ^= PSW_CY;
            sys->cpu.cycles += 12;
            break;
        }

        case 0xC2: { //CLR bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            bit_write(sys, bit_addr, 0x00);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xD2: { //SETB bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            bit_write(sys, bit_addr, 0x01);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xB2: { //CPL bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            uint8_t val = bit_read(sys, bit_addr);
            bit_write(sys, bit_addr, !val);
            sys->cpu.cycles += 12;
            break;
        }

        case 0xA2: { //MOV C, bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(bit_read(sys, bit_addr)) sys->cpu.PSW |= PSW_CY;
            else sys->cpu.PSW &= ~PSW_CY;
            sys->cpu.cycles += 12;
            break;
        }

        case 0x92: { //MOV bit_addr, C
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.PSW & PSW_CY) bit_write(sys, bit_addr, 0x01);
            else bit_write(sys, bit_addr, 0x00);
            sys->cpu.cycles += 24;
            break;
        }

        case 0x82: { //ANL C, bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(bit_read(sys, bit_addr)) ;
            else sys->cpu.PSW &= ~PSW_CY;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xB0: { //ANL C, /[bit_addr]
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(!(bit_read(sys, bit_addr))) ;
            else sys->cpu.PSW &= ~PSW_CY;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x72: { //ORL C, bit_addr
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(bit_read(sys, bit_addr)) sys->cpu.PSW |= PSW_CY;
            else ;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xA0: { //ORL C, /[bit_addr]
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(!(bit_read(sys, bit_addr))) sys->cpu.PSW |= PSW_CY;
            else ;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x40: { //JC label 
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(sys->cpu.PSW & PSW_CY) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x50: { //JNC label 
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(!(sys->cpu.PSW & PSW_CY)) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x20: { //JB bit_addr, label 
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(bit_read(sys, bit_addr)) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x30: { //JNB bit_addr, label 
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(!(bit_read(sys, bit_addr))) sys->cpu.PC += offset;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x10: { //JBC bit_addr, label 
            uint8_t bit_addr = system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            int8_t offset = (int8_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;
            if(bit_read(sys, bit_addr)) {
                sys->cpu.PC += offset;
                bit_write(sys, bit_addr, 0x00);
            }
            sys->cpu.cycles += 24;
            break;
        }

        case 0x90: { //MOV DPTR, #value16
            uint16_t high = ((uint16_t)system_read_code(sys, sys->cpu.PC)) << 8;
            sys->cpu.PC++;
            uint16_t low = (uint16_t)system_read_code(sys, sys->cpu.PC);
            sys->cpu.PC++;

            sys->cpu.DPTR = high + low;
            sys->cpu.cycles += 24;
            break;
        }

        case 0xA3: { //INC DPTR
            sys->cpu.DPTR++;
            sys->cpu.cycles += 24;
            break;
        }

        case 0x93: { //MOVC A, @A+DPTR
            uint16_t addr = sys->cpu.DPTR + sys->cpu.A;
            sys->cpu.A = system_read_code(sys, addr);
            sys->cpu.cycles += 24;
            break;
        }

        case 0x83: { //MOVC A, @A+PC
            uint16_t addr = sys->cpu.PC + sys->cpu.A;
            sys->cpu.A = system_read_code(sys, addr);
            sys->cpu.cycles += 24;
            break;
        }

        case 0xE0: { //MOVX A, @DPTR
            sys->cpu.A = system_read_xram(sys, sys->cpu.DPTR);
            sys->cpu.cycles += 24;
            break;
        }

        case 0xF0: { //MOVX @DPTR, A
            system_write_xram(sys, sys->cpu.DPTR, sys->cpu.A);
            sys->cpu.cycles += 24;
            break;
        }

        //MOVX A, @Rx
        case 0xE2: case 0xE3: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t low = sys->iram[get_rx_addr(sys, reg_index)];
            uint16_t high = (uint16_t)sys->sfr.P2; // Paging byte
            uint16_t addr = (high << 8) + low;
            
            sys->cpu.A = system_read_xram(sys, addr);
            sys->cpu.cycles += 24;
            break;
        }

        // MOVX @Rx, A
        case 0xF2: case 0xF3: {
            uint8_t reg_index = opcode & 0x01;
            uint8_t low = sys->iram[get_rx_addr(sys, reg_index)];
            uint16_t high = (uint16_t)sys->sfr.P2;
            uint16_t addr = (high << 8) + low;
            
            system_write_xram(sys, addr, sys->cpu.A);
            sys->cpu.cycles += 24;
            break;
        }

        default:
            printf("ERROR: Unknown Opcode 0x%02X at Address 0x%04X\n", opcode, sys->cpu.PC - 1);
            break;
            
    }
}