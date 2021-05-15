#ifndef INTERCODE_H
#define INTERCODE_H

#include "SemanticAnalysis.h"
#include <stdarg.h>

typedef struct Operand_ Operand;
typedef struct InterCode_ InterCode;
typedef struct InterCodeNode_ ICNode;
typedef struct ArgList_ ArgList;

struct Operand_ {
    enum { 
        OP_VAR, OP_TEMP, OP_CONST, 
        OP_VAR_ADDR, OP_TEMP_ADDR, OP_VAR_MEM, OP_TEMP_MEM, 
        OP_LABEL, OP_RELOP
    } kind;
    union {
        int label_no;   // LABEL 
        int var_no;     // TEMP & VAR
        int val;        // CONST
        char* relop;    // RELOP
    };
};

enum ICType { 
    IC_LABEL, IC_FUNC, 
    IC_ASSIGN, IC_ADD, IC_SUB, IC_MUL, IC_DIV, 
    /* IC_ADDR, IC_MEM_R, IC_MEM_L, */
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
        struct { Operand *relop, *op1, *op2, *label; } ifgoto;  // IF_GOTO
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


#define SET_OPERAND(op, k) \
    op = (Operand*)malloc(sizeof(Operand)); \
    op->kind = k;
#define SET_INTERCODE(code, k) \
    code = (InterCode*)malloc(sizeof(InterCode)); \
    code->kind = k;
#define SET_ICNODE(node) \
    node = (ICNode*)malloc(sizeof(ICNode)); \
    node->code = (InterCode*)malloc(sizeof(InterCode)); \
    node->prev = NULL; \
    node->next = NULL;
#define LINK_ICNODE(n1, n2, n3) \
    n1->next = n2; \
    n2->next = n3; \
    n2->prev = n1; \
    n3->prev = n2;


void initIC();
Operand* newVar();
Operand* newTemp();
Operand* newLabel();
ICNode* newNode(enum ICType type, ...);
ICNode* link(ICNode* n1, ICNode* n2);
ICNode* placeAssign(Operand* place, Operand* right);
DataType* getExpType(TreeNode* node);

ICNode* translateProgram(TreeNode* node);
ICNode* translateExtDefList(TreeNode* node);
ICNode* translateExtDef(TreeNode* node);
ICNode* translateVarDec(TreeNode* node);
ICNode* translateFunDec(TreeNode* node);
ICNode* translateCompSt(TreeNode* node);
ICNode* translateStmtList(TreeNode* node);
ICNode* translateStmt(TreeNode* node);
ICNode* translateDefList(TreeNode* node);
ICNode* translateDef(TreeNode* node);
ICNode* translateDecList(TreeNode* node);
ICNode* translateDec(TreeNode* node);
ICNode* translateCond(TreeNode* node, Operand* label_true, Operand* label_false);
ICNode* translateLeftExp(TreeNode* node, Operand* place, DataType** param);
ICNode* translateExp(TreeNode* node, Operand* place, DataType** param);
ICNode* translateArgs(TreeNode* node, ArgList** argList);

void printTranslateError(char* msg);
char* translateOperand(Operand* op);
void printInterCodes(ICNode* head);

#endif // INTERCODE_H