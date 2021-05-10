#include "SemanticAnalysis.h"

extern HashBucket* symbolTable;
extern StackEle* symbolStack;

DataType* intSpecifier = NULL;
DataType* floatSpecifier = NULL;
DataType* errorSpecifier = NULL;
Symbol* fieldSymbol = NULL;
Symbol* errorSymbol = NULL;
Symbol* tmpFuncSymbol = NULL;
int smtcErrorNum = 0;
int anonStructNum = 0;


void smtcProgram(TreeNode* node) {
    init();
    if(SMTC_PROD_CHECK_1(node, ExtDefList)) {    
        smtcExtDefList(node->children[0]);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Program)
}

// done
void smtcExtDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, ExtDef, ExtDefList)) {
        smtcExtDef(node->children[0]);
        smtcExtDefList(node->children[1]);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(ExtDefList)
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
        SMTC_SET_SYMBOL(funcSymbol, NS_FUNC)
        funcSymbol->funcData = (FuncData*)malloc(sizeof(FuncData));
        funcSymbol->funcData->retType = specifier;
        funcSymbol->funcData->paramList = NULL;
        funcSymbol->funcData->tail = NULL;
        tmpFuncSymbol = funcSymbol;
        stackPush();
        smtcFunDec(node->children[1], funcSymbol);
        if(search4Insert(funcSymbol->name, NS_FUNC)) {
            char errMsg[256];
            sprintf(errMsg, "Redefined function \"%s\"", funcSymbol->name);
            printSmtcError(node->children[1]->lineno, 4, errMsg);
        }
        else
            insertSymbol(funcSymbol);
        smtcCompSt(node->children[2]);
        tmpFuncSymbol = NULL;
        stackPop();
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(ExtDef)
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
    else SMTC_ERROR
    SMTC_PRINT_ERROR(ExtDecList)
}

// done
DataType* smtcSpecifier(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, TYPE)) {
        char* type = SVAL(node->children[0]);
        if(strcmp(type, "int") == 0) {
            return intSpecifier;
        }
        else if(strcmp(type, "float") == 0) {
            return floatSpecifier;
        }
    }
    else if(SMTC_PROD_CHECK_1(node, StructSpecifier)) {
        Symbol* s = smtcStructSpecifier(node->children[0]);
        if(s) {
            SMTC_PRINT_ERROR(Specifier)
            return s->dataType;
        }
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Specifier)
    return NULL;
}

// done
Symbol* smtcStructSpecifier(TreeNode* node) {        
    if(SMTC_PROD_CHECK_5(node, STRUCT, OptTag, LC, DefList, RC)) {
        Symbol* newSymbol;
        SMTC_SET_SYMBOL(newSymbol, NS_STRUCT)
        SMTC_SET_SPECIFIER(newSymbol->dataType, DT_STRUCT)
        newSymbol->dataType->structure.fieldList = NULL;
        newSymbol->dataType->structure.tail = NULL;
        smtcOptTag(node->children[1], newSymbol);
        smtcDefList4Field(node->children[3], newSymbol);

        Symbol* s = search4Insert(newSymbol->name, NS_STRUCT);
        if(s) {
            char errMsg[256];
            if(s->nameSrc == NS_STRUCT) {
                sprintf(errMsg, "Redefined struct \"%s\"", newSymbol->name);
                printSmtcError(node->children[1]->lineno, 16, errMsg);
                SMTC_PRINT_ERROR(StructSpecifier)
                return s;
            }
            else {  // NS_GVAR or NS_LVAR
                sprintf(errMsg, "Duplicated name \"%s\"", newSymbol->name);
                printSmtcError(node->children[1]->lineno, 16, errMsg);
                // insert anyway
                insertSymbol(newSymbol);
                SMTC_PRINT_ERROR(StructSpecifier)
                return newSymbol;
            }
        }
        else {
            insertSymbol(newSymbol);
            SMTC_PRINT_ERROR(StructSpecifier)
            return newSymbol;
        } 
    }
    else if(SMTC_PROD_CHECK_2(node, STRUCT, Tag)) {
        SMTC_PRINT_ERROR(StructSpecifier)
        return smtcTag(node->children[1]);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(StructSpecifier)
    return NULL;
}

// done
void smtcOptTag(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        newSymbol->name = SVAL(node->children[0]);
    }
    else if(node->childrenNum == 0) { 
        newSymbol->name = (char*)malloc(sizeof(char)*64);
        sprintf(newSymbol->name, "__unnamed_struct_%d", anonStructNum);
        anonStructNum++;
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(OptTag)
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
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Tag)
    return NULL;
}

// done
void smtcVarDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Field* newField;
        SMTC_SET_FIELD(newField)
        newField->name = SVAL(node->children[0]);
        newField->dataType = specifier;
        Symbol* s = search4Field(SVAL(node->children[0]));
        if(s) {
            if(s == fieldSymbol) {  // field in other struct
                char errMsg[256];
                sprintf(errMsg, "Duplicated field name \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 15, errMsg);
            }
            else {  // NS_GVAR
                char errMsg[256];
                sprintf(errMsg, "Duplicated name \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 15, errMsg);
            }
            // insert anyway
            if(newSymbol->dataType->structure.fieldList)
                newSymbol->dataType->structure.tail->next = newField;
            else
                newSymbol->dataType->structure.fieldList = newField;
            newSymbol->dataType->structure.tail = newField;
        }
        else {
            Field* p = newSymbol->dataType->structure.fieldList;
            while(p) {
                if(strcmp(p->name, SVAL(node->children[0])) == 0) {
                    char errMsg[256];
                    sprintf(errMsg, "Redefined field \"%s\"", SVAL(node->children[0]));
                    printSmtcError(node->children[0]->lineno, 15, errMsg);
                    break;
                }
                p = p->next;
            }
            if(!p) {
                if(newSymbol->dataType->structure.fieldList)
                    newSymbol->dataType->structure.tail->next = newField;
                else
                    newSymbol->dataType->structure.fieldList = newField;
                newSymbol->dataType->structure.tail = newField;
            }
        }
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SMTC_SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Field(node->children[0], newSymbol, newSpecifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(VarDec4Field)
}

// done
void smtcVarDec4Param(TreeNode* node, Symbol* funcSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Param* newParam;
        Symbol* paramSymbol;
        SMTC_SET_FIELD(newParam)
        newParam->name = SVAL(node->children[0]);
        newParam->dataType = specifier;
        SMTC_SET_SYMBOL(paramSymbol, NS_LVAR)
        paramSymbol->name = SVAL(node->children[0]);
        paramSymbol->dataType = specifier;

        Symbol* s = search4Insert(SVAL(node->children[0]), NS_LVAR);
        if(s) {
            if(s->nameSrc == NS_LVAR) {
                char errMsg[256];
                sprintf(errMsg, "Redefined function parameter \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 3, errMsg);
            }
            else {  // NS_STRUCT
                char errMsg[256];
                sprintf(errMsg, "Duplicated name \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 3, errMsg);
                // insert anyway
                if(funcSymbol->funcData->paramList)
                    funcSymbol->funcData->tail->next = newParam;
                else
                    funcSymbol->funcData->paramList = newParam;
                funcSymbol->funcData->tail = newParam;
                insertSymbol(paramSymbol);
            }   
        }
        else {
            if(funcSymbol->funcData->paramList)
                funcSymbol->funcData->tail->next = newParam;
            else
                funcSymbol->funcData->paramList = newParam;
            funcSymbol->funcData->tail = newParam;
            insertSymbol(paramSymbol);
        }
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SMTC_SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Param(node->children[0], funcSymbol, newSpecifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(VarDec4Param)
}

// done
DataType* smtcVarDec4Var(TreeNode* node, DataType* specifier, enum NameSrc ns) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol;
        SMTC_SET_SYMBOL(varSymbol, ns)
        varSymbol->name = SVAL(node->children[0]);
        varSymbol->dataType = specifier;

        Symbol* s = search4Insert(SVAL(node->children[0]), ns);
        if(ns == NS_GVAR) {
            if(s) {
                char errMsg[256];
                if(s != fieldSymbol && s->nameSrc == NS_GVAR) {
                    sprintf(errMsg, "Redefined variable \"%s\"", SVAL(node->children[0]));
                    printSmtcError(node->children[0]->lineno, 3, errMsg);
                }
                else {  // field or NS_STRUCT
                    sprintf(errMsg, "Duplicated name \"%s\"", SVAL(node->children[0]));
                    printSmtcError(node->children[0]->lineno, 3, errMsg);
                    // insert anyway
                    insertSymbol(varSymbol);
                }
            }
            else
                insertSymbol(varSymbol);
        }
        else {  // NS_LVAR
            if(s) {
                char errMsg[256];
                if(s->nameSrc == NS_LVAR) {
                    sprintf(errMsg, "Redefined variable \"%s\"", SVAL(node->children[0]));
                    printSmtcError(node->children[0]->lineno, 3, errMsg);
                }
                else {  // NS_STRUCT
                    sprintf(errMsg, "Duplicated name \"%s\"", SVAL(node->children[0]));
                    printSmtcError(node->children[0]->lineno, 3, errMsg);
                    // insert anyway
                    insertSymbol(varSymbol);
                }
            }
            else
                insertSymbol(varSymbol);
        }
        
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        DataType* newSpecifier;
        SMTC_SET_SPECIFIER(newSpecifier, DT_ARRAY)
        newSpecifier->array.elem = specifier;
        newSpecifier->array.size = IVAL(node->children[2]);
        smtcVarDec4Var(node->children[0], newSpecifier, ns);
        return newSpecifier;
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(VarDec4Var)
    return specifier;
}

void smtcFunDec(TreeNode* node, Symbol* funcSymbol) {
    if(SMTC_PROD_CHECK_4(node, ID, LP, VarList, RP)) {
        funcSymbol->name = SVAL(node->children[0]);
        smtcVarList(node->children[2], funcSymbol);
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        funcSymbol->name = SVAL(node->children[0]);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(FunDec)
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
    else SMTC_ERROR
    SMTC_PRINT_ERROR(VarList)
}

// done
void smtcParamDec(TreeNode* node, Symbol* funcSymbol) {
    if(SMTC_PROD_CHECK_2(node, Specifier, VarDec)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcVarDec4Param(node->children[1], funcSymbol, specifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(ParamDec)
}

void smtcCompSt(TreeNode* node) {
    if(SMTC_PROD_CHECK_4(node, LC, DefList, StmtList, RC)) {
        smtcDefList4LocalVar(node->children[1]);
        smtcStmtList(node->children[2]);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(CompSt)
}

// done
void smtcStmtList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Stmt, StmtList)) {
        smtcStmt(node->children[0]);
        smtcStmtList(node->children[1]);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(StmtList)
}

void smtcStmt(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Exp, SEMI)) {
        DataType* dataType = smtcExp(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_1(node, CompSt)) {
        stackPush();
        smtcCompSt(node->children[0]);
        stackPop();
    }
    else if(SMTC_PROD_CHECK_3(node, RETURN, Exp, SEMI)) {
        DataType* dataType = smtcExp(node->children[1]);
        if(!equalDataType(dataType, tmpFuncSymbol->funcData->retType)) {
            printSmtcError(node->children[1]->lineno, 8, "Type mismatched for return");
        }
    }
    else if(SMTC_PROD_CHECK_5(node, IF, LP, Exp, RP, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(!equalDataType(dataType, intSpecifier)) {
            printSmtcError(node->children[1]->lineno, 7, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
    }
    else if(SMTC_PROD_CHECK_7(node, IF, LP, Exp, RP, Stmt, ELSE, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(!equalDataType(dataType, intSpecifier)) {
            printSmtcError(node->children[1]->lineno, 7, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
        smtcStmt(node->children[6]);
    }
    else if(SMTC_PROD_CHECK_5(node, WHILE, LP, Exp, RP, Stmt)) {
        DataType* dataType = smtcExp(node->children[2]);
        if(!equalDataType(dataType, intSpecifier)) {
            printSmtcError(node->children[1]->lineno, 7, "Expecting \"int\" type between \"(\"\")\"");
        }
        smtcStmt(node->children[4]);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Stmt)
}

// done
void smtcDefList4Field(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        smtcDef4Field(node->children[0], newSymbol);
        smtcDefList4Field(node->children[1], newSymbol);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(DefList4Field)
}

void smtcDefList4LocalVar(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        smtcDef4LocalVar(node->children[0]);
        smtcDefList4LocalVar(node->children[1]);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(DefList4LocalVar)
}

// done
void smtcDef4Field(TreeNode* node, Symbol* newSymbol) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcDecList4Field(node->children[1], newSymbol, specifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Def4Field)
}

void smtcDef4LocalVar(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        DataType* specifier = smtcSpecifier(node->children[0]);
        smtcDecList4LocalVar(node->children[1], specifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Def4LocalVar)
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
    else SMTC_ERROR
    SMTC_PRINT_ERROR(DecList4Field)
}

void smtcDecList4LocalVar(TreeNode* node, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, Dec)) {
        smtcDec4LocalVar(node->children[0], specifier);
    }
    else if(SMTC_PROD_CHECK_3(node, Dec, COMMA, DecList)) {
        smtcDec4LocalVar(node->children[0], specifier);
        smtcDecList4LocalVar(node->children[2], specifier);
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(DecList4LocalVar)
}

void smtcDec4Field(TreeNode* node, Symbol* newSymbol, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        smtcVarDec4Field(node->children[0], newSymbol, specifier);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        smtcVarDec4Field(node->children[0], newSymbol, specifier);
        DataType* type = smtcExp(node->children[2]);
        printSmtcError(node->children[1]->lineno, 15, "Define and initialize field simutanously");
        /*
        if(!equalDataType(type, specifier)) {
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        }*/
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Dec4Field)
}

void smtcDec4LocalVar(TreeNode* node, DataType* specifier) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        smtcVarDec4Var(node->children[0], specifier, NS_LVAR);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        DataType* typeA = smtcVarDec4Var(node->children[0], specifier, NS_LVAR);
        DataType* typeB = smtcExp(node->children[2]);
        if(!equalDataType(typeA, typeB)) {
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        }
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Dec4LocalVar)
}

DataType* smtcExp(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Exp, ASSIGNOP, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(!SMTC_PROD_CHECK_1(node->children[0], ID) &&
               !SMTC_PROD_CHECK_4(node->children[0], Exp, LB, Exp, RB) &&
               !SMTC_PROD_CHECK_3(node->children[0], Exp, DOT, ID)) {
                printSmtcError(node->children[0]->lineno, 6, "The left-hand side of an assignment must be a variable");
            }
            else {
                SMTC_PRINT_ERROR(Exp)
                return typeA;
            }
        }
        else
            printSmtcError(node->children[0]->lineno, 5, "Type mismatched for assignment");
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp) || SMTC_PROD_CHECK_3(node, Exp, OR, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(!equalDataType(typeA, intSpecifier))
                printSmtcError(node->children[0]->lineno, 7, "Only integers can be involved in logical operations");
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        SMTC_PRINT_ERROR(Exp)
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
                if(!equalDataType(typeA, intSpecifier) && !equalDataType(typeA, floatSpecifier)) {
                    printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in relational operations");
                }
            }
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        SMTC_PRINT_ERROR(Exp)
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, PLUS, Exp) || SMTC_PROD_CHECK_3(node, Exp, MINUS, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, STAR, Exp) || SMTC_PROD_CHECK_3(node, Exp, DIV, Exp)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        if(equalDataType(typeA, typeB)) {
            if(!equalDataType(typeA, intSpecifier) && !equalDataType(typeA, floatSpecifier)) {
                printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in arithmetic operations");
            }
            else {
                SMTC_PRINT_ERROR(Exp)
                return typeA;
            }
        }
        else
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, LP, Exp, RP)) {
        return smtcExp(node->children[1]);
    }
    else if(SMTC_PROD_CHECK_2(node, MINUS, Exp)) {
        DataType* type = smtcExp(node->children[1]);
        if(!equalDataType(type, intSpecifier) && !equalDataType(type, floatSpecifier)) {
            printSmtcError(node->children[0]->lineno, 7, "Only integers and float numbers can be involved in arithmetic operations");
            SMTC_PRINT_ERROR(Exp)
            return errorSpecifier;
        }
        SMTC_PRINT_ERROR(Exp)
        return type;
    }
    else if(SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        DataType* type = smtcExp(node->children[1]);
        if(!equalDataType(type, intSpecifier)) {
            printSmtcError(node->children[0]->lineno, 7, "Type mismatched for operands");
        }
        SMTC_PRINT_ERROR(Exp)
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_4(node, ID, LP, Args, RP)) {
        // TODO: func duplicated name
        Symbol* s = search4Use(SVAL(node->children[0]), NS_FUNC);
        if(s) {
            smtcArgs(node->children[2], s, s->funcData->paramList);
            SMTC_PRINT_ERROR(Exp)
            return s->funcData->retType;
        }
        else {
            s = search4Use(SVAL(node->children[0]), NS_LVAR);
            if(!s)
                s = search4Use(SVAL(node->children[0]), NS_GVAR);
            if(s) {
                char errMsg[256];
                sprintf(errMsg, "\"%s\" is not a function", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 11, errMsg);
            }
            /*
            else {
                char errMsg[256];
                sprintf(errMsg, "Undefined function \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 2, errMsg);
                smtcArgs(node->children[2], NULL, NULL, false);
            }*/
        }
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        // TODO: func duplicated name
        Symbol* s = search4Use(SVAL(node->children[0]), NS_FUNC);
        if(s) {
            SMTC_PRINT_ERROR(Exp)
            return s->funcData->retType;
        }
        else {
            s = search4Use(SVAL(node->children[0]), NS_LVAR);
            if(!s)
                s = search4Use(SVAL(node->children[0]), NS_GVAR);
            if(s) {
                char errMsg[256];
                sprintf(errMsg, "\"%s\" is not a function", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 11, errMsg);
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "Undefined function \"%s\"", SVAL(node->children[0]));
                printSmtcError(node->children[0]->lineno, 2, errMsg);
            }    
        }
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB)) {
        DataType* typeA = smtcExp(node->children[0]);
        DataType* typeB = smtcExp(node->children[2]);
        // TODO: type check
        if(!equalDataType(typeB, intSpecifier)) {
            printSmtcError(node->children[2]->lineno, 12, "Expecting \"int\" type when accessing values of an array");
        }
        if(SMTC_TYPE_CHECK(typeA, DT_ARRAY)) {
            SMTC_PRINT_ERROR(Exp)
            return typeA->array.elem;
        }
        else
            printSmtcError(node->children[0]->lineno, 10, "Expecting \"array\" type before \"[]\"");
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        DataType* type = smtcExp(node->children[0]);
        // TODO: type check
        if(SMTC_TYPE_CHECK(type, DT_STRUCT)) {
        #ifdef SMTC_DEBUG
            fprintf(stderr, "[SEM DEGUB] Search for Use: name: %s, ns: Field\n", SVAL(node->children[2]));
        #endif
            Field* fieldList = type->structure.fieldList;
            while(fieldList) {
                if(strcmp(fieldList->name, SVAL(node->children[2])) == 0) {
                    SMTC_PRINT_ERROR(Exp)
                    return fieldList->dataType;
                }
                fieldList = fieldList->next;
            }
            char errMsg[256];
            sprintf(errMsg, "Non-existent field \"%s\"", SVAL(node->children[2]));
            printSmtcError(node->children[2]->lineno, 14, errMsg);
        }
        else
            printSmtcError(node->children[0]->lineno, 13, "Expecting \"struct\" type before \".\"");
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* s = search4Use(SVAL(node->children[0]), NS_LVAR);
        if(!s)
            s = search4Use(SVAL(node->children[0]), NS_GVAR);
        if(s) {
            SMTC_PRINT_ERROR(Exp)
            return s->dataType;
        }
        else {
            char errMsg[256];
            sprintf(errMsg, "Undefined variable \"%s\"", SVAL(node->children[0]));
            printSmtcError(node->children[0]->lineno, 1, errMsg);
        }
        SMTC_PRINT_ERROR(Exp)
        return errorSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, INT)) {
        SMTC_PRINT_ERROR(Exp)
        return intSpecifier;
    }
    else if(SMTC_PROD_CHECK_1(node, FLOAT)) {
        SMTC_PRINT_ERROR(Exp)
        return floatSpecifier;
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Exp)
    return errorSpecifier;;
}

// done
void smtcArgs(TreeNode* node, Symbol* funcSymbol, Field* paramList) {
    if(SMTC_PROD_CHECK_3(node, Exp, COMMA, Args)) {
        DataType* type = smtcExp(node->children[0]);
        if(funcSymbol) {
            if(paramList) {
                if(!equalDataType(paramList->dataType, type)) {
                    char errMsg[256];
                    sprintf(errMsg, "Inapplicable argument type for function \"%s\"", funcSymbol->name);
                    printSmtcError(node->children[0]->lineno, 9, errMsg);
                    //smtcArgs(node->children[2], funcSymbol, paramList->next);
                }
                else 
                    smtcArgs(node->children[2], funcSymbol, paramList->next);
            }
            else {
                char errMsg[256];
                sprintf(errMsg, "Too many arguments for function \"%s\"", funcSymbol->name);
                printSmtcError(node->children[0]->lineno, 9, errMsg);
                //smtcArgs(node->children[2], funcSymbol, NULL);
            }
        }
        /* Handled by high levels
        else smtcArgs(node->children[2], NULL, NULL);*/        
    }
    else if(SMTC_PROD_CHECK_1(node, Exp)) {
        DataType* type = smtcExp(node->children[0]);
        if(funcSymbol) {
            if(paramList) {
                if(!equalDataType(paramList->dataType, type)) {
                    if(paramList->next) {
                        char errMsg[256];
                        sprintf(errMsg, "Mismatched arguments for function \"%s\"", funcSymbol->name);
                        printSmtcError(node->children[0]->lineno, 9, errMsg);
                    }
                    else {
                        char errMsg[256];
                        sprintf(errMsg, "Inapplicable argument type for function \"%s\"", funcSymbol->name);
                        printSmtcError(node->children[0]->lineno, 9, errMsg);
                    }     
                }
                else if(paramList->next) {
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
        /* Handled by high levels
        else smtcArgs(node->children[2], NULL, NULL);*/
    }
    else SMTC_ERROR
    SMTC_PRINT_ERROR(Args)
}


bool equalDataType(DataType* typeA, DataType* typeB) {
    if(!typeA || !typeB)
        return false;
    if(typeA == errorSpecifier || typeB == errorSpecifier)
        return true;
    if(typeA->kind == DT_ARRAY && typeB->kind == DT_ARRAY) {
        DataType* pA = typeA->array.elem;
        DataType* pB = typeB->array.elem;
        while(pA && pB) {
            if(pA->kind == pB->kind) {
                if(pA->kind == DT_ARRAY) {
                    pA = pA->array.elem;
                    pB = pB->array.elem;
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
        printf("Error at Line %d: Unknown error, %s.\n",  lineno, msg);
    else
	    printf("Error type %d at Line %d: %s.\n", errorno, lineno, msg);
}