#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "SymbolTable.h"
#include "SyntaxTree.h"
#include <string.h>

//#define STMC_DEGUB
#ifdef STMC_DEGUB
#define STMC_ERROR \
    fprintf(stderr, "[DEBUG] Error in semantic analysis.\n");
#else
#define STMC_ERROR while(0);    // TODO
#endif


#define SMTC_SET_STACKTOP(top) \
    symbolStack[top] = NULL;

#define SET_SPECIFIER(newSpecifier, dt) \
    newSpecifier = (DataType*)malloc(sizeof(DataType)); \
    newSpecifier->kind = dt;
#define SET_FIELD(newField) \
    newField = (Field*)malloc(sizeof(Field)); \
    newField->next = NULL;
#define SET_SYMBOL(newSymbol, ns) \
    newSymbol = (Symbol*)malloc(sizeof(Symbol)); \
    newSymbol->nameSrc = ns; \
    newSymbol->next = NULL; \
    newSymbol->stackNext = NULL;
#define SET_BASIC_SYMBOL(newSymbol, basicType) \
    newSymbol->dataType->basic = basicType;

#define SMTC_TYPE_CHECK(dataType, k) \
    (dataType && dataType->kind == k)

#define SMTC_PROD_CHECK_1(node, t0) \
    (node->childrenNum == 1 && \
    node->children[0]->nodeType == NT_##t0)
#define SMTC_PROD_CHECK_2(node, t0, t1) \
    (node->childrenNum == 2 && \
    node->children[0]->nodeType == NT_##t0 && \
    node->children[1]->nodeType == NT_##t1)
#define SMTC_PROD_CHECK_3(node, t0, t1, t2) \
    (node->childrenNum == 3 && \
    node->children[0]->nodeType == NT_##t0 && \
    node->children[1]->nodeType == NT_##t1 && \
    node->children[2]->nodeType == NT_##t2)
#define SMTC_PROD_CHECK_4(node, t0, t1, t2, t3) \
    (node->childrenNum == 4 && \
    node->children[0]->nodeType == NT_##t0 && \
    node->children[1]->nodeType == NT_##t1 && \
    node->children[2]->nodeType == NT_##t2 && \
    node->children[3]->nodeType == NT_##t3)
#define SMTC_PROD_CHECK_5(node, t0, t1, t2, t3, t4) \
    (node->childrenNum == 5 && \
    node->children[0]->nodeType == NT_##t0 && \
    node->children[1]->nodeType == NT_##t1 && \
    node->children[2]->nodeType == NT_##t2 && \
    node->children[3]->nodeType == NT_##t3 && \
    node->children[4]->nodeType == NT_##t4)
#define SMTC_PROD_CHECK_7(node, t0, t1, t2, t3, t4, t5, t6) \
    (node->childrenNum == 7 && \
    node->children[0]->nodeType == NT_##t0 && \
    node->children[1]->nodeType == NT_##t1 && \
    node->children[2]->nodeType == NT_##t2 && \
    node->children[3]->nodeType == NT_##t3 && \
    node->children[4]->nodeType == NT_##t4 && \
    node->children[5]->nodeType == NT_##t5 && \
    node->children[6]->nodeType == NT_##t6)

#define SMTC_PROD_HANDLER_1(node, t0) \
    smtc##t0(node->children[0]);
#define SMTC_PROD_HANDLER_2(node, t0, t1) \
    smtc##t0(node->children[0]); \
    smtc##t1(node->children[1]);
#define SMTC_PROD_HANDLER_3(node, t0, t1, t2) \
    smtc##t0(node->children[0]); \
    smtc##t1(node->children[1]); \
    smtc##t2(node->children[2]);
#define SMTC_PROD_HANDLER_4(node, t0, t1, t2, t3) \
    smtc##t0(node->children[0]); \
    smtc##t1(node->children[1]); \
    smtc##t2(node->children[2]); \
    smtc##t3(node->children[3]);
#define SMTC_PROD_HANDLER_5(node, t0, t1, t2, t3, t4) \
    smtc##t0(node->children[0]); \
    smtc##t1(node->children[1]); \
    smtc##t2(node->children[2]); \
    smtc##t3(node->children[3]); \
    smtc##t4(node->children[4]);

void smtcProgram(TreeNode* root);
void smtcExtDefList(TreeNode* node);
void smtcExtDef(TreeNode* node);
void smtcExtDecList(TreeNode* node, DataType* specifier);
DataType* smtcSpecifier(TreeNode* node);
Symbol* smtcStructSpecifier(TreeNode* node);
void smtcOptTag(TreeNode* node, Symbol* newSymbol);
Symbol* smtcTag(TreeNode* node);
void smtcVarDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier);
void smtcVarDec4Param(TreeNode* node, Symbol* funcSymbol, DataType* specifier);
void smtcVarDec4Var(TreeNode* node, DataType* specifier, enum NameSrc ns);
void smtcFunDec(TreeNode* node, Symbol* funcSymbol);
void smtcVarList(TreeNode* node, Symbol* funcSymbol);
void smtcParamDec(TreeNode* node, Symbol* funcSymbol);
void smtcCompSt(TreeNode* node);    
void smtcStmtList(TreeNode* node);
void smtcStmt(TreeNode* node);
void smtcDefList4Field(TreeNode* node, Symbol* newSymbol);
void smtcDefList4LocalVar(TreeNode* node);
void smtcDef4Field(TreeNode* node, Symbol* newSymbol);
void smtcDef4LocalVar(TreeNode* node);
void smtcDecList4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier);
void smtcDecList4LocalVar(TreeNode* node, DataType* specifier);
void smtcDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier);
void smtcDec4LocalVar(TreeNode* node, DataType* specifier);
DataType* smtcExp(TreeNode* node);
void smtcArgs(TreeNode* node, Symbol* funcSymbol, Field* paramList);

bool equalDataType(DataType* typeA, DataType* typeB);
void printSmtcError(int lineno, int errorno, char* msg);

#endif