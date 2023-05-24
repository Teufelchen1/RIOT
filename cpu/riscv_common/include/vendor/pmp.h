#include "riscv_csr.h"

/* see riscv-privileged-20211203.pdf, 2.2 */
#define pmpcfg0 	0x3A0 /* pmp0cfg - pmp3cfg */
#define pmpcfg1 	0x3A1 /* pmp4cfg - pmp7cfg */
#define pmpcfg2 	0x3A2
#define pmpcfg3 	0x3A3
#define pmpcfg4 	0x3A4
#define pmpcfg5 	0x3A5
#define pmpcfg6 	0x3A6
#define pmpcfg7 	0x3A7
#define pmpcfg8 	0x3A8
#define pmpcfg9 	0x3A9
#define pmpcfg10 	0x3AA
#define pmpcfg11 	0x3AB
#define pmpcfg12 	0x3AC
#define pmpcfg13 	0x3AD
#define pmpcfg14 	0x3AE
#define pmpcfg15 	0x3AF

#define pmpaddr0	(0x3B0)
#define pmpaddr1	0x3B1
#define pmpaddr2	0x3B2
#define pmpaddr3	0x3B3
#define pmpaddr4	0x3B4
#define pmpaddr5	0x3B5
#define pmpaddr6	0x3B6
#define pmpaddr7	0x3B7
#define pmpaddr8	0x3B8
#define pmpaddr9	0x3B9
#define pmpaddr10	0x3BA
#define pmpaddr11	0x3BB
#define pmpaddr12	0x3BC
#define pmpaddr13	0x3BD
#define pmpaddr14	0x3BE
#define pmpaddr15	0x3BF
#define pmpaddr16	0x3C0
#define pmpaddr17	0x3C1
#define pmpaddr18	0x3C2
#define pmpaddr19	0x3C3
#define pmpaddr20	0x3C4
#define pmpaddr21	0x3C5
#define pmpaddr22	0x3C6
#define pmpaddr23	0x3C7
#define pmpaddr24	0x3C8
#define pmpaddr25	0x3C9
#define pmpaddr26	0x3CA
#define pmpaddr27	0x3CB
#define pmpaddr28	0x3CC
#define pmpaddr29	0x3CD
#define pmpaddr30	0x3CE
#define pmpaddr31	0x3CF
#define pmpaddr32	0x3D0
#define pmpaddr33	0x3D1
#define pmpaddr34	0x3D2
#define pmpaddr35	0x3D3
#define pmpaddr36	0x3D4
#define pmpaddr37	0x3D5
#define pmpaddr38	0x3D6
#define pmpaddr39	0x3D7
#define pmpaddr40	0x3D8
#define pmpaddr41	0x3D9
#define pmpaddr42	0x3DA
#define pmpaddr43	0x3DB
#define pmpaddr44	0x3DC
#define pmpaddr45	0x3DD
#define pmpaddr46	0x3DE
#define pmpaddr47	0x3DF
#define pmpaddr48	0x3E0
#define pmpaddr49	0x3E1
#define pmpaddr50	0x3E2
#define pmpaddr51	0x3E3
#define pmpaddr52	0x3E4
#define pmpaddr53	0x3E5
#define pmpaddr54	0x3E6
#define pmpaddr55	0x3E7
#define pmpaddr56	0x3E8
#define pmpaddr57	0x3E9
#define pmpaddr58	0x3EA
#define pmpaddr59	0x3EB
#define pmpaddr60	0x3EC
#define pmpaddr61	0x3ED
#define pmpaddr62	0x3EE
#define pmpaddr63	0x3EF

#define PMP_NONE	0x00
#define PMP_R		0x01
#define PMP_W		0x02
#define PMP_X		0x04

#define PMP_A 		0x18
#define PMP_OFF		0x00
#define PMP_TOR		0x08
#define PMP_NA4		0x10
#define PMP_NAPOT	0x18

#define PMP_L 		0x80

uint8_t read_pmpXcfg(int X);

uint32_t read_pmpaddrX(int X);

void print_pmpXcfg(int X);

void set_pmpXcfg(int X, uint32_t *addr, uint32_t size, uint8_t mode);


