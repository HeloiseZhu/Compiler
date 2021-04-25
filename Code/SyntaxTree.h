#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H

#include <stdio.h>

//#define DEBUGGING
// TODO: nodeType to string
#define NT2STR(x) \
	nodeTypeString[x]
#define IVAL(x) \
	x->nodeValue.intVal
#define FVAL(x) \
	x->nodeValue.floatVal
#define SVAL(x) \
	x->nodeValue.textVal

static const char* nodeTypeString[49] = {
	// Lex
	"SEMI", "COMMA", "DOT",
	"ASSIGNOP", "RELOP",
	"PLUS", "MINUS", "STAR", "DIV",
	"AND", "OR", "NOT",
	"TYPE", "INT", "FLOAT",
	"LP", "RP", "LB", "RB", "LC", "RC",
	"STRUCT", "RETURN", "IF", "ELSE", "WHILE",
	"ID",
	// High-level Definitions
	"Program", "ExtDefList", "ExtDef", "ExtDecList",
	// Specifiers
	"Specifier", "StructSpecifier", "OptTag", "Tag",
	// Declarators
	"VarDec", "FunDec", "VarList", "ParamDec",
	// Statements
	"CompSt", "StmtList", "Stmt",
	// Local Definitions
	"DefList", "Def", 
	"DecList", "Dec",
	// Expressions
	"Exp", "Args",
	// Error
	"error"
};

typedef enum {
	/* Lex */
	NT_SEMI, NT_COMMA, NT_DOT,
	NT_ASSIGNOP, NT_RELOP,
	NT_PLUS, NT_MINUS, NT_STAR, NT_DIV,
	NT_AND, NT_OR, NT_NOT,
	NT_TYPE, NT_INT, NT_FLOAT,
	NT_LP, NT_RP, NT_LB, NT_RB, NT_LC, NT_RC,
	NT_STRUCT, NT_RETURN, NT_IF, NT_ELSE, NT_WHILE, 
	NT_ID,
	/* Syntax */
	/* High-level Definitions */
	NT_Program, NT_ExtDefList, NT_ExtDef, NT_ExtDecList,
	/* Specifiers */
	NT_Specifier, NT_StructSpecifier, NT_OptTag, NT_Tag,
	/* Declarators */
	NT_VarDec, NT_FunDec, NT_VarList, NT_ParamDec,
	/* Statements */
	NT_CompSt, NT_StmtList, NT_Stmt,
	/* Local Definitions */
	NT_DefList, NT_Def, 
	NT_DecList, NT_Dec,
	/* Expressions */
	NT_Exp, NT_Args, 
	/* Error */
	NT_ERROR
} NodeType;


typedef union {
	int intVal;
	float floatVal;
	char textVal[32];
} NodeValue;

typedef struct Node {
	NodeType nodeType;
	NodeValue nodeValue;
	int lineno;
	int childrenNum;
	struct Node* parent;
	//struct Node* nextSibling;
	struct Node** children;
} TreeNode;


TreeNode* createNode(NodeType type, int lineno);
void addChildNode(TreeNode* parent, int count, ...);
void printSyntaxTree(TreeNode* root, int depth);

#endif	// SYNTAXTREE_H