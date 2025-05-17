#pragma once

#define RAX 0x00
#define RBX 0x01
#define RCX 0x02
#define RDX 0x03
#define RSP 0x04
#define RBP 0x05
#define RSI 0x06
#define RDI 0x07


#define mRAX 0b000
#define mRCX 0b001
#define mRDX 0b010
#define mRBX 0b011
#define mRSP 0b100
#define mRBP 0b101
#define mRSI 0b110
#define mRDI 0b111


#define PUSHR 0x50
#define POPR 0x58

#define JMP 0xE9
#define JZ  0x0F84
#define JL  0x0F8C

#define RET 0xC3

#define CALL 0xE8

#define JMP_SIZE 5
#define JZ_SIZE 6
#define PUSH_NUM_SIZE 5
#define JL_SIZE 6
#define CALL_SIZE 5
