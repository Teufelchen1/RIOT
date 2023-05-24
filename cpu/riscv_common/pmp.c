#include <stdio.h>
#include <stdint.h>
#include "vendor/pmp.h"

uint32_t read_pmpcfgX(int X) {
    switch (X) {
        case 0:
            return read_csr(pmpcfg0 + 0);
        case 1:
            return read_csr(pmpcfg0 + 1);
        case 2:
            return read_csr(pmpcfg0 + 2);
        case 3:
            return read_csr(pmpcfg0 + 3);
        case 4:
            return read_csr(pmpcfg0 + 4);
        case 5:
            return read_csr(pmpcfg0 + 5);
        case 6:
            return read_csr(pmpcfg0 + 6);
        case 7:
            return read_csr(pmpcfg0 + 7);
        case 8:
            return read_csr(pmpcfg0 + 8);
        case 9:
            return read_csr(pmpcfg0 + 9);
        case 10:
            return read_csr(pmpcfg0 + 10);
        case 11:
            return read_csr(pmpcfg0 + 11);
        case 12:
            return read_csr(pmpcfg0 + 12);
        case 13:
            return read_csr(pmpcfg0 + 13);
        case 14:
            return read_csr(pmpcfg0 + 14);
        case 15:
            return read_csr(pmpcfg0 + 15);
    }
    return 0;
}

uint8_t read_pmpXcfg(int X) {
	/* e.g. getting pmp13cfg, X = 13; X/4 = 4; Select pmpcfg0 + 4; */
	uint32_t value = read_pmpcfgX(X / 4);

	/* In pmpcfg4, bits 0..7 are pmp12cfg, 7..15 are pmp13cfg */
	/* 13 % 4 = 1; 1 * 8 = 8; shifting 8 bits to right */
	value = value >> (X % 4) * 8;
	return value & 0xFF;
}

uint32_t read_pmpaddrX(int X) {
    switch (X) {
        case 0:
            return read_csr(pmpaddr0 + 0);
        case 1:
            return read_csr(pmpaddr0 + 1);
        case 2:
            return read_csr(pmpaddr0 + 2);
        case 3:
            return read_csr(pmpaddr0 + 3);
        case 4:
            return read_csr(pmpaddr0 + 4);
        case 5:
            return read_csr(pmpaddr0 + 5);
        case 6:
            return read_csr(pmpaddr0 + 6);
        case 7:
            return read_csr(pmpaddr0 + 7);
        case 8:
            return read_csr(pmpaddr0 + 8);
        case 9:
            return read_csr(pmpaddr0 + 9);
        case 10:
            return read_csr(pmpaddr0 + 10);
        case 11:
            return read_csr(pmpaddr0 + 11);
        case 12:
            return read_csr(pmpaddr0 + 12);
        case 13:
            return read_csr(pmpaddr0 + 13);
        case 14:
            return read_csr(pmpaddr0 + 14);
        case 15:
            return read_csr(pmpaddr0 + 15);
    }
    return 0;
}

uint32_t get_NAPOT_baseaddr(uint32_t reg) {
    (void) reg;
    return 0;
}

uint32_t get_NAPOT_range(uint32_t reg) {
    (void) reg;
    return 0;
}

void print_pmpXcfg(int X) {
    uint32_t start_addr = 0;
    uint32_t stop_addr = 0;
	uint8_t cfg = read_pmpXcfg(X);
	if (cfg & PMP_L) {
		printf("L ");
	} else {
		printf("- ");
	}
	switch (cfg & PMP_A) {
        case PMP_OFF:
            printf("OFF   ");
            break;
        case PMP_TOR:
            printf("TOR   ");
            stop_addr = read_pmpaddrX(X);
            start_addr = read_pmpaddrX(X-1);
            break;
        case PMP_NA4:
            printf("NA4   ");
            start_addr = read_pmpaddrX(X);
            stop_addr = start_addr + 4;
            break;
        case PMP_NAPOT:
            printf("NAPOT ");
            start_addr = get_NAPOT_baseaddr(read_pmpaddrX(X));
            stop_addr = get_NAPOT_range(read_pmpaddrX(X));
            break;
        default:
            printf("???   ");
	}
	if (cfg & PMP_X) {
		printf("X");
	} else {
		printf("-");
	}
	if (cfg & PMP_W) {
		printf("W");
	} else {
		printf("-");
	}
	if (cfg & PMP_R) {
		printf("R");
	} else {
		printf("-");
	}
    printf("0x%lX - 0x%lX", start_addr, stop_addr);
	printf("\n");
}


