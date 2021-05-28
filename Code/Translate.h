#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "InterCode.h"
#include "SemanticAnalysis.h"

typedef struct VarList_ VarList;
struct VarList_ {
    Symbol* symbol;
    Operand* op;
    VarList* next;
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
Operand* newVar(Symbol* varSymbol);
Operand* newTemp();
Operand* newLabel();
ICNode* newNode(enum ICType type, ...);
void changeLabelNo(int target, int val);
Operand* getVarOp(TreeNode* node);
Operand* getVarAddr(char* name);
char* getRelop(char* relop);
ICNode* link(ICNode* n1, ICNode* n2);
ICNode* detailedAssign(DataType* type, Operand* laddr, Operand* raddr, int offset);
ICNode* handleAssign(char* left, char* right);

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
ICNode* translateCond(TreeNode* node, Operand* label_true, Operand* label_false, bool mod);
ICNode* translateLeftExp(TreeNode* node, Operand* place, DataType** param);
ICNode* translateExp(TreeNode* node, Operand* place, DataType** param);
ICNode* translateArgs(TreeNode* node, ArgList** argList);

void printTranslateError(char* msg);
char* translateOperand(Operand* op);
void printInterCodes(ICNode* head, FILE* file);
ICNode* optimize(ICNode* icnode);

#endif // TRANSLATE_H