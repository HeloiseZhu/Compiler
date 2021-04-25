#include "SymbolTable.h"
#include "SemanticAnalysis.h"

HashBucket symbolTable[MAX_ST_SIZE];
StackEle symbolStack[MAX_SS_SIZE];
int stackTop;
extern DataType* intSpecifier;
extern DataType* floatSpecifier;
extern DataType* errorSpecifier;
extern Symbol* fieldSymbol;
extern Symbol* errorSymbol;

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
    fieldSymbol = (Symbol*)malloc(sizeof(Symbol));
    errorSymbol = (Symbol*)malloc(sizeof(Symbol));
}

Symbol* search4Insert(char* name, enum NameSrc ns) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "Search for Insert: name: %s, ns: %d\n", name, ns);
    //printSymbolStack();
#endif
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
        p = symbolStack[stackTop];
        while(p) {
            if(p->nameSrc == NS_LVAR && strcmp(name, p->name) == 0)
                return p;
            p = p->stackNext;
        }
        p = symbolTable[idx];
        while(p) {
            if(p->nameSrc == NS_STRUCT)
                return p;
            p = p->next;
        }
    }
    return NULL;
}

Symbol* search4Field(char* name) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "Search for Insert: name: %s, ns: Field\n", name);
    //printSymbolStack();
#endif
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
                    return fieldSymbol;
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
#ifdef SMTC_DEBUG
    fprintf(stderr, "Insert Symbol: name: %s\n", newSymbol->name);
    printSymbolStack();
#endif
}

Symbol* search4Use(char* name, enum NameSrc ns) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "Search for Use: name: %s, ns: %d\n", name, ns);
    //printSymbolStack();
#endif
    int tableIndex = hash_pjw(name);
    Symbol* p = NULL;
    if(ns == NS_FUNC || ns == NS_STRUCT || ns == NS_GVAR) {
        p = symbolTable[tableIndex];
        while(p) {
            if(p->nameSrc == ns) 
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_LVAR) {
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
    return NULL;
}

void clearStackTop() {
    if(stackTop <= 0 || stackTop >= MAX_SS_SIZE)
        return;
    Symbol* head = symbolStack[stackTop];
    Symbol* p = NULL;
    Symbol* tmp = NULL;
    int idx;
    while(head) {
        // table
        idx = hash_pjw(head->name);
        p = symbolTable[idx];
        if(p == head) {
            symbolTable[idx] = p->next;
        }
        else {
            while(p) {
                if(p->next && p->next == head) {
                    p->next = p->next->next;
                    break;
                }
                p = p->next;
            }
        }
        // stack
        tmp = head->stackNext;
        head->stackNext = NULL;
        head = tmp;
    }
    symbolStack[stackTop] = NULL;
#ifdef SMTC_DEBUG
    fprintf(stderr, "Clear Stack Top\n");
    printSymbolStack();
#endif
}

void printSymbolStack() {
    int idx = stackTop;
    Symbol* p;
    fprintf(stderr, "------------------------------\n");
    while(idx >= 0) {
        p = symbolStack[idx];
        fprintf(stderr, "stack %d:\n", idx);
        while(p) {
            switch(p->nameSrc) {
            case NS_FUNC:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "FUNC");
                break;
            case NS_STRUCT:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "STRUCT");
                break;
            case NS_GVAR:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "GVAR");
                break;
            case NS_LVAR:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "LVAR");
                break;
            default:
                fprintf(stderr, "\tname: %s\n", p->name);
                break;
            }
            p = p->stackNext;
        }
        idx--;
    }
    fprintf(stderr, "------------------------------\n");
}