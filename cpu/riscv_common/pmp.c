#include "pmp.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

read_pmpcfg_macro(0)
read_pmpcfg_macro(1)
read_pmpcfg_macro(2)
read_pmpcfg_macro(3)
read_pmpcfg_macro(4)
read_pmpcfg_macro(5)
read_pmpcfg_macro(6)
read_pmpcfg_macro(7)
read_pmpcfg_macro(8)
read_pmpcfg_macro(9)
read_pmpcfg_macro(10)
read_pmpcfg_macro(11)
read_pmpcfg_macro(12)
read_pmpcfg_macro(13)
read_pmpcfg_macro(14)
read_pmpcfg_macro(15)

uint32_t (*read_pmpcfg_list[16])(void) = {
    read_pmpcfg0,
    read_pmpcfg1,
    read_pmpcfg2,
    read_pmpcfg3,
    read_pmpcfg4,
    read_pmpcfg5,
    read_pmpcfg6,
    read_pmpcfg7,
    read_pmpcfg8,
    read_pmpcfg9,
    read_pmpcfg10,
    read_pmpcfg11,
    read_pmpcfg12,
    read_pmpcfg13,
    read_pmpcfg14,
    read_pmpcfg15,
};

uint32_t read_pmpcfg(unsigned int reg_num) {
    assert(reg_num < 16);
    return read_pmpcfg_list[reg_num]();
}


write_pmpcfg_macro(0)
write_pmpcfg_macro(1)
write_pmpcfg_macro(2)
write_pmpcfg_macro(3)
write_pmpcfg_macro(4)
write_pmpcfg_macro(5)
write_pmpcfg_macro(6)
write_pmpcfg_macro(7)
write_pmpcfg_macro(8)
write_pmpcfg_macro(9)
write_pmpcfg_macro(10)
write_pmpcfg_macro(11)
write_pmpcfg_macro(12)
write_pmpcfg_macro(13)
write_pmpcfg_macro(14)
write_pmpcfg_macro(15)

void (*write_pmpcfg_list[16])(uint32_t value) = {
    write_pmpcfg0,
    write_pmpcfg1,
    write_pmpcfg2,
    write_pmpcfg3,
    write_pmpcfg4,
    write_pmpcfg5,
    write_pmpcfg6,
    write_pmpcfg7,
    write_pmpcfg8,
    write_pmpcfg9,
    write_pmpcfg10,
    write_pmpcfg11,
    write_pmpcfg12,
    write_pmpcfg13,
    write_pmpcfg14,
    write_pmpcfg15,
};

void write_pmpcfg(unsigned int reg_num, uint32_t value) {
    assert(reg_num < 16);
    write_pmpcfg_list[reg_num](value);
}


read_pmpaddr_macro(0)
read_pmpaddr_macro(1)
read_pmpaddr_macro(2)
read_pmpaddr_macro(3)
read_pmpaddr_macro(4)
read_pmpaddr_macro(5)
read_pmpaddr_macro(6)
read_pmpaddr_macro(7)
read_pmpaddr_macro(8)
read_pmpaddr_macro(9)
read_pmpaddr_macro(10)
read_pmpaddr_macro(11)
read_pmpaddr_macro(12)
read_pmpaddr_macro(13)
read_pmpaddr_macro(14)
read_pmpaddr_macro(15)

uint32_t (*read_pmpaddr_list[16])(void) = {
    read_pmpaddr0,
    read_pmpaddr1,
    read_pmpaddr2,
    read_pmpaddr3,
    read_pmpaddr4,
    read_pmpaddr5,
    read_pmpaddr6,
    read_pmpaddr7,
    read_pmpaddr8,
    read_pmpaddr9,
    read_pmpaddr10,
    read_pmpaddr11,
    read_pmpaddr12,
    read_pmpaddr13,
    read_pmpaddr14,
    read_pmpaddr15,
};

uint32_t read_pmpaddr(unsigned int reg_num) {
    assert(reg_num < 16);
    return read_pmpaddr_list[reg_num]() << 2;
}


write_pmpaddr_macro(0)
write_pmpaddr_macro(1)
write_pmpaddr_macro(2)
write_pmpaddr_macro(3)
write_pmpaddr_macro(4)
write_pmpaddr_macro(5)
write_pmpaddr_macro(6)
write_pmpaddr_macro(7)
write_pmpaddr_macro(8)
write_pmpaddr_macro(9)
write_pmpaddr_macro(10)
write_pmpaddr_macro(11)
write_pmpaddr_macro(12)
write_pmpaddr_macro(13)
write_pmpaddr_macro(14)
write_pmpaddr_macro(15)

void (*write_pmpaddr_list[16])(uint32_t) = {
    write_pmpaddr0,
    write_pmpaddr1,
    write_pmpaddr2,
    write_pmpaddr3,
    write_pmpaddr4,
    write_pmpaddr5,
    write_pmpaddr6,
    write_pmpaddr7,
    write_pmpaddr8,
    write_pmpaddr9,
    write_pmpaddr10,
    write_pmpaddr11,
    write_pmpaddr12,
    write_pmpaddr13,
    write_pmpaddr14,
    write_pmpaddr15,
};

void write_pmpaddr(unsigned int reg_num, uint32_t addr) {
    assert(reg_num < 16);
    write_pmpaddr_list[reg_num](addr >> 2);
}

uint8_t read_pmpXcfg(int X) {
	/* e.g. getting pmp13cfg, X = 13; X/4 = 4; Select pmpcfg0 + 4; */
	uint32_t value = read_pmpcfg(X / 4);

	/* In pmpcfg4, bits 0..7 are pmp12cfg, 7..15 are pmp13cfg */
	/* 13 % 4 = 1; 1 * 8 = 8; shifting 8 bits to right */
	value = value >> (X % 4) * 8;
	return value & 0xFF;
}

uint32_t _NAPOT_base(uint32_t addr) {
    addr >>= 2;
    uint32_t mask = 1;
    while (addr & mask) {
        addr &= ~mask;
        mask <<= 1;
    }
    addr <<= 2;
    return addr;
}

uint32_t _NAPOT_range(uint32_t addr) {
    uint32_t mask = 1;
    addr >>= 2;
    while (addr & mask) {
        mask <<= 1;
    }
    return mask * 8;
}

void print_pmpXcfg(int reg_num) {
    uint8_t cfg = read_pmpXcfg(reg_num);
    uint32_t start = 0;
    uint32_t stop = 0;
    char *mode = "OFF  ";
    switch (cfg & PMP_A) {
        case PMP_TOR:
            mode = "TOR  ";
            start = (reg_num > 0) ? read_pmpaddr(reg_num - 1) : 0;
            stop = read_pmpaddr(reg_num);
            break;
        case PMP_NA4:
            mode = "NA4  ";
            start = read_pmpaddr(reg_num);
            stop = start + 3;
            break;
        case PMP_NAPOT:
            mode = "NAPOT";
            uint32_t _tmp = read_pmpaddr(reg_num);
            start = _NAPOT_base(_tmp);
            stop = start + _NAPOT_range(_tmp);
            break;
    }
    printf("pmp%02dcfg: %c %c%c%c %s 0x%08x - 0x%08x\n",
        reg_num,
        (cfg & PMP_L) ? 'L' : '-',
        (cfg & PMP_R) ? 'R' : '-',
        (cfg & PMP_W) ? 'W' : '-',
        (cfg & PMP_X) ? 'X' : '-',
        mode,
        (unsigned int)start,
        (unsigned int)stop
    );
}

void set_pmpXcfg(int reg_num, uintptr_t addr, uint8_t mode) {
    /* TODO: Assert alignment */
    write_pmpaddr(reg_num, (uint32_t) addr);
    uint32_t _tmp = read_pmpcfg(reg_num / 4);
    _tmp &= ~(0xff << (reg_num % 4) * 8); /* clear */
    _tmp |= (mode << (reg_num % 4) * 8); /* set */
    write_pmpcfg(reg_num / 4, _tmp);
}

uint32_t make_napot(uint32_t addr, uint32_t size) {
    assert((size % 2) == 0);
    assert(size >= 8);
    assert((addr % size) == 0);
    uint32_t mask = size/2;
    addr &= ~size;
    addr |= mask - 1;
    printf("NAPUT %x\n", (unsigned int)addr);
    return addr;
}

