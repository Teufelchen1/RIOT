#include "vendor/pmp.h"

uint8_t read_pmpXcfg(int X) {
	/* e.g. getting pmp13cfg, X = 13; X/4 = 4; Select pmpcfg0 + 4; */
	uint32_t value = read_csr(pmpcfg0 + X / 4);

	/* In pmpcfg4, bits 0..7 are pmp12cfg, 7..15 are pmp13cfg */
	/* 13 % 4 = 1; 1 * 8 = 8; shifting 8 bits to right */
	value = value >> (X % 4) * 8;
	return value & 0xFF;
}

uint32_t read_pmpaddrX(int X) {
	return read_csr(pmpaddr0 + X);
}

void print_pmpXcfg(int X) {
	uint8_t cfg = read_pmpXcfg(X);
	if (cfg & PMP_L) {
		printf("L ");
	} else {
		printf("- ");
	}
	switch ((cfg & PMP_A) >> 3) {
	case PMP_OFF:
		printf("OFF   ");
		break;
	case PMP_TOR:
		printf("TOR   ");
		break;
	case PMP_NA4:
		printf("NA4   ");
		break;
	case PMP_NAPOT:
		printf("NAPOT ");
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
	printf("\n");
}


