// Intel Hex format reader
#include <stdint.h>
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

// Helper to print debug info
void print_state(system_8051_t *sys) {

}

int main(int argc, char *argv[]) {
    system_8051_t sys;
    system_reset(&sys);

    const char *filename = "blinky.hex";
    if (argc < 2) {
        printf("Usage: %s <filename.hex>\n", argv[0]);
        return 1;
    }

    if(load_hex(&sys, argv[1])) return 1;

    while(1) {
        uint64_t prev_cycles = sys.cpu.cycles;

        cpu_step(&sys);

        int step_cycles = (int)(sys.cpu.cycles - prev_cycles);

        peripherals_step(&sys, step_cycles);

    }

    return 0;
}