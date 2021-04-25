#include "SemanticAnalysis.h"

extern HashBucket* symbolTable;
extern StackEle* symbolStack;
extern int stackTop;

DataType* intSpecifier;
DataType* floatSpecifier;
DataType* errorSpecifier;
int smtcErrorNum = 0;
int anonStructNum = 0;
Symbol* tmpFuncSymbol = NULL;


void smtcProgram(TreeNode* root) {
    if(SMTC_PROD_CHECK_1(root, ExtDefList)) {
        init();
        stackTop = 0;
        SMTC_SET_STACKTOP(stackTop)
        smtcExtDefList(root->children[0]);
    }
    else STMC_ERROR
}

// done
void smtcExtDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, ExtDef, ExtDefList)) {
        SMTC_PROD_HANDLER_2(node, ExtDef, ExtDefList)
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else STMC_ERROR
}

// done
void smtcExtDef(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, ExtDecList, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcExtDecList(node->children[1], specifier);
    }
    else if(SMTC_PROD_CHECK_2(node, Specifier, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_3(node, Specifier, FunDec, CompSt)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        Symbol* funcSymbol;
        SET_SYMBOL(funcSymbol, NS_FUNC)
        funcSymbol->funcData = (FuncData*)malloc(sizeof(FuncData));
        funcSymbol->funcData->retType = specifier;
        funcSymbol->funcData->paramList = NULL;
        funcSymbol->funcData->tail = NULL;
        tmpFuncSymbol = funcSymbol;
        smtcFunDec(node->children[1], funcSymbol);
        if(search4Insert(SVAL(node->children[1]), NS_FUNC)) {
            char errMsg[256];
            sprintf(errMsg, "Redefined function \"%s\"", SVAL(node->children[1]));
            printSmtcError(node->children[1]->lineno, 4, errMsg);
        }
        else
            insertSymbol(funcSymbol);
        smtcCompSt(node->children[2]);
        tmpFuncSymbol = NULL;
        SMTC_SET_STACKTOP(stackTop)
        stackTop--;
    }
    else STMC_ERROR
}

// done
void smtcExtDecList(TreeNode* node, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        smtcVarDec4Var(node->children[0], specifier, NS_GVAR);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, COMMA, ExtDecList)) {
        smtcVarDec4Var(node->children[0], specifier, NS_GVAR);
        smtcExtDecList(node->children[2], specifier);
    }
    else STMC_ERROR
}

// done
DataType* smtcSpecifier(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, TYPE)) {
        DataType* newSpecifier = NULL;
        char* type = SVAL(node->children[0]);
        if(strcmp(type, "int") == 0) {
            newSpecifier = intSpecifier;
        }
        else if(strcmp(type, "float") == 0) {
            newSpecifier = floatSpecifier;
        }
        return newSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, StructSpecifier)) {
        Symbol* s = smtcStructSpecifier(node->children[0]);
        return s->dataType;
    }
    else STMC_ERROR
    return NULL;
}

// done
Symbol* smtcStructSpecifier(TreeNode* node) {        
    if(SMTC_PROD_CHECK_5(node, STRUCT, OptTag, LC, DefList, RC)) {
        Symbol* newSymbol;
        SET_SYMBOL(newSymbol, NS_STRUCT)
        SET_SPECIFIER(newSymbol->dataType, DT_STRUCT)
        newSymbol->dataType->structure.fieldList = NULL;
        newSymbol->dataType->structure.tail = NULL;
        smtcOptTag(node->children[1], newSymbol);
        smtcDefList4Field(node->children[3], newSymbol);
        Symbol* errSymbol = search4Insert(SVAL(node->children[0]), NS_STRUCT);
        if(!errSymbol) {
            insertSymbol(newSymbol);
            return newSymbol;
        }
        else {
            char errMsg[256];
            sprintf(errMsg, "Duplicated name \"%s\"", SVAL(node->children[1]));
            printSmtcError(node->children[1]->lineno, 16, errMsg);
            return errSymbol;
        } 
    }
    else if(SMTC_PROD_CHECK_2(node, STRUCT, Tag)) {
        return smtcTag(node->children[1]);
    }
    else STMC_ERROR
    return NULL;
}

// done
void smtcOptTag(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        newSymbol->name = SVAL(node->children[0]);
    }
    else if(node->childrenNum == 0) { 
        newSymbol->name = (char*)malloc(sizeof(char)*32);
        sprintf(newSymbol->name, "AnonStruct_%d", anonStructNum);
        anonStructNum++;
    }
    else STMC_ERROR
}

// done
Symbol* smtcTag(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* structSymbol = search4Use(SVAL(node->children[0]), NS_STRUCT);
        if(!structSymbol) {
            char errMsg[256];
            sprintf(errMsg, "Undefined structure \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 17, errMsg);
        }
        return structSymbol;
    }
    else STMC_ERROR
    return NULL;
}

// done
void smtcVarDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        if(search4Field(SVAL(node->children[0]), newSymbol)) {
            // TODO: rename and insert
            char errMsg[256];
            sprintf(errMsg, "Redefined field \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 15, errMsg);
        }
        else {
            Field* newField;
            SET_FIELD(newField)
            newField->name = SVAL(node->children[0]);
            newField->dataType = specifier;
            if(newSymbol->dataType->structure.fieldList)
                newSymbol->dataType->structure.tail->next = newField;
            else
                newSymbol->dataType->structure.fieldList = newField;
            newSymbol->dataType->structure.tail = newField;
        }
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Field(node->children[0], newSymbol, newSpecifier);
    }
    else STMC_ERROR
}

// done
void smtcVarDec4Param(TreeNode* node, Symbol* funcSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        if(search4Insert(SVAL(node->children[0]), NS_LVAR)) {
            // TODO: rename and insert
            char errMsg[256];
            sprintf(errMsg, "Redefined function parameter \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 3, errMsg);
        }
        else {
            Param* newParam;
            Symbol* paramSymbol;
            SET_FIELD(newParam)
            newParam->name = SVAL(node->children[0]);
            newParam->dataType = specifier;
            if(funcSymbol->funcData->paramList)
                funcSymbol->funcData->tail->next = newParam;
            else
                funcSymbol->funcData->paramList = newParam;
            funcSymbol->funcData->tail = newParam;
            SET_SYMBOL(paramSymbol, NS_LVAR)
            paramSymbol->dataType = specifier;
            insertSymbol(paramSymbol);
        }
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Param(node->children[0], funcSymbol, newSpecifier);
    }
    else STMC_ERROR
}

// done
void smtcVarDec4Var(TreeNode* node, DataType* specifier, enum NameSrc ns) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol;
        SET_SYMBOL(varSymbol, ns)
        varSymbol->name = SVAL(node->children[0]);
        varSymbol->dataType = specifier;
        insertSymbol(varSymbol);
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Var(node->children[0], newSpecifier, NS_GVAR);
    }
    else STMC_ERROR
}

void smtcFunDec(TreeNode* node, Symbol* funcSymbol) {
    if(SMTC_PROD_CHECK_4(node, ID, LP, VarList, RP)) {
        funcSymbol->name = SVAL(node->children[0]);
        smtcVarList(node->children[2], funcSymbol);
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        funcSymbol->name = SVAL(node->children[0]);
    }
    else STMC_ERROR
}

// done
void smtcVarList(TreeNode* node, Symbol* funcSymbol) {
    if(SMTC_PROD_CHECK_3(node, ParamDec, COMMA, VarList)) {
        smtcParamDec(node->children[0], funcSymbol);
        smtcVarList(node->children[2], funcSymbol);
    }
    else if(SMTC_PROD_CHECK_1(node, ParamDec)) {
        smtcParamDec(node->children[0], funcSymbol);
    }
    else STMC_ERROR
}

// done
void smtcParamDec(TreeNode* node, Symbol* funcSymbol) {
    if(SMTC_PROD_CHECK_2(node, Specifier, VarDec)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcVarDec4Param(node->children[1], funcSymbol, specifier);
    }
    else STMC_ERROR
}

void smtcCompSt(TreeNode* node) {
    if(SMTC_PROD_CHECK_4(node, LC, DefList, StmtList, RC)) {
        stackTop++;
        if(stackTop >= MAX_SS_SIZE || stackTop < 0) {
            // TODO: handle stack error
        }
        SMTC_SET_STACKTOP(stackTop)
        smtcDefList4LocalVar(node->children[1]);
        smtcStmtList(node->children[2]);
    }
    else STMC_ERROR
}

// done
void smtcStmtList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Stmt, StmtList)) {
        smtcStmt(node->children[0]);
        smtcStmtList(node->children[1]);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else STMC_ERROR
}

void smtcStmt(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Exp, SEMI)) {
        DataType* dataType = smtcExp(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_1(node, CompSt)) {
        smtcCompSt(node->children[0]);
        SMTC_SET_STACKTOP(stackTop)
        stackTop--;
    }
    else if(SMTC_PROD_CHECK_3(node, RETURN, Exp, SEMI)) {
        DataType* dataType = smtcExp(node->children[1]);
        if(dataType != tmpFuncSymbol->funcData->retType) {
            printSmtcError(node->children[1]->lineno, 8, "Type mismatched for return");
        }
    }
    else if(SMTC_PROD_CHECK_5(node, IF, LP, Exp, RP, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(dataType != intSpecifier) {
            printSmtcError(node->children[1]->lineno, 0, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
    }
    else if(SMTC_PROD_CHECK_7(node, IF, LP, Exp, RP, Stmt, ELSE, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(dataType != intSpecifier) {
            printSmtcError(node->children[1]->lineno, 0, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
        smtcStmt(node->children[6]);
    }
    else if(SMTC_PROD_CHECK_5(node, WHILE, LP, Exp, RP, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(dataType != intSpecifier) {
            printSmtcError(node->children[1]->lineno, 0, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
    }
    else STMC_ERROR
}

// done
void smtcDefList4Field(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        smtcDef4Field(node->children[0], newSymbol);
        smtcDefList4Field(node->children[1], newSymbol);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else STMC_ERROR
}

void smtcDefList4LocalVar(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        smtcDef4LocalVar(node->children[0]);
        smtcDefList4LocalVar(node->children[1]);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else STMC_ERROR
}

// done
void smtcDef4Field(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcDecList4Field(node->children[1], newSymbol, specifier);
    }
    else STMC_ERROR
}

void smtcDef4LocalVar(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcDecList4LocalVar(node->children[1], specifier);
    }
    else STMC_ERROR
}

// done
void smtcDecList4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, Dec)) {
        smtcDec4Field(node->children[0], newSymbol, specifier);
    }
    else if(SMTC_PROD_CHECK_3(node, Dec, COMMA, DecList)) {
        smtcDec4Field(node->children[0], newSymbol, specifier);
        smtcDecList4Field(node->children[2], newSymbol, specifier);
    }
    else STMC_ERROR
}

void smtcDecList4LocalVar(TreeNode* node, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, Dec)) {
        smtcDec4LocalVar(node->children[0], specifier);
    }
    else if(SMTC_PROD_CHECK_3(node, Dec, COMMA, DecList)) {
        smtcDec4LocalVar(node->children[0], specifier);
        smtcDecList4LocalVar(node->children[2], specifier);
    }
    else STMC_ERROR
}

void smtcDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        smtcVarDec4Field(node->children[0], newSymbol, specifier);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        smtcVarDec4Field(node->children[0], newSymbol, specifier);
        DataType* type = smtcExp(node->children[2]);
        if(!equalDataType(type, specifier)) {
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        }
    }
    else STMC_ERROR
}

void smtcDec4LocalVar(TreeNode* node, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        smtcVarDec4Var(node->children[0], specifier, NS_LVAR);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        smtcVarDec4Var(node->children[0], specifier, NS_LVAR);
        DataType* type = smtcExp(node->children[2]);
        if(!equalDataType(type, specifier)) {
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        }
    }
    else STMC_ERROR
}

DataType* smtcExp(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Exp, ASSIGNOP, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(!(SMTC_PROD_CHECK_1(node->children[0], ID)) || 
               !(SMTC_PROD_CHECK_4(node->children[0], Exp, LB, Exp, RB)) || 
               !(SMTC_PROD_CHECK_3(node->children[0], Exp, DOT, ID))) {
                printSmtcError(node->children[0]->lineno, 6, "The left-hand side of an assignment must be a variable");
            }
            else
                return typeA;
        }
        else
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp) || SMTC_PROD_CHECK_3(node, Exp, OR, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(typeA != intSpecifier)
                printSmtcError(node->children[0]->lineno, 7, "Only integers can be involved in logical operations");
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, RELOP, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(strcmp(SVAL(node->children[1]), "==") == 0 || strcmp(SVAL(node->children[1]), "!=") == 0) { 
                /* do nothing */
            }
            else {  // > / < / >= / <=
                if(typeA != intSpecifier && typeA != floatSpecifier) {
                    printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in relational operations");
                }
            }
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, PLUS, Exp) || SMTC_PROD_CHECK_3(node, Exp, MINUS, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, STAR, Exp) || SMTC_PROD_CHECK_3(node, Exp, DIV, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(typeA != intSpecifier && typeA != floatSpecifier) {
                printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in arithmetic operations");
            }
            else
                return typeA;
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, LP, Exp, RP)) {
        return smtcExp(node->children[1]);
    }
    else if(SMTC_PROD_CHECK_2(node, MINUS, Exp)) {
        DataType* type = smtcExp(node->children[1]);
        if(type != intSpecifier && type != floatSpecifier) {
            printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in arithmetic operations");
            return errorSpecifier;
        }
        return type;
    }
    else if(SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        DataType* type = smtcExp(node->children[1]);
        if(type != intSpecifier) {
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        }
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_4(node, ID, LP, Args, RP)) {
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        if(funcSymbol) {
            if(funcSymbol->nameSrc == NS_FUNC) {
                smtcArgs(node->children[2], funcSymbol, funcSymbol->funcData->paramList);
                return funcSymbol->funcData->retType;
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "\"%s\" is not a function", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 11, errMsg);
                smtcArgs(node->children[2], NULL, NULL);
            }
        }
        else {
            char errMsg[256];
            sprintf(errMsg, "Undefined function \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 2, errMsg);
            smtcArgs(node->children[2], NULL, NULL);
        }
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        if(funcSymbol) {
            if(funcSymbol->nameSrc == NS_FUNC) {
                return funcSymbol->funcData->retType;
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "\"%s\" is not a function", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 11, errMsg);
            }
        }
        else {
            char errMsg[256];
            sprintf(errMsg, "Undefined function \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 2, errMsg);    
        }
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(SMTC_TYPE_CHECK(typeA, DT_ARRAY)) {
            return typeA->array.elem;
        }
        else
            printSmtcError(node->children[0]->lineno, 10, "Expecting \"array\" type before \"[]\"");
        if(typeB != intSpecifier) {
            printSmtcError(node->children[2]->lineno, 12, "Expecting \"int\" type when accessing values of an array");
        }
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        DataType* type = smtcExp(node->children[0]);
        if(SMTC_TYPE_CHECK(type, DT_STRUCT)) {
            Field* fieldList = type->structure.fieldList;
            while(fieldList) {
                if(strcmp(fieldList->name, SVAL(node->children[2])) == 0) {
                    return fieldList->dataType;
                }
                fieldList = fieldList->next;
            }
            if(!fieldList) {
                char errMsg[256];
                sprintf(errMsg, "Non-existent field \"%s\"", SVAL(node->children[2]));
                printSmtcError(node->children[2]->lineno, 14, errMsg);
            }
        }
        else
            printSmtcError(node->children[0]->lineno, 13, "Expecting \"struct\" type before \".\"");
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* s = search4Use(SVAL(node->children[0]), NS_LVAR);
        if(!s)
            s = search4Use(SVAL(node->children[0]), NS_GVAR);
        if(s)
            return s->dataType;
        else {
            char errMsg[256];
            sprintf(errMsg, "Undefined variable \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 1, errMsg);
        }
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, INT)) {
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, FLOAT)) {
        return floatSpecifier;
    }
    else STMC_ERROR
    return errorSpecifier;;
}

// done
void smtcArgs(TreeNode* node, Symbol* funcSymbol, Field* paramList) {
    if(SMTC_PROD_CHECK_3(node, Exp, COMMA, Args)) {
        DataType* type = smtcExp(node->children[0]);
        if(funcSymbol) {
            if(paramList) {
                if(paramList->dataType != type) {
                    char errMsg[256];
                    sprintf(errMsg, "Inapplicable argument type for function \"%s\"", funcSymbol->name);
                    printSmtcError(node->children[0]->lineno, 9, errMsg);
                }
                smtcArgs(node->children[2], funcSymbol, paramList->next);
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "Too many arguments for function \"%s\"", funcSymbol->name);
                printSmtcError(node->children[0]->lineno, 9, errMsg);
                smtcArgs(node->children[2], funcSymbol, NULL);
            }
        }
        else
            smtcArgs(node->children[2], NULL, NULL);        
    }
    else if(SMTC_PROD_CHECK_1(node, Exp)) {
        DataType* type = smtcExp(node->children[0]);
        if(funcSymbol) {
            if(paramList) {
                if(paramList->dataType != type) {
                    char errMsg[256];
                    sprintf(errMsg, "Inapplicable argument type for function \"%s\"", funcSymbol->name);
                    printSmtcError(node->children[0]->lineno, 9, errMsg);
                }
                if(paramList->next) {
                    char errMsg[256];
                    sprintf(errMsg, "Too few arguments for function \"%s\"", funcSymbol->name);
                    printSmtcError(node->children[0]->lineno, 9, errMsg);
                }
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "Too many arguments for function \"%s\"", funcSymbol->name);
                printSmtcError(node->children[0]->lineno, 9, errMsg);
            }
        }
    }
    else STMC_ERROR
}


bool equalDataType(DataType* typeA, DataType* typeB) {
    if(!typeA || !typeB)
        return false;
    if(typeA == errorSpecifier || typeB == errorSpecifier)
        return true;
    if(typeA->kind == DT_ARRAY && typeB->kind == DT_ARRAY) {
        DataType* pA = typeA;
        DataType* pB = typeB;
        while(pA && pB) {
            if(pA->kind == pB->kind) {
                if(pA->kind == DT_ARRAY) {
                    if(pA->array.size == pB->array.size) {
                        pA = typeA->array.elem;
                        pB = typeB->array.elem;
                    }
                    else
                        return false;
                }
                else 
                    return (pA == pB);
            }
            else 
                return false;
        }
        if(pA || pB)
            return false;
        else
            return true;
    }
    else if(typeA->kind == DT_BASIC && typeB->kind == DT_BASIC) {
        return (typeA == typeB);
    }
    else if(typeA->kind == DT_STRUCT && typeB->kind == DT_STRUCT) {
        return (typeA == typeB);
    }
    return false;
}

void printSmtcError(int lineno, int errorno, char* msg) {
	smtcErrorNum++;
    if(errorno == 0)
        fprintf(stderr, "Error at Line %d: Unknown error, %s.\n",  lineno, msg);
    else
	    fprintf(stderr, "Error type %d at Line %d: %s.\n", errorno, lineno, msg);
}