#include "SymbolTable.h"

HashBucket symbolTable[MAX_ST_SIZE];
StackEle symbolStack[MAX_SS_SIZE];
int stackTop;
extern DataType* intSpecifier;
extern DataType* floatSpecifier;
extern DataType* errorSpecifier;

unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if ((i = val & ~(MAX_ST_SIZE-1))) {
            val = (val ^ (i >> 12)) & (MAX_ST_SIZE-1);
        }
    }
    return val;
}

void init() {
    stackTop = 0;
    memset(symbolTable, 0, MAX_ST_SIZE * sizeof(HashBucket));
    memset(symbolStack, 0, MAX_SS_SIZE * sizeof(StackEle));
    intSpecifier = (DataType*)malloc(sizeof(DataType));
    intSpecifier->kind = DT_BASIC;
    intSpecifier->basic = BASIC_INT;
    floatSpecifier = (DataType*)malloc(sizeof(DataType));
    floatSpecifier->kind = DT_BASIC;
    floatSpecifier->basic = BASIC_FLOAT;
    errorSpecifier = (DataType*)malloc(sizeof(DataType));
}

Symbol* search4Insert(char* name, enum NameSrc ns) {
    int idx = hash_pjw(name);
    Symbol* p = symbolTable[idx];
    if(ns == NS_FUNC) {    
        while(p) {
            if(p->nameSrc == NS_FUNC) 
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_STRUCT) {    
        while(p) {
            if(p->nameSrc == NS_STRUCT || p->nameSrc == NS_GVAR || p->nameSrc == NS_LVAR)
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_GVAR) {    
        while(p) {
            if(p->nameSrc == NS_STRUCT || p->nameSrc == NS_GVAR)   
                return p;
            p = p->next;
        }
        p = symbolStack[0];
        while(p) {
            if(p->nameSrc == NS_STRUCT) {
                Field* cField = p->dataType->structure.fieldList;
                while(cField) {
                    if(strcmp(name, cField->name) == 0)
                        return fieldSymbol;
                    cField = cField->next;
                }
            }
            p = p->stackNext;
        }
    }
    else if(ns == NS_LVAR) {
        p = symbolTable[stackTop];
        while(p) {
            if(p->nameSrc == NS_LVAR)
                return p;
            p = p->next;
        }
    }
    return NULL;
}

Symbol* search4Field(char* name, Symbol* newSymbol) {
    int idx = hash_pjw(name);
    Symbol* p = symbolTable[idx];
    while(p) {
        if(p->nameSrc == NS_GVAR) 
            return p;
        p = p->next;
    }
    p = symbolStack[0];
    while(p) {
        if(p->nameSrc == NS_STRUCT) {
            Field* cField = p->dataType->structure.fieldList;
            while(cField) {
                if(strcmp(name, cField->name) == 0)
                    return p;
                cField = cField->next;
            }
        }
        p = p->stackNext;
    }
    return NULL;
}

void insertSymbol(Symbol* newSymbol) {
    if(newSymbol == NULL)
        return;
    int idx = hash_pjw(newSymbol->name);
    switch(newSymbol->nameSrc) {
    case NS_LVAR:
        newSymbol->next = symbolTable[idx];
        newSymbol->stackNext = symbolStack[stackTop];
        symbolTable[idx] = newSymbol;
        symbolStack[stackTop] = newSymbol;
        break;
    default:
        newSymbol->next = symbolTable[idx];
        newSymbol->stackNext = symbolStack[0];
        symbolTable[idx] = newSymbol;
        symbolStack[0] = newSymbol;
        break;
    }
}

Symbol* search4Use(char* name, enum NameSrc ns) {
    int tableIndex = hash_pjw(name);
    Symbol* p = NULL;
    if(ns == NS_LVAR) {
        int stackIndex = stackTop;
        while(stackIndex >= 1) {
            p = symbolStack[stackIndex];
            while(p) {
                if(p->nameSrc == NS_LVAR && strcmp(name, p->name) == 0)
                    return p;
                p = p->stackNext;
            }
            stackIndex--;
        }
    }
    else if(ns == NS_FUNC) {
        p = symbolTable[tableIndex];
        while(p) {
            if(p->nameSrc == NS_FUNC) 
                return p;
            p = p->next;
        }
        p = symbolTable[tableIndex];
        while(p) {
            if(p->nameSrc == NS_GVAR || p->nameSrc == NS_LVAR) 
                return p;
            p = p->next;
        }
    }
    else {
        if(ns == NS_STRUCT) {
            p = symbolTable[tableIndex];    
        }
        else if(ns == NS_GVAR) {    
            p = symbolTable[0];
        }
        while(p) {
            if(p->nameSrc == ns) 
                return p;
            p = p->next;
        }
    } 
    return NULL;
}