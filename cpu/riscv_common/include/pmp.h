#ifndef PMP_H
#define PMP_H

#include <stdint.h>
#include <assert.h>

#define PMP_NONE	0x00 // 0b00000000
#define PMP_R		0x01 // 0b00000001
#define PMP_W		0x02 // 0b00000010
#define PMP_X		0x04 // 0b00000100

#define PMP_A 		0x18 // 0b00011000
#define PMP_OFF		0x00 // 0b00000000
#define PMP_TOR		0x08 // 0b00001000
#define PMP_NA4		0x10 // 0b00010000
#define PMP_NAPOT	0x18 // 0b00011000

#define PMP_L 		0x80 // 0b10000000

#define _STR(REG) #REG

//#define NATIVE 1
#ifdef NATIVE
uint32_t pmp_regs[16];
uint32_t pmp_addr[64];

#define read_pmpcfg_macro(REG) read_pmpcfg_macro_native(REG)
#define write_pmpcfg_macro(REG) write_pmpcfg_macro_native(REG)
#define read_pmpaddr_macro(REG) read_pmpaddr_macro_native(REG)
#define write_pmpaddr_macro(REG) write_pmpaddr_macro_native(REG)

#define read_pmpcfg_macro_native(REG) \
uint32_t read_pmpcfg##REG(void) { \
    unsigned long value; \
    value = pmp_regs[(REG)]; \
    return value; \
}

#define write_pmpcfg_macro_native(REG) \
void write_pmpcfg##REG(uint32_t value) { \
    pmp_regs[(REG)] = value; \
}

#define read_pmpaddr_macro_native(REG) \
uint32_t read_pmpaddr##REG(void) { \
    unsigned long value; \
    value = pmp_addr[(REG)]; \
    return value; \
}

#define write_pmpaddr_macro_native(REG) \
void write_pmpaddr##REG(uint32_t addr) { \
    pmp_addr[(REG)] = addr; \
}

#else

#define read_pmpcfg_macro(REG) read_pmpcfg_macro_riscv(REG)
#define write_pmpcfg_macro(REG) write_pmpcfg_macro_riscv(REG)
#define read_pmpaddr_macro(REG) read_pmpaddr_macro_riscv(REG)
#define write_pmpaddr_macro(REG) write_pmpaddr_macro_riscv(REG)

#define read_pmpcfg_macro_riscv(REG) \
uint32_t read_pmpcfg##REG(void) { \
    unsigned long value; \
    __asm__ volatile ("csrr %0, " _STR(0x3A0 + REG) : "=r"(value)); \
    return value; \
}

#define write_pmpcfg_macro_riscv(REG) \
void write_pmpcfg##REG(uint32_t value) { \
    __asm__ volatile ("csrw " _STR(0x3A0 + REG) ", %0" :: "r"(value)); \
}

#define read_pmpaddr_macro_riscv(REG) \
uint32_t read_pmpaddr##REG(void) { \
    unsigned long value; \
    __asm__ volatile ("csrr %0, " _STR(0x3B0 + REG) : "=r"(value)); \
    return value; \
}

#define write_pmpaddr_macro_riscv(REG) \
void write_pmpaddr##REG(uint32_t addr) { \
    __asm__ volatile ("csrw " _STR(0x3B0 + REG) ", %0" :: "r"(addr)); \
}

#endif

uint32_t read_pmpcfg(unsigned int reg_num);

void write_pmpcfg(unsigned int reg_num, uint32_t value);

uint32_t read_pmpaddr(unsigned int reg_num);

void write_pmpaddr(unsigned int reg_num, uint32_t addr);
#endif

uint8_t read_pmpXcfg(int X);

void print_pmpXcfg(int reg_num);

void set_pmpXcfg(int reg_num, uintptr_t addr, uint8_t mode);

uint32_t make_napot(uint32_t addr, uint32_t size);
