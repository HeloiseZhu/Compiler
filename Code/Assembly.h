#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <assert.h>
#include <stdlib.h>
#include "Translate.h"

typedef struct Reg_ Reg;
typedef struct AsmCode_ AsmCode;

#define reg_0 regs[0]
#define reg_at regs[1]
#define reg_v0 regs[2]
#define reg_v1 regs[3]
#define reg_a0 regs[4]
#define reg_a1 regs[5]
#define reg_a2 regs[6]
#define reg_a3 regs[7]
#define reg_t0 regs[8]
#define reg_t1 regs[9]
#define reg_t2 regs[10]
#define reg_t3 regs[11]
#define reg_t4 regs[12]
#define reg_t5 regs[13]
#define reg_t6 regs[14]
#define reg_t7 regs[15]
#define reg_s0 regs[16]
#define reg_s1 regs[17]
#define reg_s2 regs[18]
#define reg_s3 regs[19]
#define reg_s4 regs[20]
#define reg_s5 regs[21]
#define reg_s6 regs[22]
#define reg_s7 regs[23]
#define reg_t8 regs[24]
#define reg_t9 regs[25]
#define reg_k0 regs[26]
#define reg_k1 regs[27]
#define reg_gp regs[28]
#define reg_sp regs[29]
#define reg_fp regs[30]     // TODO: or reg_s8
#define reg_ra regs[31]

#define LINK_ASM(n1, n2, n3) \
    n1->next = n2; n2->next = n3; 


struct Reg_ {
    // TODO: reg definition
    int reg_no;
};

enum AsmType {
    ASM_LABEL,
    ASM_LI, ASM_LA,
    ASM_MOVE,
    ASM_ADDI,           // TODO: +/- #k
    ASM_ADD, ASM_SUB,
    ASM_MUL, ASM_DIV,   // TODO: [x := y * #7]
    ASM_MFLO,
    ASM_LW, ASM_SW,
    ASM_J, ASM_JAL, ASM_JR,
    ASM_BEQ, ASM_BNE,
    ASM_BGT, ASM_BLT,
    ASM_BGE, ASM_BLE,
    ASM_STORAGE, ASM_SYSCALL
};

enum StorageType {
    ST_ASCII, ST_ASCIIZ,
    ST_BYTE, ST_HALF, ST_WORF,
    ST_SPACE
};

// TODO: notice Pseudo Instruction
struct AsmCode_ {
    enum AsmType kind;
    union {
        char* label;    // label & j
        Reg* reg;       // mflo & jr
        char* func;     // jal
        struct { Reg *reg; int imm; } li;               // li
        struct { Reg *reg; char* addr; } la;            // la TODO: addr is?
        struct { Reg *rd, *rs; } move;                  // move
        struct { Reg *rd, *rs; int imm; } addi;         // addi
        struct { Reg *rs1, *rs2; } div;                 // div
        struct { Reg *rd, *rs1, *rs2; } binop;          // add & sub & mul
        struct { Reg *rd, *rs; int imm; } lw;           // lw
        struct { Reg *rs, *rd; int imm; } sw;           // sw
        struct { Reg *rs1, *rs2; char* label; } br;     // beq & bne & bgt & blt & bge & ble
        struct {    // name: storage_type value(s)
            char* name;
            enum StorageType kind;
            union {
                char* str;  // .ascii & .asciizs
                // TODO: .byte & .half & .word
                int size;   // .space
            };  // value(s)
        } storage;
    };
    AsmCode* next;
};


void asm_init(ICNode* icnode);
AsmCode* newAsm(enum AsmType type, ...);
AsmCode* asm_link(AsmCode* n1, AsmCode* n2);
AsmCode* get_reg(Operand* op, Reg** reg);
AsmCode* binop2asm(InterCode* code);
AsmCode* ifgoto2asm(InterCode* code);
AsmCode* ir2asm(ICNode* ir);
void printAsmCodes(AsmCode* codes, FILE* file);

#endif // ASSEMBLY_H