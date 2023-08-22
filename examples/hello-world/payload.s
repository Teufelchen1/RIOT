start:
    .ascii "coap://[::1]:12345678"
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
    addi x17,x17,100
loop:
    lui a0, 0x80000
    addi a0, a0, 0x750
    lui x16, 0x2001a
    jalr ra, -2(x16)
    jal x0, loop
    .word 0x80000680
    .ascii "Hacked!"
    .ascii "/test/"
    .byte 0xA