#include "Translate.h"

int tempVarNum = 0;
int labelNum = 0;
int varNum = 0;
Operand *zeroOp, *oneOp;
int translateErrorNum = 0;
LabelList* labelList = NULL;

extern DataType* intSpecifier;

void initIC() {
    SET_OPERAND(zeroOp, OP_CONST)
    zeroOp->val = 0;
    SET_OPERAND(oneOp, OP_CONST)
    oneOp->val = 1;
}

Operand* newVar(Symbol* varSymbol) {
    Operand* t;
    SET_OPERAND(t, OP_VAR)
    t->var_no = varNum+1;
    varNum++;
    return t;
}

Operand* newTemp() {
    Operand* t;
    SET_OPERAND(t, OP_TEMP)
    t->var_no = tempVarNum+1;
    tempVarNum++;
    return t;
}

Operand* newLabel() {
    Operand* t;
    SET_OPERAND(t, OP_LABEL)
    t->label_no = labelNum+1;
    labelNum++;
    LabelList* newNode = (LabelList*)malloc(sizeof(LabelList));
    newNode->label = t;
    newNode->next = labelList;
    labelList = newNode;
    return t;
}

ICNode* newNode(enum ICType type, ...) {
    ICNode* node = NULL;
    va_list argList;
    va_start(argList, type);
    SET_ICNODE(node)
    node->code->kind = type;
    switch (type) {
    case IC_LABEL:
    case IC_GOTO:
    case IC_RETURN:
    case IC_ARG:
    case IC_PARAM:
    case IC_READ:
    case IC_WRITE:
        node->code->op = va_arg(argList, Operand*);
        break;
    case IC_FUNC:
        node->code->func = va_arg(argList, char*);
        break;
    case IC_ASSIGN:
        node->code->assign.left = va_arg(argList, Operand*);
        node->code->assign.right = va_arg(argList, Operand*);
        break; 
    case IC_ADD:
    case IC_SUB:
    case IC_MUL:
    case IC_DIV:
        node->code->binop.result = va_arg(argList, Operand*);
        node->code->binop.op1 = va_arg(argList, Operand*);
        node->code->binop.op2 = va_arg(argList, Operand*);
        break;
    case IC_IF_GOTO:
        node->code->ifgoto.relop = va_arg(argList, char*);
        node->code->ifgoto.op1 = va_arg(argList, Operand*);
        node->code->ifgoto.op2 = va_arg(argList, Operand*);
        node->code->ifgoto.label = va_arg(argList, Operand*);
        break;
    case IC_DEC:
        node->code->dec.op = va_arg(argList, Operand*);
        node->code->dec.size = va_arg(argList, int);
        break;
    case IC_CALL_FUNC:
        node->code->call.result = va_arg(argList, Operand*);
        node->code->call.func = va_arg(argList, char*);
        break;
    default:
        node = NULL;
        break;
    }
    va_end(argList);
    return node;
}

void changeLabelNo(int target, int val) {
    if(target <= 0 || val <= 0) return;
#ifdef SMTC_DEBUG
    fprintf(stderr, "label %d --> label %d\n", target, val);
#endif
    LabelList* cur = labelList;
    while(cur) {
        if(cur->label->label_no == target) 
            cur->label->label_no = val;
        cur = cur->next;
    }
}

Operand* getVarOp(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol);
        return varSymbol->op;
    }
    return NULL;
}

Operand* getVarAddr(char* name) {
    Symbol* varSymbol = search4Use(name, NS_LVAR);
    assert(varSymbol && varSymbol->dataType->kind != DT_BASIC);
    Operand* vaddr = NULL;
    if(varSymbol->op->kind == OP_VAR_ADDR) {
        SET_OPERAND(vaddr, OP_VAR) vaddr->var_no = varSymbol->op->var_no;
    } else /*if(varSymbol->op->kind == OP_VAR)*/ {
        SET_OPERAND(vaddr, OP_VAR_ADDR) vaddr->var_no = varSymbol->op->var_no;
    }
    return vaddr;
}

char* getRelop(char* relop) {
    if(strcmp(relop, ">") == 0) {
        return "<=";
    }
    else if(strcmp(relop, "<") == 0) {
        return ">=";
    }
    else if(strcmp(relop, ">=") == 0) {
        return "<";
    }
    else if(strcmp(relop, "<=") == 0) {
        return ">";
    }
    else if(strcmp(relop, "==") == 0) {
        return "!=";
    }
    else if(strcmp(relop, "!=") == 0) {
        return "==";
    }
    else return NULL;
}

ICNode* link(ICNode* n1, ICNode* n2) {
    if(n1 == NULL)
        return n2;
    if(n2 == NULL)
        return n1;
    ICNode* cur = n1;
    while(cur->next) {
        cur = cur->next;
    }
    cur->next = n2;
    n2->prev = cur;
    return n1;
}

ICNode* detailedAssign(DataType* type, Operand* laddr, Operand* raddr, int offset) {
    if(type->kind == DT_BASIC) {
        Operand *offsetOp, *t1, *t2, *lmem, *rmem;
        SET_OPERAND(offsetOp, OP_CONST) offsetOp->val = offset;
        ICNode *icnode1 = NULL, *icnode2 = NULL;
        if(offset == 0) {
            if(laddr->kind != OP_VAR && laddr->kind != OP_TEMP) {
                t1 = newTemp();
                icnode1 = newNode(IC_ASSIGN, t1, laddr);
            } else t1 = laddr;
            if(raddr->kind != OP_VAR && raddr->kind != OP_TEMP) {
                t2 = newTemp();
                icnode2 = newNode(IC_ASSIGN, t2, raddr);
            } else t2 = raddr;
        }
        else {
            t1 = newTemp(); t2 = newTemp();
            icnode1 = newNode(IC_ADD, t1, laddr, offsetOp);
            icnode2 = newNode(IC_ADD, t2, raddr, offsetOp);
        }
        SET_OPERAND(lmem, OP_TEMP_MEM) lmem->var_no = t1->var_no;
        SET_OPERAND(rmem, OP_TEMP_MEM) rmem->var_no = t2->var_no;
        ICNode* icnode3 = newNode(IC_ASSIGN, lmem, rmem);
        return link(icnode1, link(icnode2, icnode3));
    } else if(type->kind == DT_ARRAY) {
        Operand *t1, *t2 , *t3, *t4, *t5, *t6;
        Operand *label1 = newLabel(), *label2 = newLabel();
        t1 = newTemp();
        ICNode* icnode1 = newNode(IC_ASSIGN, t1, zeroOp);
        ICNode *icnode1_1 = NULL, *icnode1_2 = NULL;
        if(offset == 0) {
            t5 = laddr; t6 = raddr;
        } else {
            t5 = newTemp(); t6 = newTemp();
            Operand* offsetOp; SET_OPERAND(offsetOp, OP_CONST) offsetOp->val = offset;
            icnode1_1 = newNode(IC_ADD, t5, laddr, offsetOp);
            icnode1_2 = newNode(IC_ADD, t6, raddr, offsetOp);
        }
        ICNode* icnode2 = newNode(IC_LABEL, label1);
        Operand* size; SET_OPERAND(size, OP_CONST) size->val = type->array.size;
        ICNode* icnode3 = newNode(IC_IF_GOTO, ">=", t1, size, label2);
        Operand* elemsize; SET_OPERAND(elemsize, OP_CONST) elemsize->val = getsizeof(type->array.elem);
        t2 = newTemp(); t3 = newTemp(); t4 = newTemp();
        ICNode* icnode4 = newNode(IC_MUL, t2, t1, elemsize);
        ICNode* icnode5 = newNode(IC_ADD, t3, t5, t2);
        ICNode* icnode6 = newNode(IC_ADD, t4, t6, t2);
        ICNode* icnode7 = detailedAssign(type->array.elem, t3, t4, 0);
        ICNode* icnode8 = newNode(IC_ADD, t1, t1, oneOp);
        ICNode* icnode9 = newNode(IC_GOTO, label1);
        ICNode* icnode10 = newNode(IC_LABEL, label2);
        icnode1 = link(icnode1, link(icnode1_1, icnode1_2));
        icnode1 = link(icnode1, link(icnode2, icnode3));
        LINK_ICNODE(icnode4, icnode5, icnode6)
        LINK_ICNODE(icnode8, icnode9, icnode10)
        return link(link(icnode1, icnode4), link(icnode7, icnode8));
    } else {   // type->kind == DT_STRUCT
        ICNode *head = NULL, *tail = NULL;
        Field* curField = type->structure.fieldList;
        int structOffset = offset;
        while(curField) {
            tail = detailedAssign(curField->dataType, laddr, raddr, structOffset);
            head = link(head, tail);
            structOffset += getsizeof(curField->dataType);
            curField = curField->next;
        }
        return head;
    }
}

ICNode* handleAssign(char* left, char* right) {
    Symbol* lsym = search4Use(left, NS_LVAR);
    Symbol* rsym = search4Use(right, NS_LVAR);
    assert(lsym && rsym);
    if(lsym->dataType->kind == DT_BASIC) 
        return newNode(IC_ASSIGN, lsym->op, rsym->op);
    else {
        Operand *laddr, *raddr;
        if(lsym->op->kind == OP_VAR_ADDR) { SET_OPERAND(laddr, OP_VAR) }
        else { SET_OPERAND(laddr, OP_VAR_ADDR) }
        laddr->var_no = lsym->op->var_no;
        if(rsym->op->kind == OP_VAR_ADDR) { SET_OPERAND(raddr, OP_VAR) }
        else { SET_OPERAND(raddr, OP_VAR_ADDR) }
        raddr->var_no = rsym->op->var_no;
        if(lsym->dataType->kind == DT_ARRAY && (lsym->dataType->array.size > rsym->dataType->array.size))
            return detailedAssign(rsym->dataType, laddr, raddr, 0);
        else return detailedAssign(lsym->dataType, laddr, raddr, 0);
    }
}

ICNode* translateProgram(TreeNode* node) {
    initIC();
    if(SMTC_PROD_CHECK_1(node, ExtDefList)) {    
        return translateExtDefList(node->children[0]);
    }
    else while(0);
    return NULL;
}

ICNode* translateExtDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, ExtDef, ExtDefList)) {
        ICNode* icnode1 = translateExtDef(node->children[0]);
        ICNode* icnode2 = translateExtDefList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else while(0);
    return NULL;
}

ICNode* translateExtDef(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, ExtDecList, SEMI)) {
        printTranslateError("Code contains global variables");
    }
    else if(SMTC_PROD_CHECK_2(node, Specifier, SEMI)) { /* do nothing */ }
    else if(SMTC_PROD_CHECK_3(node, Specifier, FunDec, CompSt)) {
        ICNode* icnode1 = translateFunDec(node->children[1]);
        ICNode* icnode2 = translateCompSt(node->children[2]);
        return link(icnode1, icnode2);
    }
    else while(0);
    return NULL;
}

ICNode* translateVarDec(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        if(varSymbol->dataType->kind == DT_BASIC) return NULL;
        else return newNode(IC_DEC, varSymbol->op, getsizeof(varSymbol->dataType));
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            return translateVarDec(node->children[0]);
        } else printTranslateError("Code contains variables of multi-dimensional array type");
    }
    else while(0);
    return NULL;
}

ICNode* translateFunDec(TreeNode* node) {
    if(SMTC_PROD_CHECK_4(node, ID, LP, VarList, RP) || 
       SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        assert(funcSymbol != NULL);
        ICNode* icnode1 = newNode(IC_FUNC, funcSymbol->name);
        ICNode* tail = icnode1;
        Field* curParam = funcSymbol->funcData->paramList;
        while(curParam) {
            Symbol* paramSymbol = search4Use(curParam->name, NS_LVAR);
            assert(paramSymbol != NULL);
            if(paramSymbol->dataType->kind == DT_ARRAY)
                printTranslateError("Code contains parameters of array type");
            Operand* paramOp;
            SET_OPERAND(paramOp, OP_VAR)
            paramOp->var_no = paramSymbol->op->var_no;
            ICNode* paramNode = newNode(IC_PARAM, paramOp);
            tail->next = paramNode;
            paramNode->prev = tail;
            tail = paramNode;
            curParam = curParam->next;
        }
        return icnode1;
    }
    else while(0);
    return NULL;
}

ICNode* translateCompSt(TreeNode* node) {
    if(SMTC_PROD_CHECK_4(node, LC, DefList, StmtList, RC)) {
        ICNode* icnode1 = translateDefList(node->children[1]);
        ICNode* icnode2 = translateStmtList(node->children[2]);
        return link(icnode1, icnode2);
    }
    else while(0);
    return NULL;
}

ICNode* translateStmtList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Stmt, StmtList)) {
        ICNode* icnode1 = translateStmt(node->children[0]);
        ICNode* icnode2 = translateStmtList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else while(0);
    return NULL;
}

ICNode* translateStmt(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Exp, SEMI)) {
        return translateExp(node->children[0], NULL, NULL);
    }
    else if(SMTC_PROD_CHECK_1(node, CompSt)) {
        return translateCompSt(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_3(node, RETURN, Exp, SEMI)) {
        Operand* t1;
        // optimize
        if(SMTC_PROD_CHECK_1(node->children[1], ID)) {
            t1 = getVarOp(node->children[1]);
        }
        else if(SMTC_PROD_CHECK_1(node->children[1], INT)) {
            SET_OPERAND(t1, OP_CONST)
            t1->val = IVAL(node->children[1]->children[0]);
        }
        else {
            t1 = newTemp();
            ICNode* icnode1 = translateExp(node->children[1], t1, NULL);
            ICNode* icnode2 = newNode(IC_RETURN, t1);
            return link(icnode1, icnode2);
        }
        return newNode(IC_RETURN, t1);
    }
    else if(SMTC_PROD_CHECK_5(node, IF, LP, Exp, RP, Stmt)) {
        Operand* label2 = newLabel();
        ICNode* icnode1 = translateCond(node->children[2], NULL, label2, false);
        ICNode* icnode2_2 = translateStmt(node->children[4]);
        ICNode* icnode3 = newNode(IC_LABEL, label2);
        return link(link(icnode1, icnode2_2), icnode3);
    }
    else if(SMTC_PROD_CHECK_7(node, IF, LP, Exp, RP, Stmt, ELSE, Stmt)) {
        Operand* label2 = newLabel();
        Operand* label3 = newLabel();
        ICNode* icnode1 = translateCond(node->children[2], NULL, label2, false);
        ICNode* icnode2_2 = translateStmt(node->children[4]);
        ICNode* icnode2_3 = newNode(IC_GOTO, label3);
        ICNode* icnode3_1 = newNode(IC_LABEL, label2);
        ICNode* icnode3_2 = translateStmt(node->children[6]);
        ICNode* icnode4 = newNode(IC_LABEL, label3);
        icnode2_2 = link(icnode2_2, icnode2_3);
        icnode3_1 = link(icnode3_1, icnode3_2);
        return link(link(icnode1, icnode2_2), link(icnode3_1, icnode4));
    }
    else if(SMTC_PROD_CHECK_5(node, WHILE, LP, Exp, RP, Stmt)) {
        Operand* label1 = newLabel();
        Operand* label3 = newLabel();
        ICNode* icnode1_1 = newNode(IC_LABEL, label1);
        ICNode* icnode1_2 = translateCond(node->children[2], NULL, label3, false);
        ICNode* icnode2_2 = translateStmt(node->children[4]);
        ICNode* icnode2_3 = newNode(IC_GOTO, label1);
        ICNode* icnode3 = newNode(IC_LABEL, label3);
        icnode1_1 = link(icnode1_1, icnode1_2);
        icnode2_2 = link(icnode2_2, icnode2_3);
        return link(icnode1_1, link(icnode2_2, icnode3));
    }
    else while(0);
    return NULL;
}

ICNode* translateDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        ICNode* icnode1 = translateDef(node->children[0]);
        ICNode* icnode2 = translateDefList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else while(0);
    return NULL;
}

ICNode* translateDef(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        return translateDecList(node->children[1]);
    }
    else while(0);
    return NULL;
}

ICNode* translateDecList(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, Dec)) {
        return translateDec(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_3(node, Dec, COMMA, DecList)) {
        ICNode* icnode1 = translateDec(node->children[0]);
        ICNode* icnode2 = translateDecList(node->children[2]);
        return link(icnode1, icnode2);
    }
    else while(0);
    return NULL;
}

ICNode* translateDec(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        return translateVarDec(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            // struct or int type
            Symbol* varSymbol = search4Use(SVAL(node->children[0]->children[0]), NS_LVAR);
            assert(varSymbol != NULL);
            // optimize const
            if(SMTC_PROD_CHECK_1(node->children[2], INT)) {
                Operand* constOp;
                SET_OPERAND(constOp, OP_CONST) constOp->val = IVAL(node->children[2]->children[0]);
                return newNode(IC_ASSIGN, varSymbol->op, constOp);
            }  
            else if(varSymbol->dataType->kind == DT_BASIC) {
                Operand* t1 = newTemp();
                ICNode* icnode1 = translateExp(node->children[2], t1, NULL);
                ICNode* icnode2 = newNode(IC_ASSIGN, varSymbol->op, t1);
                return link(icnode1, icnode2);
            }
            else if(varSymbol->dataType->kind == DT_STRUCT) {
                Operand* t1 = newTemp();
                ICNode* icnode1 = translateExp(node->children[2], t1, NULL);
                Operand* vaddr;
                SET_OPERAND(vaddr, OP_VAR_ADDR) vaddr->var_no = varSymbol->op->var_no;
                ICNode* icnode2 = detailedAssign(varSymbol->dataType, vaddr, t1, 0);
                return link(icnode1, icnode2);
            }
            else while(0);
        }
        else {  // VarDec LB INT RB
            Symbol* varSymbol = search4Use(SVAL(node->children[0]->children[0]->children[0]), NS_LVAR);
            assert(varSymbol != NULL);
            ICNode* icnode1 = translateVarDec(node->children[0]);   // DEC
            DataType* typeB = getExpType(node->children[2]);
            Operand* t1 = newTemp();
            ICNode* icnode2 = translateExp(node->children[2], t1, NULL);
            Operand* vaddr;
            SET_OPERAND(vaddr, OP_VAR_ADDR) vaddr->var_no = varSymbol->op->var_no;
            ICNode* icnode3;
            if(varSymbol->dataType->array.size <= typeB->array.size)
                icnode3 = detailedAssign(varSymbol->dataType, vaddr, t1, 0);
            else icnode3 = detailedAssign(typeB, vaddr, t1, 0);
            return link(icnode1, link(icnode2, icnode3));
        }
    }
    else while(0);
    return NULL;
}

ICNode* translateCond(TreeNode* node, Operand* label_true, Operand* label_false, bool mod) {
    if(SMTC_PROD_CHECK_3(node, Exp, RELOP, Exp)) {
        Operand *t1, *t2;
        ICNode *icnode1 = NULL, *icnode2 = NULL, *icnode3 = NULL, *icnode4 = NULL;
        // optimize
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            t1 = getVarOp(node->children[0]);
        } else if(SMTC_PROD_CHECK_1(node->children[0], INT)) {
            SET_OPERAND(t1, OP_CONST)
            t1->val = IVAL(node->children[0]->children[0]);
        } else {
            t1 = newTemp();
            icnode1 = translateExp(node->children[0], t1, NULL);
        }
        if(SMTC_PROD_CHECK_1(node->children[2], ID)) {
            t2 = getVarOp(node->children[2]);
        } else if(SMTC_PROD_CHECK_1(node->children[2], INT)) {
            SET_OPERAND(t2, OP_CONST)
            t2->val = IVAL(node->children[2]->children[0]);
        } else {
            t2 = newTemp();
            icnode2 = translateExp(node->children[2], t2, NULL);
        }
        if(mod) { 
            assert(label_true);
            icnode3 = newNode(IC_IF_GOTO, SVAL(node->children[1]), t1, t2, label_true);
        } else {
            assert(label_false); 
            icnode3 = newNode(IC_IF_GOTO, getRelop(SVAL(node->children[1])), t1, t2, label_false); 
        }
        return link(icnode1, link(icnode2, icnode3));
    }
    else if(SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        return translateCond(node->children[1], label_false, label_true, !mod);
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp)) {
        ICNode *icnode1 = NULL, *icnode2 = NULL;
        if(mod) {
            Operand* label1 = newLabel();
            icnode1 = translateCond(node->children[0], NULL, label1, false);
            icnode2 = translateCond(node->children[2], label_true, NULL, true);
            ICNode* icnode3 = newNode(IC_LABEL, label1);
            return link(icnode1, link(icnode2, icnode3));
        } else {
            icnode1 = translateCond(node->children[0], label_true, label_false, mod);
            icnode2 = translateCond(node->children[2], label_true, label_false, mod);
            return link(icnode1, icnode2);
        }
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, OR, Exp)) {
        Operand* label1 = label_true;
        ICNode* icnode2_1 = NULL;
        if(!label_true) {
            label1 = newLabel();
            icnode2_1 = newNode(IC_LABEL, label1);
        }
        ICNode* icnode1 = translateCond(node->children[0], label1, NULL, true);
        ICNode* icnode2 = translateCond(node->children[2], label1, label_false, mod);
        return link(icnode1, link(icnode2, icnode2_1));
    }
    else {
        Operand* t1;
        ICNode *icnode1 = NULL, *icnode2 = NULL, *icnode3 = NULL;
        char* relop = (char*)malloc(sizeof(char)*4);
        strcpy(relop, "!=");
        // optimize
        if(SMTC_PROD_CHECK_1(node, ID)) {
            t1 = getVarOp(node);
        } else {
            t1 = newTemp();
            icnode1 = translateExp(node, t1, NULL);
        }
        if(mod) {
            assert(label_true);
            icnode2 = newNode(IC_IF_GOTO, relop, t1, zeroOp, label_true);
        } else { 
            assert(label_false);
            icnode2 = newNode(IC_IF_GOTO, getRelop(relop), t1, zeroOp, label_false); 
        }
        return link(icnode1, icnode2);
    }
}

// return: [place := &var + #offset]
ICNode* translateLeftExp(TreeNode* node, Operand* place, DataType** param) {
    if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB)) {
        DataType* dt;
        Operand *t0, *t1, *t2, *offset;
        ICNode *icnode0 = NULL, *icnode1 = NULL, *icnode2 = NULL, *icnode3 = NULL;
        // optimize
        if(SMTC_PROD_CHECK_1(node->children[2], INT)) {
            SET_OPERAND(t1, OP_CONST)
            t1->val = IVAL(node->children[2]->children[0]);
        } else if(SMTC_PROD_CHECK_1(node->children[2], ID)) {
            t1 = getVarOp(node->children[2]);
        } else {
            t1 = newTemp();
            icnode0 = translateExp(node->children[2], t1, NULL);
        }
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            Symbol* varSymbol = search4Use(SVAL(node->children[0]->children[0]), NS_LVAR);
            assert(varSymbol && varSymbol->dataType->kind == DT_ARRAY);
            if(varSymbol->op->kind == OP_VAR_ADDR) { SET_OPERAND(t0, OP_VAR) }
            else { SET_OPERAND(t0, OP_VAR_ADDR) }
            t0->var_no = varSymbol->op->var_no;
            dt = varSymbol->dataType;
        } else {
            t0 = newTemp();
            icnode1 = translateLeftExp(node->children[0], t0, &dt);
        }
        assert(dt->kind == DT_ARRAY);
        if(param) *param = dt->array.elem;
        if(t1->kind == OP_CONST) {
            if(t1->val == 0)
                return link(icnode1, newNode(IC_ASSIGN, place, t0));    // [place := &var]
            else {
                SET_OPERAND(offset, OP_CONST)
                offset->val = t1->val * getsizeof(dt->array.elem);
                return link(icnode1, newNode(IC_ADD, place, t0, offset));   // [place := &var + #offset]
            }
        }
        else {
            Operand *size;
            SET_OPERAND(size, OP_CONST)
            size->val = getsizeof(dt->array.elem);
            t2 = newTemp();
            icnode2 = newNode(IC_MUL, t2, t1, size);
            icnode3 = newNode(IC_ADD, place, t0, t2);
            return link(link(icnode0, icnode1), link(icnode2, icnode3));        // ... + [place := &var + t2]
        }
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        DataType *dt;
        Operand *t1, *offset;
        ICNode *icnode1 = NULL, *icnode2 = NULL;
        // optimize
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            Symbol* varSymbol = search4Use(SVAL(node->children[0]->children[0]), NS_LVAR);
            assert(varSymbol && varSymbol->dataType->kind == DT_STRUCT);
            if(varSymbol->op->kind == OP_VAR_ADDR) {    // param
                SET_OPERAND(t1, OP_VAR)
            }   
            else {  // local var
                SET_OPERAND(t1, OP_VAR_ADDR)
            }
            t1->var_no = varSymbol->op->var_no;
            dt = varSymbol->dataType;
        }
        else {
            t1 = newTemp();
            icnode1 = translateLeftExp(node->children[0], t1, &dt);
        }
        SET_OPERAND(offset, OP_CONST)
        offset->val = getFieldOffset(dt, SVAL(node->children[2]), param);
        if(offset->val == 0) icnode2 = newNode(IC_ASSIGN, place, t1);   // [place := &var]
        else icnode2 = newNode(IC_ADD, place, t1, offset);              // [place := &var + #offset]
        return link(icnode1, icnode2);  
    }
    else if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        Operand* var;
        if(varSymbol->op->kind == OP_VAR_ADDR) { SET_OPERAND(var, OP_VAR) }
        else { SET_OPERAND(var, OP_VAR_ADDR) }
        var->var_no = varSymbol->op->var_no; 
        return newNode(IC_ASSIGN, place, var);
    }
    else while(0);
    return NULL;
}

// usage: translate right Exp
ICNode* translateExp(TreeNode* node, Operand* place, DataType** param) {
    if(SMTC_PROD_CHECK_3(node, Exp, ASSIGNOP, Exp)) {
        Operand *t1, *t2;
        ICNode *icnode1 = NULL, *icnode2 = NULL, *icnode2_1 = NULL, *icnode2_2 = NULL;
        // optimize
        t1 = getVarOp(node->children[0]); t2 = getVarOp(node->children[2]);
        DataType* typeA = getExpType(node->children[0]);
        if(t1 && t2) {
            if(t1 != t2) icnode1 = handleAssign(SVAL(node->children[0]->children[0]), SVAL(node->children[2]->children[0]));
            if(place) {
                if(typeA->kind == DT_BASIC) icnode2_2 = newNode(IC_ASSIGN, place, t1);
                else icnode2_2 = newNode(IC_ASSIGN, place, getVarAddr(SVAL(node->children[0]->children[0])));
            }
            return link(icnode1, icnode2_2);
        }
        else {
            if(typeA->kind == DT_BASIC) {
                if(t1) {
                    if(SMTC_PROD_CHECK_1(node->children[2], INT)) {
                        SET_OPERAND(t2, OP_CONST) t2->val = IVAL(node->children[2]->children[0]);
                        icnode2_1 = newNode(IC_ASSIGN, t1, t2);
                    } else icnode2_1 = translateExp(node->children[2], t1, NULL);
                    if(place) icnode2_2 = newNode(IC_ASSIGN, place, t1);
                    return link(icnode2_1, icnode2_2);
                }
                else if(t2) {
                    t1 = newTemp();
                    icnode2 = translateLeftExp(node->children[0], t1, NULL);
                }
                else {
                    t1 = newTemp(); t2 = newTemp();
                    icnode1 = translateExp(node->children[2], t2, NULL);
                    icnode2 = translateLeftExp(node->children[0], t1, NULL);
                }
                Operand* t1mem; SET_OPERAND(t1mem, OP_TEMP_MEM) t1mem->var_no = t1->var_no;
                icnode2_1 = newNode(IC_ASSIGN, t1mem, t2);
                if(place) icnode2_2 = newNode(IC_ASSIGN, place, t1mem);
                return link(link(icnode1, icnode2), link(icnode2_1, icnode2_2));
            }
            else if(typeA->kind == DT_ARRAY) {
                Operand *t1addr, *t2addr;
                DataType* typeB = getExpType(node->children[2]);
                if(t1) {
                    t1addr = getVarAddr(SVAL(node->children[0]->children[0]));
                    t2addr = newTemp();
                    icnode1 = translateExp(node->children[2], t2addr, NULL);
                }
                else if(t2) {
                    t1addr = newTemp();
                    icnode2 = translateLeftExp(node->children[0], t1addr, NULL);
                    t2addr = getVarAddr(SVAL(node->children[2]->children[0]));
                }
                else {
                    t1addr = newTemp(); t2addr = newTemp();
                    icnode1 = translateExp(node->children[2], t2addr, NULL);
                    icnode2 = translateLeftExp(node->children[0], t1addr, NULL);
                }
                if(typeA->array.size <= typeB->array.size)
                    icnode2_1 = detailedAssign(typeA, t1addr, t2addr, 0);
                else icnode2_1 = detailedAssign(typeB, t1addr, t2addr, 0);
                if(place) icnode2_2 = newNode(IC_ASSIGN, place, t1addr);
                return link(link(icnode1, icnode2), link(icnode2_1, icnode2_2));
            }
            else {  // DT_STRUCT
                Operand *t1addr, *t2addr;
                if(t1) {
                    t1addr = getVarAddr(SVAL(node->children[0]->children[0]));
                    t2addr = newTemp();
                    icnode1 = translateExp(node->children[2], t2addr, NULL);
                }
                else if(t2) {
                    t1addr = newTemp();
                    icnode2 = translateLeftExp(node->children[0], t1addr, NULL);
                    t2addr = getVarAddr(SVAL(node->children[2]->children[0]));
                }
                else {
                    t1addr = newTemp(); t2addr = newTemp();
                    icnode1 = translateExp(node->children[2], t2addr, NULL);
                    icnode2 = translateLeftExp(node->children[0], t1addr, NULL);
                }
                icnode2_1 = detailedAssign(typeA, t1addr, t2addr, 0);
                if(place) icnode2_2 = newNode(IC_ASSIGN, place, t1addr);
                return link(link(icnode1, icnode2), link(icnode2_1, icnode2_2));
            }
        }
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp) || SMTC_PROD_CHECK_3(node, Exp, OR, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, RELOP, Exp) || SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        Operand* label2 = newLabel();
        ICNode *icnode0 = NULL, *icnode1 = NULL, *icnode2 = NULL, *icnode3 = NULL;
        // optimize
        if(!place) place = newTemp();
        if(place->kind == OP_TEMP) {
            icnode0 = newNode(IC_ASSIGN, place, zeroOp);
            icnode1 = translateCond(node, NULL, label2, false);
            icnode2 = newNode(IC_ASSIGN, place, oneOp);
            icnode3 = newNode(IC_LABEL, label2);
            return link(link(icnode0, icnode1), link(icnode2, icnode3));
        } else if(place->kind == OP_VAR) {
            Operand* t1 = newTemp();
            icnode0 = newNode(IC_ASSIGN, t1, zeroOp);
            icnode1 = translateCond(node, NULL, label2, false);
            icnode2 = newNode(IC_ASSIGN, t1, oneOp);
            icnode3 = newNode(IC_LABEL, label2);
            ICNode* icnode4 = newNode(IC_ASSIGN, place, t1);
            LINK_ICNODE(icnode2, icnode3, icnode4)
            return link(icnode0, link(icnode1, icnode2));
        } else while(0);
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, PLUS, Exp) || SMTC_PROD_CHECK_3(node, Exp, MINUS, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, STAR, Exp) || SMTC_PROD_CHECK_3(node, Exp, DIV, Exp)) {
        Operand *t1, *t2;
        ICNode *icnode1 = NULL, *icnode2 = NULL, *icnode3 = NULL;
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            t1 = getVarOp(node->children[0]);
        } else if(SMTC_PROD_CHECK_1(node->children[0], INT)) {
            SET_OPERAND(t1, OP_CONST) t1->val = IVAL(node->children[0]->children[0]);
        } else {  
            t1 = newTemp();
            icnode1 = translateExp(node->children[0], t1, NULL);
        }
        if(SMTC_PROD_CHECK_1(node->children[2], ID)) {
            t2 = getVarOp(node->children[2]);
        } else if(SMTC_PROD_CHECK_1(node->children[2], INT)) {
            SET_OPERAND(t2, OP_CONST) t2->val = IVAL(node->children[2]->children[0]);
        } else {
            t2 = newTemp();
            icnode2 = translateExp(node->children[2], t2, NULL);
        }    
        icnode3 = newNode(IC_ADD, place, t1, t2);
        if(!place) icnode3->code->binop.result = newTemp();
        switch (node->children[1]->nodeType) {
        case NT_PLUS: icnode3->code->kind = IC_ADD; break;
        case NT_MINUS: icnode3->code->kind = IC_SUB; break;
        case NT_STAR: icnode3->code->kind = IC_MUL; break;
        case NT_DIV: icnode3->code->kind = IC_DIV; break;
        default: break;
        }
        return link(icnode1, link(icnode2, icnode3));
    }
    else if(SMTC_PROD_CHECK_3(node, LP, Exp, RP)) {
        return translateExp(node->children[1], place, NULL);
    }
    else if(SMTC_PROD_CHECK_2(node, MINUS, Exp)) {
        Operand *t1;
        // optimize negative const
        if(!place) place = newTemp();
        if(SMTC_PROD_CHECK_1(node->children[1], INT)) {
            SET_OPERAND(t1, OP_CONST) t1->val = -IVAL(node->children[1]->children[0]);
            return newNode(IC_ASSIGN, place, t1);
        } else {
            ICNode *icnode1 = NULL, *icnode2 = NULL;
            t1 = newTemp();
            icnode1 = translateExp(node->children[1], t1, NULL);
            icnode2 = newNode(IC_SUB, place, zeroOp, t1);
            return link(icnode1, icnode2);
        }   
    }
    else if(SMTC_PROD_CHECK_4(node, ID, LP, Args, RP)) {
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        assert(funcSymbol != NULL);
        ArgList* argList = NULL;
        ICNode* icnode1 = translateArgs(node->children[2], &argList);
        if(strcmp("write", funcSymbol->name) == 0) {
            ICNode *icnode2 = NULL, *icnode3 = NULL;
            icnode2 = newNode(IC_WRITE, argList->arg);
            icnode1 = link(icnode1, icnode2);
            if(place) {
                icnode3 = newNode(IC_ASSIGN, place, zeroOp);
                icnode1 = link(icnode1, icnode3);
            }
            return icnode1;
        }
        ICNode *head = NULL, *tail = NULL;    // icnode2
        ArgList* curArg = argList;
        while(curArg) {
            ICNode* node = newNode(IC_ARG, curArg->arg);
            if(!head) { head = node; }
            else { tail->next = node; node->prev = tail; }
            tail = node;
            curArg = curArg->next;
        }
        if(!place) place = newTemp();
        ICNode* icnode3 = newNode(IC_CALL_FUNC, place, funcSymbol->name);
        tail->next = icnode3;
        icnode3->prev = tail;
        return link(icnode1, head);
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        ICNode* icnode;
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        assert(funcSymbol != NULL);
        if(!place) place = newTemp();
        if(strcmp("read", funcSymbol->name) == 0)
            icnode = newNode(IC_READ, place);
        else icnode = newNode(IC_CALL_FUNC, place, funcSymbol->name);
        return icnode;
    }
    else if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB) ||
            SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        Operand *t1, *tmem;
        ICNode *icnode1 = NULL, *icnode2 = NULL;
        DataType* type = getExpType(node);
        if(type->kind == DT_BASIC) {
            t1 = newTemp();
            icnode1 = translateLeftExp(node, t1, NULL);
            SET_OPERAND(tmem, OP_TEMP_MEM) tmem->var_no = t1->var_no;
            if(place) icnode2 = newNode(IC_ASSIGN, place, tmem);
            return link(icnode1, icnode2);
        } else {
            if(!place) place = newTemp();
            return translateLeftExp(node, place, NULL);
        }
    }
    else if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        ICNode* icnode = NULL;
        if(place) {
            if(varSymbol->dataType->kind == DT_BASIC)
                icnode = newNode(IC_ASSIGN, place, varSymbol->op);
            else icnode = newNode(IC_ASSIGN, place, getVarAddr(SVAL(node->children[0])));
        }
        return icnode;
    }
    else if(SMTC_PROD_CHECK_1(node, INT)) {
        ICNode* icnode = NULL;
        Operand* value; SET_OPERAND(value, OP_CONST) value->val = IVAL(node->children[0]);
        if(place) icnode = newNode(IC_ASSIGN, place, value);
        return icnode;
    }
    else if(SMTC_PROD_CHECK_1(node, FLOAT)) {
        printTranslateError("Code contains constants of float type");
    }
    else while(0);
    return NULL;
}

ICNode* translateArgs(TreeNode* node, ArgList** argList) {
    Operand* t1;
    ICNode* icnode1 = NULL;
    DataType* type;
    // optimize 
    type = getExpType(node->children[0]);
    t1 = getVarOp(node->children[0]);
    if(!t1 || type->kind != DT_BASIC) {
        if(type->kind == DT_ARRAY || type->kind == DT_STRUCT) {
            if(t1) {
                Operand* t1addr;
                if(t1->kind == OP_VAR_ADDR) { SET_OPERAND(t1addr, OP_VAR) }
                else { SET_OPERAND(t1addr, OP_VAR_ADDR) }
                t1addr->var_no = t1->var_no;
                t1 = t1addr;
            } else {
                t1 = newTemp();
                icnode1 = translateLeftExp(node->children[0], t1, NULL);
            }
        }
        else {  // DT_BASIC
            t1 = newTemp();
            icnode1 = translateExp(node->children[0], t1, NULL);
        }
    }
    ArgList* newArg = (ArgList*)malloc(sizeof(ArgList));
    newArg->arg = t1;
    newArg->next = *argList;
    *argList = newArg;
    if(SMTC_PROD_CHECK_3(node, Exp, COMMA, Args)) {
        ICNode* icnode2 = translateArgs(node->children[2], argList);
        return link(icnode1, icnode2);
    }
    else if(SMTC_PROD_CHECK_1(node, Exp)) {
        return icnode1;
    }
    else while(0);
    return NULL;
}


void printTranslateError(char* msg) {
    translateErrorNum++;
    // only print the first error
    if(translateErrorNum == 1)
        printf("Cannot translate: %s.\n", msg);
}

char* translateOperand(Operand* op) {
    if(op == NULL)
        return NULL;
    char* str = (char*)malloc(sizeof(char)*32);
    switch (op->kind) {
    case OP_VAR:
        sprintf(str, "v%d", op->var_no);
        break;
    case OP_TEMP:
        sprintf(str, "t%d", op->var_no);
        break;
    case OP_CONST:
        sprintf(str, "#%d", op->val);
        break;
    case OP_VAR_ADDR:
        sprintf(str, "&v%d", op->var_no);
        break;
    case OP_TEMP_ADDR:
        sprintf(str, "&t%d", op->var_no);
        break;
    case OP_VAR_MEM:
        sprintf(str, "*v%d", op->var_no);
        break;
    case OP_TEMP_MEM:
        sprintf(str, "*t%d", op->var_no);
        break;
    case OP_LABEL:
        sprintf(str, "label%d", op->label_no);
        break;
    default:
        break;
    }
    return str;
}

void printInterCodes(ICNode* head, FILE* file) {
    ICNode* curNode = head;
    InterCode* code;
    while(curNode) {
        code = curNode->code;
        switch (code->kind) {
        case IC_LABEL: fprintf(file, "LABEL label%d :\n", code->op->label_no); break;
        case IC_FUNC: fprintf(file, "FUNCTION %s :\n", code->func); break;
        case IC_ASSIGN: fprintf(file, "%s := %s\n", translateOperand(code->assign.left), translateOperand(code->assign.right)); break;
        case IC_ADD: fprintf(file, "%s := %s + %s\n", translateOperand(code->binop.result), translateOperand(code->binop.op1), translateOperand(code->binop.op2)); break;
        case IC_SUB: fprintf(file, "%s := %s - %s\n", translateOperand(code->binop.result), translateOperand(code->binop.op1), translateOperand(code->binop.op2)); break;
        case IC_MUL: fprintf(file, "%s := %s * %s\n", translateOperand(code->binop.result), translateOperand(code->binop.op1), translateOperand(code->binop.op2)); break;
        case IC_DIV: fprintf(file, "%s := %s / %s\n", translateOperand(code->binop.result), translateOperand(code->binop.op1), translateOperand(code->binop.op2)); break;
        case IC_GOTO: fprintf(file, "GOTO label%d\n", code->op->label_no); break;
        case IC_IF_GOTO: fprintf(file, "IF %s %s %s GOTO label%d\n", translateOperand(code->ifgoto.op1), code->ifgoto.relop, translateOperand(code->ifgoto.op2), code->ifgoto.label->label_no); break;
        case IC_RETURN: fprintf(file, "RETURN %s\n", translateOperand(code->op)); break;
        case IC_DEC: fprintf(file, "DEC %s %d\n", translateOperand(code->dec.op), code->dec.size); break;
        case IC_ARG: fprintf(file, "ARG %s\n", translateOperand(code->op)); break;
        case IC_CALL_FUNC: fprintf(file, "%s := CALL %s\n", translateOperand(code->call.result), code->call.func); break;
        case IC_PARAM: fprintf(file, "PARAM %s\n", translateOperand(code->op)); break;
        case IC_READ: fprintf(file, "READ %s\n", translateOperand(code->op)); break;
        case IC_WRITE: fprintf(file, "WRITE %s\n", translateOperand(code->op)); break;
        default: break;
        }
        curNode = curNode->next;
    }
}

ICNode* optimize(ICNode* icnode) {
    // eliminate redundant temp variables
    ICNode* cur = icnode;
    InterCode *code1, *code2;
    int count = 0;
    int circleNum = 0;
    while(cur && cur->next) {
    #ifdef SMTC_DEBUG
        fprintf(stderr, "------------%d----------\n", count);
        ICNode* next = cur->next->next;
        cur->next->next = NULL;
        printInterCodes(cur, stderr);
        cur->next->next = next;
    #endif
        code1 = cur->code;
        code2 = cur->next->code;
        if(code2->kind == IC_ASSIGN) {
            switch (code1->kind) {
            case IC_ASSIGN:
                // [t1 := x] + [y := t1] => [y := x]
                if(code1->assign.left == code2->assign.right && 
                   code2->assign.right->kind == OP_TEMP) {
                    code1->assign.left = code2->assign.left;
                    cur->next->next->prev = cur;
                    cur->next = cur->next->next;
                }
                // [t1 := x] + [t2 := *t1] => [t2 := *x] 
                else if(code1->assign.left->var_no == code2->assign.right->var_no && 
                        code1->assign.left->kind == OP_TEMP && code1->assign.right->kind == OP_VAR &&
                        code2->assign.right->kind == OP_TEMP_MEM) {
                    code1->assign.left = code2->assign.left;
                    Operand* newOp;
                    SET_OPERAND(newOp, OP_VAR_MEM) newOp->var_no = code1->assign.right->var_no;
                    code1->assign.right = newOp;
                    cur->next->next->prev = cur;
                    cur->next = cur->next->next;
                }
                else { cur = cur->next; }
                break;
            case IC_ADD:
            case IC_SUB:
            case IC_MUL:
            case IC_DIV:
                // [t1 := x + y] + [z := t1] => [z := x + y]
                if(code1->binop.result == code2->assign.right &&
                   code2->assign.right->kind == OP_TEMP) {
                    code1->binop.result = code2->assign.left;
                    cur->next->next->prev = cur;
                    cur->next = cur->next->next;
                }
                else { cur = cur->next; }
                break;
            default: cur = cur->next; break;
            }
        }
        else if(code1->kind == IC_LABEL && code2->kind == IC_LABEL) {
            changeLabelNo(code1->op->label_no, code2->op->label_no);
            cur->next->next->prev = cur;
            cur->next = cur->next->next;
        }
        // [Label label1 :] + [GOTO label2] => [GOTO label2]
        else if(code1->kind == IC_LABEL && code2->kind == IC_GOTO) {
            changeLabelNo(code1->op->label_no, code2->op->label_no);
            ICNode* next = cur->next;
            cur->prev->next = next;
            next->prev = cur->prev;
            cur = next;
        }
        // [t1 := &x] + [ARG t1] => [ARG &x]
        else if(code1->kind == IC_ASSIGN && code2->kind == IC_ARG &&
                code1->assign.left->kind == OP_TEMP && code1->assign.right->kind == OP_VAR_ADDR &&
                code2->op == code1->assign.left) {
            code2->op = code1->assign.right;
            cur->code = code2;
            cur->next->next->prev = cur;
            cur->next = cur->next->next;
        }
        // [t1 := &v1] + [t2 := t1 + #4] => [t2 := &v1 + #4]
        else if(code1->kind == IC_ASSIGN && code2->kind == IC_ADD &&
                code1->assign.left->kind == OP_TEMP && code1->assign.right->kind == OP_VAR_ADDR &&
                code2->binop.result->kind == OP_TEMP && code2->binop.op1 == code1->assign.left && code2->binop.op2->kind == OP_CONST) {
            code2->binop.op1 = code1->assign.right;
            cur->code = code2;
            cur->next->next->prev = cur;
            cur->next = cur->next->next;
        }
        else { cur = cur->next; }
        count++; 
    }
    return icnode;
}