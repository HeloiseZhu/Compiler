#include "Assembly.h"

Reg* regs[32];
AsmCode* initAsmCodes = NULL;


void asm_init(ICNode* icnode) {
    // Regs
    for(int i = 0; i < 32; i++) {
        // TODO: init regs
        regs[i] = (Reg*)malloc(sizeof(Reg));
        regs[i]->reg_no = i;
    }
    
    AsmCode* l_data = newAsm(ASM_LABEL, ".data");
    AsmCode* st1 = newAsm(ASM_STORAGE, "_prompt", ST_ASCIIZ, "Enter an integer:");
    AsmCode* st2 = newAsm(ASM_STORAGE, "_ret", ST_ASCIIZ, "\\n");   // TODO: test
    LINK_ASM(l_data, st1, st2)
    // DEC
    AsmCode* tail = st2;
    ICNode* node = icnode;
    while(node) {
        if(node->code->kind == IC_DEC) {
            // array2: .space 40
            AsmCode* st = newAsm(ASM_STORAGE, translateOperand(node->code->dec.op), ST_SPACE, node->code->dec.size);
            tail->next = st;
            tail = st;
        }
        node = node->next;
    }

    AsmCode* l_main = newAsm(ASM_LABEL, ".globl main");
    AsmCode* l_text = newAsm(ASM_LABEL, ".text");
    l_main->next = l_text;

    AsmCode* l_read = newAsm(ASM_LABEL, "read");
    AsmCode* r_c1 = newAsm(ASM_LI, reg_v0, 4);
    AsmCode* r_c2 = newAsm(ASM_LA, reg_a0, "_prompt");
    AsmCode* r_c3 = newAsm(ASM_SYSCALL);
    AsmCode* r_c4 = newAsm(ASM_LI, reg_v0, 5);
    AsmCode* r_c5 = newAsm(ASM_SYSCALL);
    AsmCode* r_c6 = newAsm(ASM_JR, reg_ra);
    LINK_ASM(l_read, r_c1, r_c2) LINK_ASM(r_c2, r_c3, r_c4) LINK_ASM(r_c4, r_c5, r_c6)

    AsmCode* l_write = newAsm(ASM_LABEL, "write");
    AsmCode* w_c1 = newAsm(ASM_LI, reg_v0, 1);
    AsmCode* w_c2 = newAsm(ASM_SYSCALL);
    AsmCode* w_c3 = newAsm(ASM_LI, reg_v0, 4);
    AsmCode* w_c4 = newAsm(ASM_LA, reg_a0, "_ret");
    AsmCode* w_c5 = newAsm(ASM_SYSCALL);
    AsmCode* w_c6 = newAsm(ASM_MOVE, reg_v0, reg_0);
    AsmCode* w_c7 = newAsm(ASM_JR, reg_ra);
    LINK_ASM(l_write, w_c1, w_c2) LINK_ASM(w_c2, w_c3, w_c4) LINK_ASM(w_c4, w_c5, w_c6) w_c6->next = w_c7;

    initAsmCodes = asm_link(asm_link(l_data, l_main), asm_link(l_read, l_write));
}

AsmCode* newAsm(enum AsmType type, ...) {
    // TODO: [!] new AsmCode
    return NULL;
}

AsmCode* asm_link(AsmCode* n1, AsmCode* n2) {
    if(n1 == NULL) return n2;
    if(n2 == NULL) return n1;
    AsmCode* cur = n1;
    while(cur->next) { cur = cur->next; }
    cur->next = n2;
    return n1;
}

AsmCode* get_reg(Operand* op, Reg** reg) {
    // TODO: [!] allocate regs for var
    return NULL;
}

AsmCode* binop2asm(InterCode* code) {
    if(!code) return NULL;
    Reg *l_reg = NULL, *op1_reg = NULL, *op2_reg = NULL;
    AsmCode* code1 = get_reg(code->binop.result, &l_reg);
    AsmCode* code2 = get_reg(code->binop.op1, &op1_reg);
    AsmCode* code3 = get_reg(code->binop.op2, &op2_reg);
    AsmCode* code4 = NULL;
    switch (code->kind) {
    case IC_ADD: code4 = newAsm(ASM_ADD, l_reg, op1_reg, op2_reg); break;
    case IC_SUB: code4 = newAsm(ASM_SUB, l_reg, op1_reg, op2_reg); break;
    case IC_MUL: code4 = newAsm(ASM_MUL, l_reg, op1_reg, op2_reg); break;
    default: assert(0); break;
    }
    return asm_link(asm_link(code1, code2), asm_link(code3, code4));
}

AsmCode* ifgoto2asm(InterCode* code) {
    if(!code || code->kind != IC_IF_GOTO) return NULL;
    Reg *op1_reg = NULL, *op2_reg = NULL;
    AsmCode* code1 = get_reg(code->ifgoto.op1, &op1_reg);
    AsmCode* code2 = get_reg(code->ifgoto.op2, &op2_reg);
    AsmCode* code3 = NULL;
    char* label = translateOperand(code->ifgoto.label);
    if(strcmp(code->ifgoto.relop, "==") == 0) {
        code3 = newAsm(ASM_BEQ, op1_reg, op2_reg, label);
    } else if(strcmp(code->ifgoto.relop, "!=") == 0) {
        code3 = newAsm(ASM_BNE, op1_reg, op2_reg, label);
    } else if(strcmp(code->ifgoto.relop, ">") == 0) {
        code3 = newAsm(ASM_BGT, op1_reg, op2_reg, label);
    } else if(strcmp(code->ifgoto.relop, "<") == 0) {
        code3 = newAsm(ASM_BLT, op1_reg, op2_reg, label);
    } else if(strcmp(code->ifgoto.relop, ">=") == 0) {
        code3 = newAsm(ASM_BGE, op1_reg, op2_reg, label);
    } else if(strcmp(code->ifgoto.relop, "<=") == 0) {
        code3 = newAsm(ASM_BLE, op1_reg, op2_reg, label);
    } else assert(0);
    return asm_link(code1, asm_link(code2, code3));
}

AsmCode* ir2asm(ICNode* icnode) {
    AsmCode *head = NULL, *tail = NULL;
    ICNode* node = icnode;
    InterCode* code = NULL;

    asm_init(icnode);
    // TODO: link init asm codes
    
    while(node) {
        code = node->code;
        AsmCode* new_asm = NULL;

        switch (code->kind) {
        case IC_LABEL: new_asm = newAsm(ASM_LABEL, translateOperand(code->op)); break;
        case IC_FUNC: new_asm = newAsm(ASM_LABEL, code->func); break;
        case IC_ASSIGN:
            if(code->assign.left->kind == OP_VAR || code->assign.left->kind == OP_TEMP) {
                //code->assign [x := y / &y / *y / #k]
                if(code->assign.right->kind == OP_VAR || code->assign.right->kind == OP_TEMP) {
                    Reg *l_reg = NULL, *r_reg = NULL;
                    AsmCode* code1 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code2 = get_reg(code->assign.right, &r_reg);
                    AsmCode* code3 = newAsm(ASM_MOVE, l_reg, r_reg);
                    new_asm = asm_link(code1, asm_link(code2, code3));
                }
                else if(code->assign.right->kind == OP_VAR_ADDR) {
                    // TODO: [x := &y]
                    Reg *l_reg = NULL, *r_reg = NULL;
                    AsmCode* code1 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code2 = get_reg(code->assign.right, &r_reg);
                    AsmCode* code3 = newAsm(ASM_MOVE, l_reg, r_reg);
                    new_asm = asm_link(code1, asm_link(code2, code3));
                }
                else if(code->assign.right->kind == OP_VAR_MEM || code->assign.right->kind == OP_TEMP_MEM) {
                    // [x := *y]
                    Reg *l_reg = NULL, *r_reg = NULL;
                    AsmCode* code1 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code2 = get_reg(code->assign.right, &r_reg);
                    AsmCode* code3 = newAsm(ASM_LW, l_reg, r_reg, 0);
                    new_asm = asm_link(code1, asm_link(code2, code3));
                }
                else if(code->assign.right->kind == OP_CONST) {
                    Reg* l_reg = NULL;
                    AsmCode* code1 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code2 = newAsm(ASM_LI, l_reg, code->assign.right->val);
                    new_asm = asm_link(code1, code2);
                }
                else assert(0);
            } 
            else if(code->assign.left->kind == OP_VAR_MEM || code->assign.left->kind == OP_TEMP_MEM) {
                // [*x := y / *y / #k]
                if(code->assign.right->kind == OP_VAR || code->assign.right->kind == OP_TEMP) {
                    Reg *l_reg = NULL, *r_reg = NULL;
                    AsmCode* code1 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code2 = get_reg(code->assign.right, &r_reg);
                    AsmCode* code3 = newAsm(ASM_SW, r_reg, l_reg, 0);
                    new_asm = asm_link(code1, asm_link(code2, code3));
                }
                else if(code->assign.right->kind == OP_VAR_MEM || code->assign.right->kind == OP_TEMP_MEM) {
                    // [*x := *y] => [t := *y] + [*x := t] 
                    Reg *l_reg = NULL, *r_reg = NULL, *t_reg = NULL;
                    AsmCode* code1 = get_reg(NULL, &t_reg);
                    AsmCode* code2 = get_reg(code->assign.right, &r_reg);
                    AsmCode* code3 = newAsm(ASM_LW, t_reg, r_reg, 0);
                    AsmCode* code4 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code5 = newAsm(ASM_SW, t_reg, l_reg, 0);
                    code1 = asm_link(code1, asm_link(code2, code3));
                    new_asm = asm_link(code1, asm_link(code4, code5));
                }
                else if(code->assign.right->kind == OP_CONST) {
                    // [*x := #k] => [t := #k] + [*x := t] 
                    Reg *l_reg = NULL, *t_reg = NULL;
                    AsmCode* code1 = get_reg(NULL, &t_reg);
                    AsmCode* code2 = newAsm(ASM_LI, t_reg, code->assign.right->val);
                    AsmCode* code3 = get_reg(code->assign.left, &l_reg);
                    AsmCode* code4 = newAsm(ASM_SW, t_reg, l_reg, 0);
                    new_asm = asm_link(asm_link(code1, code2), asm_link(code3, code4));
                }
                else assert(0);
            }
            else assert(0);
            break;
        case IC_ADD:
            if(code->binop.op1->kind == OP_CONST && code->binop.op2->kind == OP_CONST) {
                Reg *l_reg = NULL, *op1_reg = NULL;
                AsmCode* code1 = get_reg(code->binop.result, &l_reg);
                AsmCode* code2 = get_reg(code->binop.op1, &op1_reg);
                AsmCode* code3 = newAsm(ASM_ADDI, l_reg, op1_reg, code->binop.op2->val);
                new_asm = asm_link(code1, asm_link(code2, code3));
            }
            else if(code->binop.op1->kind == OP_CONST || code->binop.op2->kind == OP_CONST) {
                Reg *l_reg = NULL, *op_reg = NULL;
                AsmCode *code1 = NULL, *code2 = NULL, *code3 = NULL;
                code2 = get_reg(code->binop.result, &l_reg);
                if(code->binop.op1->kind == OP_CONST) {
                    code1 = get_reg(code->binop.op1, &op_reg);
                    code3 = newAsm(ASM_ADDI, l_reg, op_reg, code->binop.op2->val);
                } else {
                    code1 = get_reg(code->binop.op2, &op_reg);
                    code3 = newAsm(ASM_ADDI, l_reg, op_reg, code->binop.op1->val);
                }
                new_asm = asm_link(code1, asm_link(code2, code3));
            }
            else new_asm = binop2asm(code);
            break;
        case IC_SUB:
            if(code->binop.op2->kind == OP_CONST) {
                // addi reg(x), reg(y), -k
                Reg *l_reg = NULL, *op1_reg = NULL;
                AsmCode* code1 = get_reg(code->binop.result, &l_reg);
                AsmCode* code2 = get_reg(code->binop.op1, &op1_reg);
                AsmCode* code3 = newAsm(ASM_ADDI, l_reg, op1_reg, -code->binop.op2->val);
                new_asm = asm_link(code1, asm_link(code2, code3));
            }
            else new_asm = binop2asm(code);
            break;
        case IC_MUL: new_asm = binop2asm(code); break;
        case IC_DIV: {  // div reg(y), reg(z) + mflo reg(x)
            Reg *l_reg = NULL, *op1_reg = NULL, *op2_reg = NULL;
            AsmCode* code1 = get_reg(code->binop.result, &l_reg);
            AsmCode* code2 = get_reg(code->binop.op1, &op1_reg);
            AsmCode* code3 = get_reg(code->binop.op2, &op2_reg);
            AsmCode* code4 = newAsm(ASM_DIV, op1_reg, op2_reg);
            AsmCode* code5 = newAsm(ASM_MFLO, l_reg);
            code4->next = code5;
            new_asm = asm_link(asm_link(code1, code2), asm_link(code3, code4));
            break;
        }
        case IC_GOTO: new_asm = newAsm(ASM_J, translateOperand(code->op)); break;
        case IC_IF_GOTO: new_asm = ifgoto2asm(code); break;
        case IC_RETURN: {
            Reg* op_reg = NULL;
            AsmCode* code1 = get_reg(code->op, &op_reg);
            AsmCode* code2 = newAsm(ASM_MOVE, reg_v0, op_reg);
            AsmCode* code3 = newAsm(ASM_JR, reg_ra);
            new_asm = asm_link(code1, asm_link(code2, code3));
            break;
        }
        case IC_DEC: break;
        case IC_ARG:
            // TODO
            break;
        case IC_CALL_FUNC:
            // TODO
            break;
        case IC_PARAM:
            // TODO
            break;
        case IC_READ:
            // TODO
            break;
        case IC_WRITE:
            // TODO
            break;
        default:
            break;
        }

        // link
        if(head == NULL) { head = new_asm; tail = new_asm; }
        asm_link(tail, new_asm); tail = new_asm;

        node = node->next;
    }
    return head;
}

char* trReg(Reg* reg) {
    // TODO
    return NULL;
}

void printAsmCodes(AsmCode* codes, FILE* file) {
    AsmCode* cur_asm = codes;
    while(cur_asm) {
        switch(cur_asm->kind) {
        case ASM_LABEL: fprintf(file, "%s:\n", cur_asm->label); break;
        case ASM_LI: fprintf(file, "\tli %s, %d\n", trReg(cur_asm->li.reg), cur_asm->li.imm); break;
        case ASM_LA: fprintf(file, "\tla %s, %s\n", trReg(cur_asm->la.reg), cur_asm->la.addr); break;   // TODO
        case ASM_MOVE: fprintf(file, "\tmove %s, %s\n", trReg(cur_asm->move.rd), trReg(cur_asm->move.rs)); break;
        case ASM_ADDI: fprintf(file, "\taddi %s, %s, %d\n", trReg(cur_asm->addi.rd), trReg(cur_asm->addi.rs), cur_asm->addi.imm); break;
        case ASM_ADD: fprintf(file, "\tadd %s, %s, %s\n", trReg(cur_asm->binop.rd), trReg(cur_asm->binop.rs1), trReg(cur_asm->binop.rs2)); break;
        case ASM_SUB: fprintf(file, "\tsub %s, %s, %s\n", trReg(cur_asm->binop.rd), trReg(cur_asm->binop.rs1), trReg(cur_asm->binop.rs2)); break;
        case ASM_MUL: fprintf(file, "\tadd %s, %s, %s\n", trReg(cur_asm->binop.rd), trReg(cur_asm->binop.rs1), trReg(cur_asm->binop.rs2)); break;
        case ASM_DIV: fprintf(file, "\tdiv %s, %s\n", trReg(cur_asm->div.rs1), trReg(cur_asm->div.rs2)); break;
        case ASM_MFLO: fprintf(file, "\tmflo %s\n", trReg(cur_asm->reg)); break;
        case ASM_LW: fprintf(file, "\tlw %s, %d(%s)\n", trReg(cur_asm->lw.rd), cur_asm->lw.imm, trReg(cur_asm->lw.rs)); break;
        case ASM_SW: fprintf(file, "\tsw %s, %d(%s)\n", trReg(cur_asm->sw.rs), cur_asm->sw.imm, trReg(cur_asm->sw.rd)); break;
        case ASM_J: fprintf(file, "\tj %s\n", cur_asm->label); break;
        case ASM_JAL: fprintf(file, "\tjal %s\n", cur_asm->func); break;
        case ASM_JR: fprintf(file, "\tjr %s\n", trReg(cur_asm->reg)); break;
        case ASM_BEQ: fprintf(file, "\tbeq %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_BNE: fprintf(file, "\tbne %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_BGT: fprintf(file, "\tbgt %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_BLT: fprintf(file, "\tblt %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_BGE: fprintf(file, "\tbge %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_BLE: fprintf(file, "\tble %s, %s, %s\n", trReg(cur_asm->br.rs1), trReg(cur_asm->br.rs2), cur_asm->br.label); break;
        case ASM_STORAGE: 
            if(cur_asm->storage.kind == ST_ASCII) {
                fprintf(file, "%s: .ascii %s\n", cur_asm->storage.name, cur_asm->storage.str); 
            } else if(cur_asm->storage.kind == ST_ASCIIZ) {
                fprintf(file, "%s: .asciiz %s\n", cur_asm->storage.name, cur_asm->storage.str); 
            } else if(cur_asm->storage.kind == ST_SPACE) {
                fprintf(file, "%s: .space %d\n", cur_asm->storage.name, cur_asm->storage.size); 
            } else assert(0);   // TODO: ST_BYTE, ST_HALF, ST_WORF
            break;
        case ASM_SYSCALL: fprintf(file, "\tsyscall\n"); break;
        default: break;
        }
        cur_asm = cur_asm->next;
    }
}