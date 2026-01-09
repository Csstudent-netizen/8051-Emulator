#include "system.h"
#include <stdio.h>

void peripherals_step(system_8051_t * sys, uint64_t step_cycles) {
    int ticks = step_cycles / 12;
    if(ticks == 0) ticks = 1;
    uint8_t t0_mode = sys->sfr.TMOD & 0x03;
    uint8_t t1_mode = sys->sfr.TMOD & 0x30;

    if(t0_mode == 0x03) {
        if (sys->sfr.TCON & TCON_TR0) { 
             uint16_t val = sys->sfr.TL0 + ticks;
             if (val > 0xFF) {
                 sys->sfr.TCON |= TCON_TF0;
                 val &= 0xFF;
             }
             sys->sfr.TL0 = (uint8_t)val;
        }
        if (sys->sfr.TCON & TCON_TR1) {
             uint16_t val = sys->sfr.TH0 + ticks;
             if (val > 0xFF) {
                 sys->sfr.TCON |= TCON_TF1;
                 val &= 0xFF;
             }
             sys->sfr.TH0 = (uint8_t)val;
        }

        return;
    }

    if(sys->sfr.TCON & TCON_TR0) {
        int count;
        if(t0_mode == 0x01) {
            count = ((uint16_t)sys->sfr.TH0 << 8) + sys->sfr.TL0 + ticks;
            if(count > 0xFFFF) {
                sys->sfr.TCON |= TCON_TF0;
                count &= 0xFFFF;
            }
            sys->sfr.TH0 = count >> 8;
            sys->sfr.TL0 = count & 0xFF;
        }
        else if(t0_mode == 0x00) {
            count = ((uint16_t)sys->sfr.TH0 << 5) + (sys->sfr.TL0 & 0x1F) + ticks;
            if(count > 0x1FFF) {
                sys->sfr.TCON |= TCON_TF0;
                count &= 0x1FFF;
            }
            sys->sfr.TH0 = count >> 5;
            sys->sfr.TL0 = count & 0x1F;
        }
        else if(t0_mode == 0x02) {
            count = sys->sfr.TL0 + ticks;
            if(count > 0xFF) {
                sys->sfr.TCON |= TCON_TF0;
                int ov = count - 0x100;
                count = sys->sfr.TH0 + ov;
            }
            sys->sfr.TL0 = count & 0xFF;
        }
    }

    if(sys->sfr.TCON & TCON_TR1) {
        int count;
        if(t1_mode == 0x10) {
            count = ((uint16_t)sys->sfr.TH1 << 8) + sys->sfr.TL1 + ticks;
            if(count > 0xFFFF) {
                sys->sfr.TCON |= TCON_TF1;
                count &= 0xFFFF;
            }
            sys->sfr.TH1 = count >> 8;
            sys->sfr.TL1 = count & 0xFF;
        }
        else if(t1_mode == 0x00) {
            count = ((uint16_t)sys->sfr.TH1 << 5) + (sys->sfr.TL1 & 0x1F) + ticks;
            if(count > 0x1FFF) {
                sys->sfr.TCON |= TCON_TF1;
                count &= 0x1FFF;
            }
            sys->sfr.TH1 = count >> 5;
            sys->sfr.TL1 = count & 0x1F;
        }
        else if(t1_mode == 0x20) {
            count = sys->sfr.TL1 + ticks;
            if(count > 0xFF) {
                sys->sfr.TCON |= TCON_TF1;
                int ov = count - 0x100;
                count = sys->sfr.TH1 + ov;
            }
            sys->sfr.TL1 = count & 0xFF;
        }
    }
}