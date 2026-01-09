// Intel Hex format reader
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"

int load_hex(system_8051_t *sys, const char *filename) {
    FILE *file = fopen(filename, "r");
    if(file == NULL) {
        printf("Could not open file %s\n", filename);
        return 1;
    }

    char line[1024];
    int line_num = 0;
    while(fgets(line, sizeof(line), file)) {
        line_num++;

        if(line[0] != ':') {
            if(line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;
            else {
                printf("Invalid hex line %d\n", line_num);
                return 1;
            }
        }

        int byte_count, address, record_type;
        if(sscanf(line + 1, "%02X%04X%02X", &byte_count, &address, &record_type) != 3) {
            printf("Cannot parse header of line %d\n", line_num);
            fclose(file);
            return 1;
        }

        if(record_type == 0x00) {
            char *ptr = line + 9;   

            for(int i = 0; i < byte_count; ++i) {
                int data_byte;
                if(sscanf(ptr, "%02X", &data_byte) != 1) {
                    printf("Cannot parse data byte of line %d\n", line_num);
                    fclose(file);
                    return 1;
                }

                if(address + i < INT_ROM_SIZE) {
                    sys->irom[address + i] = data_byte;
                }
                else {
                    sys->xrom[address + i] = data_byte;
                }

                ptr += 2;
            }
        }
        else if(record_type == 0x01) { //EOF record
            break;
        }
    }

    fclose(file);
    printf("File loaded\n");
    return 0;
}


void print_state(system_8051_t *sys) {
    // 1. Decode PSW (Flags)
    char psw_str[9] = "--------";
    if (sys->cpu.PSW & 0x80) psw_str[0] = 'C'; // Carry
    if (sys->cpu.PSW & 0x40) psw_str[1] = 'A'; // Aux Carry
    if (sys->cpu.PSW & 0x20) psw_str[2] = 'F'; // Flag 0
    if (sys->cpu.PSW & 0x04) psw_str[5] = 'V'; // Overflow
    if (sys->cpu.PSW & 0x01) psw_str[7] = 'P'; // Parity

    //Decode Register Bank (R0-R7)
    uint8_t bank_num = (sys->cpu.PSW & 0x18) >> 3;
    uint8_t bank_addr = bank_num * 8;
    
    //Decode Timers (Logic Updated for Mode 3)
    uint8_t t0_mode = sys->sfr.TMOD & 0x03;
    uint8_t t1_mode = (sys->sfr.TMOD >> 4) & 0x03;

    char t0_val_str[20];
    char t0_status_str[20];
    char t1_val_str[20];
    char t1_status_str[20];

    // --- TIMER 0 HANDLING ---
    if (t0_mode == 3) {
        // Mode 3: Split Timer. TL0 uses TR0, TH0 uses TR1.
        sprintf(t0_val_str, "TL:%02X TH:%02X", sys->sfr.TL0, sys->sfr.TH0);
        sprintf(t0_status_str, "TR0:%d TR1:%d", 
               (sys->sfr.TCON & 0x10) ? 1 : 0, 
               (sys->sfr.TCON & 0x40) ? 1 : 0);
    } 
    else {
        // Standard Modes (0, 1, 2)
        uint16_t val = ((uint16_t)sys->sfr.TH0 << 8) | sys->sfr.TL0;
        sprintf(t0_val_str, "0x%04X     ", val);
        sprintf(t0_status_str, "%s       ", (sys->sfr.TCON & 0x10) ? "RUN" : "STP");
    }

    uint16_t t1_val = ((uint16_t)sys->sfr.TH1 << 8) | sys->sfr.TL1;
    sprintf(t1_val_str, "0x%04X     ", t1_val);
    
    if (t0_mode == 3) {
        sprintf(t1_status_str, "No Int(Baud)"); 
    } else {
        sprintf(t1_status_str, "%s       ", (sys->sfr.TCON & 0x40) ? "RUN" : "STP");
    }


    printf("\n");
    printf("====================================================================\n");
    printf(" SYSTEM TIME: %-10lu CYCLES  |  PC: 0x%04X  |  OPCODE: 0x%02X\n", 
           sys->cpu.cycles, sys->cpu.PC, system_read_code(sys, sys->cpu.PC));
    printf("====================================================================\n");

    // --- TABLE 1: CPU CORE & REGISTERS (Removed Raw PSW Row) ---
    printf("| CORE REGISTERS      | FLAGS: %s | ACTIVE BANK: %d (0x%02X)  |\n", psw_str, bank_num, bank_addr);
    printf("|---------------------+------------------+-----------------------|\n");
    printf("| A    : 0x%02X         | R0 : 0x%02X        | R4 : 0x%02X             |\n", sys->cpu.A, sys->iram[bank_addr+0], sys->iram[bank_addr+4]);
    printf("| B    : 0x%02X         | R1 : 0x%02X        | R5 : 0x%02X             |\n", sys->cpu.B, sys->iram[bank_addr+1], sys->iram[bank_addr+5]);
    printf("| SP   : 0x%02X         | R2 : 0x%02X        | R6 : 0x%02X             |\n", sys->cpu.SP, sys->iram[bank_addr+2], sys->iram[bank_addr+6]);
    printf("| DPTR : 0x%04X       | R3 : 0x%02X        | R7 : 0x%02X             |\n", sys->cpu.DPTR, sys->iram[bank_addr+3], sys->iram[bank_addr+7]);
    printf("|---------------------+------------------+-----------------------|\n");

    // --- TABLE 2: TIMERS & INTERRUPTS (Dynamic Strings) ---
    printf("\n| TIMERS & INTERRUPTS | TCON: 0x%02X       | TMOD: 0x%02X            |\n", sys->sfr.TCON, sys->sfr.TMOD);
    printf("|---------------------+------------------+-----------------------|\n");
    printf("| TIMER 0: %s|STATUS: %s| MODE: %d               |\n", t0_val_str, t0_status_str, t0_mode);
    printf("| TIMER 1: %s|STATUS: %s| MODE: %d               |\n", t1_val_str, t1_status_str, t1_mode);
    printf("| IE     : 0x%02X       | IP    : 0x%02X     |                       |\n", sys->sfr.IE, sys->sfr.IP);
    printf("|---------------------+------------------+-----------------------|\n");

    // --- TABLE 3: PORTS ---
    printf("\n| I/O PORTS           |                                          |\n");
    printf("|---------------------+------------------+-----------------------|\n");
    printf("| P0: 0x%02X            | P1: 0x%02X         | P2: 0x%02X   | P3: 0x%02X |\n", 
           sys->sfr.P0, sys->sfr.P1, sys->sfr.P2, sys->sfr.P3);
    printf("====================================================================\n");
}



int main(int argc, char *argv[]) {
    system_8051_t sys;
    system_reset(&sys);

    if (argc < 2) {
        printf("Usage: %s <filename.hex>\n", argv[0]);
        return 1;
    }

    if(load_hex(&sys, argv[1])) return 1;

    printf("Use 's', 'r' or 'q', where:\n");
    printf("'r' is to directly view state after max ~20000000 instructions\n's' for stepwise status\n'q' for exiting emulator");
    char input_buffer[100];

    while(1) {
        printf("\nEnter command: ");
        if(fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("No command entered.");
            break;
        }
        char cmd = input_buffer[0];

        if (cmd == 'q') {
            printf("Exiting emulator.\n");
            break;
        }
        else if(cmd == 's' || cmd == '\n') {
            uint64_t prev_cycles = sys.cpu.cycles;
            cpu_step(&sys);
            int step_cycles = (int)(sys.cpu.cycles - prev_cycles);
            peripherals_step(&sys, step_cycles);
            print_state(&sys);
        }
        else if(cmd == 'r') {
            int instructions_executed = 0;
            int batch_limit = 20000000;
            int halted = 0;

            while(instructions_executed < batch_limit) {
                uint8_t op = system_read_code(&sys, sys.cpu.PC);
                uint8_t arg = system_read_code(&sys, sys.cpu.PC+1);
    
                if (op == 0x80 && arg == 0xFE) {
                    halted = 1;
                    break;
                }
                uint64_t prev_cycles = sys.cpu.cycles;
                cpu_step(&sys);
                int step_cycles = (int)(sys.cpu.cycles - prev_cycles);
                peripherals_step(&sys, step_cycles);
                instructions_executed++;
            }
            if (halted) {
                printf("Program Halted normally (SJMP $ detected).\n");
            }
            else {
                printf("Execution Paused (Batch limit reached).\n");
            }

            print_state(&sys);
        }
        else {
            printf("Unknown command.");
        }

    }

    return 0;
}