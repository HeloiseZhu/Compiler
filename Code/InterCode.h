#ifndef INTERCODE_H
#define INTERCODE_H

typedef struct Operand_ Operand;
typedef struct InterCode_ InterCode;
typedef struct InterCodeNode_ ICNode;
typedef struct ArgList_ ArgList;
typedef struct ConstList_ ConstList;
typedef struct LabelList_ LabelList;

struct Operand_ {
    enum { 
        OP_VAR, OP_VAR_ADDR, OP_VAR_MEM, 
        OP_TEMP, OP_TEMP_ADDR, OP_TEMP_MEM, 
        OP_CONST, OP_LABEL
    } kind;
    union {
        int label_no;   // LABEL 
        int var_no;     // TEMP & VAR
        int val;        // CONST
    };
};

enum ICType { 
    IC_LABEL, IC_FUNC, 
    IC_ASSIGN, IC_ADD, IC_SUB, IC_MUL, IC_DIV, 
    IC_GOTO, IC_IF_GOTO, IC_RETURN, 
    IC_DEC, IC_ARG, IC_CALL_FUNC, IC_PARAM, 
    IC_READ, IC_WRITE
};

struct InterCode_ {
    enum ICType kind;
    union {
        Operand* op;    // LABEL & GOTO & RETURN & ARG & PARAM & READ & WRITE
        char* func;     // FUNC
        struct { Operand *left, *right; } assign; // ASSIGN & ADDR & MEM_R & MEM_L
        struct { Operand *result, *op1, *op2; } binop;  // ADD & SUB & MUL & DIV
        struct { Operand *op1, *op2, *label; char* relop; } ifgoto;  // IF_GOTO
        struct { Operand *op; int size; } dec;  // DEC
        struct { Operand *result; char* func; } call;
    };
};

struct InterCodeNode_ { 
    InterCode* code;
    ICNode *prev, *next; 
};

struct ArgList_ {
    Operand* arg;
    ArgList* next;
};

struct LabelList_ {
    Operand* label; // [tmp := #cst]
    LabelList* next;
};

#endif // INTERCODE_H