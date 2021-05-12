#include "InterCode.h"

int tempVarNum = 0;
int labelNum = 0;
int varNum = 0;
Operand *zeroOp, *oneOp;
int translateError;

void initIC() {
    SET_OPERAND(zeroOp, OP_CONST)
    zeroOp->val = 0;
    SET_OPERAND(oneOp, OP_CONST)
    oneOp->val = 1;
}

Operand* newVar() {
    Operand* t;
    SET_OPERAND(t, OP_VAR)
    t->var_no = varNum;
    varNum++;
    return t;
}

Operand* newTemp() {
    Operand* t;
    SET_OPERAND(t, OP_TEMP)
    t->var_no = tempVarNum;
    tempVarNum++;
    return t;
}

Operand* newLabel() {
    Operand* t;
    SET_OPERAND(t, OP_LABEL)
    t->label_no = labelNum;
    labelNum++;
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
        node->code->ifgoto.relop = va_arg(argList, Operand*);
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

ICNode* placeAssign(Operand* place, Operand* right) {
    ICNode* node;
    SET_ICNODE(node)
    node->code->kind = IC_ASSIGN;
    node->code->assign.left = place;
    node->code->assign.right = right;
    return node;
}


ICNode* translateProgram(TreeNode* node) {
    initIC();
    if(SMTC_PROD_CHECK_1(node, ExtDefList)) {    
        return translateExtDefList(node->children[0]);
    }
    else assert(0);
    return NULL;
}

ICNode* translateExtDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, ExtDef, ExtDefList)) {
        ICNode* icnode1 = translateExtDef(node->children[0]);
        ICNode* icnode2 = translateExtDefList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else assert(0);
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
    else assert(0);
    return NULL;
}

ICNode* translateVarDec(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        return newNode(IC_DEC, varSymbol->op, getsizeof(varSymbol->dataType));
    }
    else if(SMTC_PROD_CHECK_4(node, VarDec, LB, INT, RB)) {
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            return translateVarDec(node->children[0]);
        }
        else {
            printTranslateError("Code contains variables of multi-dimensional array type");
        }
    }
    else assert(0);
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
    else assert(0);
    return NULL;
}

ICNode* translateCompSt(TreeNode* node) {
    if(SMTC_PROD_CHECK_4(node, LC, DefList, StmtList, RC)) {
        ICNode* icnode1 = translateDefList(node->children[1]);
        ICNode* icnode2 = translateStmtList(node->children[2]);
        return link(icnode1, icnode2);
    }
    else assert(0);
    return NULL;
}

ICNode* translateStmtList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Stmt, StmtList)) {
        ICNode* icnode1 = translateStmt(node->children[0]);
        ICNode* icnode2 = translateStmtList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else assert(0);
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
        Operand* t1 = newTemp();
        ICNode* icnode1 = translateExp(node->children[1], t1, NULL);
        ICNode* icnode2 = newNode(IC_RETURN, t1);
        return link(icnode1, icnode2);
    }
    else if(SMTC_PROD_CHECK_5(node, IF, LP, Exp, RP, Stmt)) {
        Operand* label1 = newLabel();
        Operand* label2 = newLabel();
        ICNode* icnode1 = translateCond(node->children[2], label1, label2);
        ICNode* icnode2 = translateStmt(node->children[4]);
        ICNode* icnode1_1 = newNode(IC_LABEL, label1);
        ICNode* icnode2_1 = newNode(IC_LABEL, label2);
        return link(link(icnode1, icnode1_1), link(icnode2, icnode2_1));
    }
    else if(SMTC_PROD_CHECK_7(node, IF, LP, Exp, RP, Stmt, ELSE, Stmt)) {
        Operand* label1 = newLabel();
        Operand* label2 = newLabel();
        Operand* label3 = newLabel();
        ICNode* icnode1 = translateCond(node->children[2], label1, label2);
        ICNode* icnode2 = translateStmt(node->children[4]);
        ICNode* icnode3 = translateStmt(node->children[6]);
        ICNode* icnode1_1 = newNode(IC_LABEL, label1);
        ICNode* icnode2_1 = newNode(IC_GOTO, label3);
        ICNode* icnode2_2 = newNode(IC_LABEL, label2);
        ICNode* icnode3_1 = newNode(IC_LABEL, label3);
        icnode1 = link(icnode1, icnode1_1);
        icnode2 = link(icnode2, link(icnode2_1, icnode2_2));
        icnode3 = link(icnode3, icnode3_1);
        return link(icnode1, link(icnode2, icnode3));
    }
    else if(SMTC_PROD_CHECK_5(node, WHILE, LP, Exp, RP, Stmt)) {
        Operand* label1 = newLabel();
        Operand* label2 = newLabel();
        Operand* label3 = newLabel();
        ICNode* icnode1 = translateCond(node->children[2], label2, label3);
        ICNode* icnode2 = translateStmt(node->children[4]);
        ICNode* icnode0 = newNode(IC_LABEL, label1);
        ICNode* icnode1_1 = newNode(IC_LABEL, label2);
        ICNode* icnode2_1 = newNode(IC_GOTO, label1);
        ICNode* icnode2_2 = newNode(IC_LABEL, label3);
        icnode1 = link(icnode1, icnode1_1);
        icnode2 = link(icnode2, link(icnode2_1, icnode2_2));
        return link(icnode0, link(icnode1, icnode2));
    }
    else assert(0);
    return NULL;
}

ICNode* translateDefList(TreeNode* node) {
    if(SMTC_PROD_CHECK_2(node, Def, DefList)) {
        ICNode* icnode1 = translateDef(node->children[0]);
        ICNode* icnode2 = translateDefList(node->children[1]);
        return link(icnode1, icnode2);
    }
    else if(node->childrenNum == 0) { /* empty */ }
    else assert(0);
    return NULL;
}

ICNode* translateDef(TreeNode* node) {
    if(SMTC_PROD_CHECK_3(node, Specifier, DecList, SEMI)) {
        return translateDecList(node->children[1]);
    }
    else assert(0);
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
    else assert(0);
    return NULL;
}

ICNode* translateDec(TreeNode* node) {
    if(SMTC_PROD_CHECK_1(node, VarDec)) {
        return translateVarDec(node->children[0]);
    }
    else if(SMTC_PROD_CHECK_3(node, VarDec, ASSIGNOP, Exp)) {
        assert(SMTC_PROD_CHECK_1(node->children[0], ID));
        Operand* t1 = newTemp();
        ICNode* icnode1 = translateExp(node->children[2], t1, NULL);
        // TODO: optimize const
        Symbol* varSymbol = search4Use(SVAL(node->children[0]->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        ICNode* icnode2 = newNode(IC_ASSIGN, varSymbol->op, t1);
        return link(icnode1, icnode2);
    }
    else assert(0);
    return NULL;
}

ICNode* translateCond(TreeNode* node, Operand* label_true, Operand* label_false) {
    if(SMTC_PROD_CHECK_3(node, Exp, RELOP, Exp)) {
        Operand *t1, *t2, *relop;
        ICNode *icnode1, *icnode2, *icnode3, *icnode4;
        t1 = newTemp();
        t2 = newTemp();
        icnode1 = translateExp(node->children[0], t1, NULL);
        icnode2 = translateExp(node->children[2], t2, NULL);
        SET_OPERAND(relop, OP_RELOP)
        relop->relop = SVAL(node->children[1]);
        icnode3 = newNode(IC_IF_GOTO, relop, t1, t2, label_true);
        icnode4 = newNode(IC_GOTO, label_false);
        icnode3->next = icnode4;
        icnode4->prev = icnode3;
        return link(icnode1, link(icnode2, icnode3));
    }
    else if(SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        return translateCond(node->children[1], label_false, label_true);
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp)) {
        Operand* label1 = newLabel();
        ICNode* icnode1 = translateCond(node->children[0], label1, label_false);
        ICNode* icnode2 = translateCond(node->children[2], label_true, label_false);
        ICNode* icnode1_1 = newNode(IC_LABEL, label1);
        return link(icnode1, link(icnode1_1, icnode2));
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, OR, Exp)) {
        Operand* label1 = newLabel();
        ICNode* icnode1 = translateCond(node->children[0], label_true, label1);
        ICNode* icnode2 = translateCond(node->children[2], label_true, label_false);
        ICNode* icnode1_1 = newNode(IC_LABEL, label1);
        return link(icnode1, link(icnode1_1, icnode2));
    }
    else {
        // TODO: other cases are?
        Operand* relop;
        Operand* t1 = newTemp();
        ICNode* icnode1 = translateExp(node, t1, NULL);
        SET_OPERAND(relop, OP_RELOP)
        relop->relop = (char*)malloc(sizeof(char)*4);
        strcpy(relop->relop, "!=");
        ICNode* icnode2 = newNode(IC_IF_GOTO, relop, t1, zeroOp, label_true);
        ICNode* icnode3 = newNode(IC_GOTO, label_false);
        return link(icnode1, link(icnode2, icnode3));
    }
}

// return: [place := &var + #offset]
// TODO: offset == 0?
ICNode* translateLeftExp(TreeNode* node, Operand* place, DataType** param) {
    if(SMTC_PROD_CHECK_1(node, ID)) {
        Operand *vaddr, *offset;
        ICNode* icnode;
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        if(varSymbol->op->kind == OP_ADDR) {
            SET_OPERAND(vaddr, OP_VAR)
            vaddr->var_no = varSymbol->op->var_no;
        }
        else {
            SET_OPERAND(vaddr, OP_ADDR)
            vaddr->var_no = varSymbol->op->var_no;
        }
        SET_OPERAND(offset, OP_CONST)
        offset->val = 0;
        icnode = newNode(IC_ADD, place, vaddr, offset);
        *param = varSymbol->dataType;
        return icnode;
    }
    else if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB)) {
        DataType* param;
        Operand *t0, *t1, *t2, *size;
        ICNode *icnode1, *icnode2, *icnode3, *icnode4;
        t0 = newTemp();
        icnode1 = translateLeftExp(node->children[0], t0, &param);
        t1 = newTemp();
        icnode2 = translateExp(node->children[2], t1, NULL);
        t2 = newTemp();
        SET_OPERAND(size, OP_CONST)
        size->val = getsizeof(param);
        icnode3 = newNode(IC_MUL, t2, t1, size);
        icnode4 = newNode(IC_ADD, place, t0, t2);
        icnode3->next = icnode4;
        icnode4->prev = icnode3;
        return link(link(icnode1, icnode2), icnode3);
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        DataType* param;
        Operand *t0, *t1, *offset;
        ICNode *icnode1, *icnode2;
        t0 = newTemp();
        icnode1 = translateLeftExp(node->children[0], t0, &param);
        t1 = newTemp();
        SET_OPERAND(offset, OP_CONST)
        offset->val = getFieldOffset(param, SVAL(node->children[2]));
        icnode2 = newNode(IC_ADD, place, t0, offset);
        return link(icnode1, icnode2);
    }
    else assert(0);
    return NULL;
}

// usage: translate right Exp
ICNode* translateExp(TreeNode* node, Operand* place, DataType** param) {
    if(SMTC_PROD_CHECK_3(node, Exp, ASSIGNOP, Exp)) {
        Operand* t1 = newTemp();    // TODO: optimize if t1 is CONST
        ICNode *icnode1, *icnode2_1, *icnode2_2;
        icnode1 = translateExp(node->children[2], t1, NULL);
        if(SMTC_PROD_CHECK_1(node->children[0], ID)) {
            Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
            assert(varSymbol != NULL);
            if(varSymbol->op->kind == OP_ADDR) {
                Operand* vmem;
                SET_OPERAND(vmem, OP_VAR_MEM)
                vmem->var_no = varSymbol->op->var_no;
                icnode2_1 = newNode(IC_ASSIGN, vmem, t1);
                if(place) {
                    icnode2_2 = placeAssign(place, vmem);
                    icnode2_1->next = icnode2_2;
                    icnode2_2->prev = icnode2_1;
                }
            }
            else {
                icnode2_1 = newNode(IC_ASSIGN, varSymbol->op, t1);
                if(place) {
                    icnode2_2 = placeAssign(place, varSymbol->op);
                    icnode2_1->next = icnode2_2;
                    icnode2_2->prev = icnode2_1;
                }
            }
        }
        else {  // Exp LB Exp RB | Exp DOT ID
            Operand* t2 = newTemp();
            ICNode* icnode2 = translateLeftExp(node->children[0], t2, NULL);
            Operand* t2mem;
            SET_OPERAND(t2mem, OP_TEMP_MEM)
            t2mem->var_no = t2->var_no;
            icnode2_1 = newNode(IC_ASSIGN, t2mem, t1);
            if(place) {
                icnode2_2 = placeAssign(place, t2mem);
                icnode2_1->next = icnode2_2;
                icnode2_2->prev = icnode2_1;
            }
        }
        return link(icnode1, icnode2_1);
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, AND, Exp) || SMTC_PROD_CHECK_3(node, Exp, OR, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, RELOP, Exp) || SMTC_PROD_CHECK_2(node, NOT, Exp)) {
        Operand *label1, *label2;
        ICNode *icnode0, *icnode1, *icnode2_1, *icnode2_2, *icnode3;
        label1 = newLabel();
        label2 = newLabel();
        icnode0 = newNode(IC_ASSIGN, place, zeroOp);
        icnode1 = translateCond(node, label1, label2);
        icnode2_1 = newNode(IC_LABEL, label1);
        icnode2_2 = placeAssign(place, oneOp);
        icnode3 = newNode(IC_LABEL, label2);
        LINK_ICNODE(icnode2_1, icnode2_2, icnode3);
        icnode1 = link(icnode1, icnode2_1);
        icnode0->next = icnode1;
        icnode1->prev = icnode0;
        return icnode0;
    }
    else if(SMTC_PROD_CHECK_3(node, Exp, PLUS, Exp) || SMTC_PROD_CHECK_3(node, Exp, MINUS, Exp) ||
            SMTC_PROD_CHECK_3(node, Exp, STAR, Exp) || SMTC_PROD_CHECK_3(node, Exp, DIV, Exp)) {
        Operand *t1, *t2;
        ICNode *icnode1, *icnode2, *icnode3;
        t1 = newTemp();
        t2 = newTemp();
        icnode1 = translateExp(node->children[0], t1, NULL);
        icnode2 = translateExp(node->children[2], t2, NULL);
        icnode3 = newNode(IC_ADD, place, t1, t2);
        if(!place)
            icnode3->code->binop.result = newTemp();
        switch (node->children[1]->nodeType) {
        case NT_PLUS:
            icnode3->code->kind = IC_ADD;
            break;
        case NT_MINUS:
            icnode3->code->kind = IC_SUB;
            break;
        case NT_STAR:
            icnode3->code->kind = IC_MUL;
            break;
        case NT_DIV:
            icnode3->code->kind = IC_DIV;
            break;
        default:
            break;
        }
        return link(icnode1, link(icnode2, icnode3));
    }
    else if(SMTC_PROD_CHECK_3(node, LP, Exp, RP)) {
        return translateExp(node->children[1], place, NULL);
    }
    else if(SMTC_PROD_CHECK_2(node, MINUS, Exp)) {
        Operand *t1;
        ICNode *icnode1, *icnode2;
        t1 = newTemp();
        icnode1 = translateExp(node->children[1], t1, NULL);
        icnode2 = newNode(IC_SUB, place, zeroOp, t1);
        if(!place)
            icnode2->code->binop.result = newTemp();
        return link(icnode1, icnode2);
    }
    else if(SMTC_PROD_CHECK_4(node, ID, LP, Args, RP)) {
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        assert(funcSymbol != NULL);
        ArgList* argList = NULL;
        ICNode* icnode1 = translateArgs(node->children[2], &argList);
        if(strcmp("write", funcSymbol->name)) {
            ICNode *icnode2, *icnode3;
            icnode2 = newNode(IC_WRITE, argList->arg);
            icnode3 = newNode(IC_ASSIGN, place, zeroOp);
            LINK_ICNODE(icnode1, icnode2, icnode3)
            return icnode1;
        }
        ICNode* tail = icnode1;    // icnode2
        ArgList* curArg = argList;
        while(curArg) {
            ICNode* node = newNode(IC_ARG, curArg->arg);
            tail->next = node;
            node->prev = tail;
            tail = node;
            curArg = curArg->next;
        }
        ICNode* icnode3 = newNode(IC_CALL_FUNC, place, funcSymbol->name);
        tail->next = icnode3;
        icnode3->prev = tail;
        return icnode1;
    }
    else if(SMTC_PROD_CHECK_3(node, ID, LP, RP)) {
        ICNode* icnode;
        Symbol* funcSymbol = search4Use(SVAL(node->children[0]), NS_FUNC);
        assert(funcSymbol != NULL);
        if(strcmp("read", funcSymbol->name))
            icnode = newNode(IC_READ, place);
        else
            icnode = newNode(IC_CALL_FUNC, place, funcSymbol->name);
        return icnode;
    }
    else if(SMTC_PROD_CHECK_4(node, Exp, LB, Exp, RB) ||
            SMTC_PROD_CHECK_3(node, Exp, DOT, ID)) {
        Operand *t1, *tmem;
        ICNode *icnode1, *icnode2;
        t1 = newTemp();
        icnode1 = translateLeftExp(node, t1, NULL);
        // assign
        SET_OPERAND(tmem, OP_TEMP_MEM)
        tmem->var_no = t1->var_no;
        icnode2 = newNode(IC_ASSIGN, place, tmem);
        return link(icnode1, icnode2);
    }
    else if(SMTC_PROD_CHECK_1(node, ID)) {
        Symbol* varSymbol = search4Use(SVAL(node->children[0]), NS_LVAR);
        assert(varSymbol != NULL);
        ICNode* icnode = NULL;
        if(varSymbol->op->kind == OP_ADDR) {
            Operand* vmem;
            SET_OPERAND(vmem, OP_VAR_MEM)
            vmem->var_no = varSymbol->op->var_no;
            if(place)
                icnode = placeAssign(place, vmem);
        }
        else {
            if(place)
                icnode = placeAssign(place, varSymbol->op);
        }
        return icnode;
    }
    else if(SMTC_PROD_CHECK_1(node, INT)) {
        ICNode* icnode = NULL;
        Operand* value;
        // TODO: optimize const
        SET_OPERAND(value, OP_CONST)
        value->val = IVAL(node->children[0]);
        if(place)
            icnode = placeAssign(place, value);
        return icnode;
    }
    else if(SMTC_PROD_CHECK_1(node, FLOAT)) {
        printTranslateError("Code contains constants of float type")
    }
    else assert(0);
    return NULL;
}

ICNode* translateArgs(TreeNode* node, ArgList** argList) {
    if(SMTC_PROD_CHECK_3(node, Exp, COMMA, Args)) {
        Operand* t1 = newTemp();
        ICNode* icnode1 = translateExp(node->children[0], t1, NULL);
        ArgList* newArg = (ArgList*)malloc(sizeof(ArgList));
        newArg->arg = t1;
        newArg->next = *argList;
        *argList = newArg;
        ICNode* icnode2 = translateArgs(node->children[2], &newArg);
        icnode1->next = icnode2;
        icnode2->prev = icnode1;
        return icnode1;
    }
    else if(SMTC_PROD_CHECK_1(node, Exp)) {
        Operand* t1 = newTemp();
        ICNode* icnode = translateExp(node->children[0], t1, NULL);
        ArgList* newArg = (ArgList*)malloc(sizeof(ArgList));
        newArg->arg = t1;
        newArg->next = *argList;
        *argList = newArg;
        return icnode;
    }
    else assert(0);
    return NULL;
}

void printTranslateError(char* msg) {
    translateError++;
    printf("Cannot translate: %s.\n", msg);
}