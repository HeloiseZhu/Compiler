#ifndef SYMBOL_H
#define SYMBOL_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "InterCode.h"

#define MAX_ST_SIZE 16384

typedef struct DataType_ DataType;
typedef struct FuncData_ FuncData;
typedef struct Field_ Field;
typedef struct Symbol_ Symbol;
typedef Field Param;
typedef Symbol* HashBucket;
typedef Symbol* StackEle;


struct DataType_ {
    enum { DT_BASIC, DT_ARRAY, DT_STRUCT } kind;
    union {
        enum { BASIC_INT, BASIC_FLOAT } basic;
        struct { 
            DataType* elem; 
            int size; 
        } array;
        struct {
            Field* fieldList;
            Field* tail;
        } structure;
    };
};

struct Field_ {
    char* name;
    DataType* dataType;
    Field* next;
};

struct FuncData_ {
    DataType* retType;
    Field* paramList;
    Field* tail;
};

enum NameSrc { NS_FUNC, NS_STRUCT, NS_GVAR, NS_LVAR };

struct Symbol_ {
    char* name;
    enum NameSrc nameSrc;
    union {
        DataType* dataType; // NS_STRUCT & NS_GVAR & NS_LVAR
        FuncData* funcData; // NS_FUNC
    };
    struct Operand_* op;    // NS_LVAR
    Symbol* next;
    Symbol* stackNext;
};


unsigned int hash_pjw(char* name);
void init();
Symbol* search4Insert(char* name, enum NameSrc ns);
Symbol* search4Field(char* name);
void insertSymbol(Symbol* newSymbol);
Symbol* search4Use(char* name, enum NameSrc ns);

void stackPop();
void stackPush();
void printSymbolStack();
void printSymbolTable();

int getsizeof(DataType* type);
int getFieldOffset(DataType* specifier, char* field, DataType** fieldType);

#endif