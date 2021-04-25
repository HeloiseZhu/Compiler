%{
	#include <string.h>
	#include "lex.yy.c"
	#include "SyntaxTree.h"
	//#define YYSTYPE TreeNode*

	TreeNode* root;
	int syntaxErrorNum = 0;
	int stdSyntaxErrorNum = 0;
	int prevErrorLine = 0;
	int prevCfmErrorLine = 0;
	char* errorText = NULL;
	
	void yyerror(char* msg);
	void printSyntaxError(int lineno, char* msg);
%}

%locations

/* declared types */
%union {
	TreeNode* type_node;
}

/* declared tokens */
%token <type_node> SEMI COMMA DOT
%token <type_node> RELOP ASSIGNOP
%token <type_node> PLUS MINUS STAR DIV
%token <type_node> NOT AND OR
%token <type_node> LP RP LB RB LC RC
%token <type_node> STRUCT RETURN IF ELSE WHILE
%token <type_node> TYPE ID
%token <type_node> INT FLOAT

/* declared non-terminals */
%type <type_node> Program ExtDefList ExtDef ExtDecList
%type <type_node> Specifier StructSpecifier OptTag Tag
%type <type_node> VarDec FunDec VarList ParamDec
%type <type_node> CompSt StmtList Stmt
%type <type_node> DefList Def DecList Dec
%type <type_node> Exp Args

/* operator precedence */
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
// %right NEG 
%left DOT
%left LB RB
%left LP RP
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

/* High-level Definitions */
Program : 
	ExtDefList {
		$$ = createNode(NT_Program, @$.first_line);
		addChildNode($$, 1, $1);
		root = $$;
	}
	;
ExtDefList : 
	ExtDef ExtDefList {
		$$ = createNode(NT_ExtDefList, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| /* empty */ {
		$$ = createNode(NT_ExtDefList, @$.first_line);
	}
	| ExtDef error ExtDefList {
		printSyntaxError(@2.first_line, "expecting correct global variable or function defination"); 
		$$ = createNode(NT_ExtDefList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}
	;
ExtDef : 
	Specifier ExtDecList SEMI {
		$$ = createNode(NT_ExtDef, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Specifier SEMI {
		$$ = createNode(NT_ExtDef, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| Specifier FunDec CompSt {
		$$ = createNode(NT_ExtDef, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	/*| Specifier ExtDecList error {
		// [s/r]: VarDec error
		printSyntaxError(@3.first_line, "expecting \";\" or correct variable declaration"); 
		$$ = createNode(NT_ExtDef, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Specifier error {
		// [s/r]: error LP RP
		printSyntaxError(@2.first_line, "expecting \";\""); 
		$$ = createNode(NT_ExtDef, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}*/
	| Specifier error SEMI {
		printSyntaxError(@2.first_line, "expecting correct defination"); 
		$$ = createNode(NT_ExtDef, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}
	| error SEMI {
		printSyntaxError(@1.first_line, "expecting correct defination"); 
		$$ = createNode(NT_ExtDef, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 2, errNode, $2);
	}
	;
ExtDecList : 
	VarDec {
		$$ = createNode(NT_ExtDecList, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| VarDec COMMA ExtDecList {
		$$ = createNode(NT_ExtDecList, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| VarDec error {
		// [usage]: initialize global variable
		printSyntaxError(@2.first_line, "expecting \";\" or correct variable declaration"); 
		$$ = createNode(NT_ExtDecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	| error COMMA ExtDecList {
		printSyntaxError(@1.first_line, "expecting correct variable declaration"); 
		$$ = createNode(NT_ExtDecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 3, errNode, $2, $3);
	}
	| VarDec error COMMA ExtDecList {
		printSyntaxError(@2.first_line, "expecting \",\" or correct variable declaration"); 
		$$ = createNode(NT_ExtDecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 4, $1, errNode, $3, $4);
	}
	;

/* Specifiers */
Specifier : 
	TYPE {
		$$ = createNode(NT_Specifier, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| StructSpecifier {
		$$ = createNode(NT_Specifier, @$.first_line);
		addChildNode($$, 1, $1);
	}
	;
StructSpecifier : 
	STRUCT OptTag LC DefList RC {
		$$ = createNode(NT_StructSpecifier, @$.first_line);
		addChildNode($$, 5, $1, $2, $3, $4, $5);
	}
	| STRUCT Tag {
		$$ = createNode(NT_StructSpecifier, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| STRUCT error LC DefList RC {
		// [usage]: struct struct() {}
		printSyntaxError(@2.first_line, "expecting correct struct tag"); 
		$$ = createNode(NT_StructSpecifier, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 5, $1, errNode, $3, $4, $5);
	}
	| STRUCT OptTag LC error {
		printSyntaxError(@4.first_line, "expecting \"}\""); 
		$$ = createNode(NT_StructSpecifier, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 4, $1, $2, $3, errNode);
	}
	/*| error Tag {
		// TODO
		printSyntaxError(@1.first_line, "expecting specifier \"struct\""); 
		$$ = createNode(NT_StructSpecifier, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 2, errNode, $2);
	}*/
	;
OptTag : 
	ID {
		$$ = createNode(NT_OptTag, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| /* empty */ {
		$$ = createNode(NT_OptTag, @$.first_line);
	}
	;
Tag : 
	ID {
		$$ = createNode(NT_Tag, @$.first_line);
		addChildNode($$, 1, $1);
	}
	;

/* Declarators */
VarDec : 
	ID {
		$$ = createNode(NT_VarDec, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| VarDec LB INT RB {
		$$ = createNode(NT_VarDec, @$.first_line);
		addChildNode($$, 4, $1, $2, $3, $4);
	}
	| VarDec LB error RB {
		printSyntaxError(@3.first_line, "expecting int between \"[\" and \"]\""); 
		$$ = createNode(NT_VarDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 4, $1, $2, errNode, $4);
	}
	/*| VarDec error LB INT RB {
		printSyntaxError(@2.first_line, "expecting correct declaration"); 
		$$ = createNode(NT_VarDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 5, $1, errNode, $3, $4, $5);
	}*/
	;
FunDec : 
	ID LP VarList RP {
		$$ = createNode(NT_FunDec, @$.first_line);
		addChildNode($$, 4, $1, $2, $3, $4);
	}
	| ID LP RP {
		$$ = createNode(NT_FunDec, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	/*| ID LP VarList error {
		printSyntaxError(@4.first_line, "expecting \")\""); 
		$$ = createNode(NT_FunDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 4, $1, $2, $3, errNode);
	}*/
	| ID LP error RP {
		printSyntaxError(@3.first_line, "expecting correct parameter defination"); 
		$$ = createNode(NT_FunDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 4, $1, $2, errNode, $4);
	}
	| error LP VarList RP {
		// [usage]: int if()
		printSyntaxError(@1.first_line, "expecting correct function identifier"); 
		$$ = createNode(NT_FunDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 4, errNode, $2, $3, $4);
	}
	| error LP RP {
		printSyntaxError(@1.first_line, "expecting correct function identifier"); 
		$$ = createNode(NT_FunDec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 3, errNode, $2, $3);
	}
	;
VarList : 
	ParamDec COMMA VarList {
		$$ = createNode(NT_VarList, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| ParamDec {
		$$ = createNode(NT_VarList, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| ParamDec error {
		printSyntaxError(@2.first_line, "expecting \",\" or correct  parameter defination"); 
		$$ = createNode(NT_VarList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	| error COMMA VarList {
		printSyntaxError(@1.first_line, "expecting correct parameter defination"); 
		$$ = createNode(NT_VarList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 3, errNode, $2, $3);
	}
	| ParamDec error COMMA VarList {
		printSyntaxError(@2.first_line, "expecting \",\" or correct parameter defination"); 
		$$ = createNode(NT_VarList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 4, $1, errNode, $3, $4);
	}
	| ParamDec COMMA error {
		printSyntaxError(@3.first_line, "expecting correct parameter defination"); 
		$$ = createNode(NT_VarList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	;
ParamDec : 
	Specifier VarDec {
		$$ = createNode(NT_ParamDec, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	;

/* Statements */
CompSt : 
	LC DefList StmtList RC {
		$$ = createNode(NT_CompSt, @$.first_line);
		addChildNode($$, 4, $1, $2, $3, $4);
	}
	/*| error RC {
		// [s/r]: DefList and StmtList can be empty
		printSyntaxError(@2.first_line, "expecting p
		printSyntaxError(@1.first_line, "expecting paired braces or correct statements between braces"); 
		$$ = createNode(NT_CompSt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 2, errNode, $2);
	}
	| LC error {
		// [s/r]: DefList and StmtList can be empty
		printSyntaxError(@2.first_line, "expecting paired braces or correct statements between braces"); 
		$$ = createNode(NT_CompSt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}*/
	;
StmtList : 
	Stmt StmtList {
		$$ = createNode(NT_StmtList, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| Stmt error StmtList {
		// [usage]:
		printSyntaxError(@2.first_line, "expecting correct statements"); 
		$$ = createNode(NT_StmtList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}
	| /* empty */ {
		$$ = createNode(NT_StmtList, @$.first_line);
	}
	;
Stmt : 
	Exp SEMI {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| CompSt {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| RETURN Exp SEMI {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 5, $1, $2, $3, $4, $5);
	}
	| IF LP Exp RP Stmt ELSE Stmt {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 7, $1, $2, $3, $4, $5, $6, $7);
	}
	| WHILE LP Exp RP Stmt {
		$$ = createNode(NT_Stmt, @$.first_line);
		addChildNode($$, 5, $1, $2, $3, $4, $5);
	}
	| error SEMI {
		printSyntaxError(@1.first_line, "expecting correct expression or return statement"); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 2, errNode, $2);
	}
	| Exp error {
		// [usage]:
		printSyntaxError(@2.first_line, "expecting \";\" or correct expression"); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	/*| Exp error SEMI {
		// TODO
		printSyntaxError(@2.first_line, "expecting correct expression"); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}*/
	| RETURN Exp error {
		printSyntaxError(@3.first_line, "expecting \";\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| IF LP Exp error Stmt %prec LOWER_THAN_ELSE {
		printSyntaxError(@4.first_line, "expecting \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 5, $1, $2, $3, errNode, $5);
	}
	| IF LP error RP Stmt %prec LOWER_THAN_ELSE {
		printSyntaxError(@3.first_line, "expecting correct expression between \"(\" and \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 5, $1, $2, errNode, $4, $5);
	}
	| IF LP Exp error Stmt ELSE Stmt {
		printSyntaxError(@4.first_line, "expecting \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 7, $1, $2, $3, errNode, $5, $6, $7);
	}
	| IF LP error RP Stmt ELSE Stmt {
		printSyntaxError(@3.first_line, "expecting correct expression between \"(\" and \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 7, $1, $2, errNode, $4, $5, $6, $7);
	}
	/*| IF LP Exp RP error ELSE Stmt {
		printSyntaxError(@5.first_line, "expecting \";\" or \"}\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @5.first_line);
		addChildNode($$, 7, $1, $2, $3, $4, errNode, $6, $7);
	}*/
	| WHILE LP Exp error Stmt {
		printSyntaxError(@4.first_line, "expecting \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 5, $1, $2, $3, errNode, $5);
	}
	| WHILE LP error RP Stmt {
		printSyntaxError(@3.first_line, "expecting \")\""); 
		$$ = createNode(NT_Stmt, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 5, $1, $2, errNode, $4, $5);
	}
	;

/* Local Definitions */
DefList : 
	Def DefList {
		$$ = createNode(NT_DefList, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| /* empty */ {
		$$ = createNode(NT_DefList, @$.first_line);
	}
	/*| Def error DefList {
		printSyntaxError(@2.first_line, "expecting correct local variable definations"); 
		$$ = createNode(NT_DefList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}*/
	;
Def : 
	Specifier DecList SEMI {
		$$ = createNode(NT_Def, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Specifier error SEMI {
		printSyntaxError(@2.first_line, "expecting correct variable declaration"); 
		$$ = createNode(NT_Def, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}
	/*| Specifier DecList error SEMI {
		printSyntaxError(@3.first_line, "expecting correct variable declaration"); 
		$$ = createNode(NT_Def, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 4, $1, $2, errNode, $4);
	}
	| error DecList SEMI {
		printSyntaxError(@1.first_line, "expecting correct specifier"); 
		$$ = createNode(NT_Def, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 3, errNode, $2, $3);
	}*/
	;
DecList : 
	Dec {
		$$ = createNode(NT_DecList, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| Dec COMMA DecList {
		$$ = createNode(NT_DecList, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| error COMMA DecList {
		printSyntaxError(@1.first_line, "expecting correct local variable defination"); 
		$$ = createNode(NT_DecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 3, errNode, $2, $3);
	}
	| Dec error {
		printSyntaxError(@2.first_line, "expecting \",\" or correct local variable defination"); 
		$$ = createNode(NT_DecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	| Dec COMMA error {
		printSyntaxError(@3.first_line, "expecting correct local variable defination"); 
		$$ = createNode(NT_DecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Dec error COMMA DecList {
		printSyntaxError(@2.first_line, "expecting \",\" or correct local variable defination"); 
		$$ = createNode(NT_DecList, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 4, $1, errNode, $3, $4);
	}
	;
Dec : 
	VarDec {
		$$ = createNode(NT_Dec, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| VarDec ASSIGNOP Exp {
		$$ = createNode(NT_Dec, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| VarDec ASSIGNOP error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"=\""); 
		$$ = createNode(NT_Dec, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	;

/* Expressions */
Exp : 
	Exp ASSIGNOP Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp AND Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp OR Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp RELOP Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp PLUS Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp MINUS Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp STAR Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp DIV Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| LP Exp RP {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| MINUS Exp {
		// TODO
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| NOT Exp {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 2, $1, $2);
	}
	| ID LP Args RP {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 4, $1, $2, $3, $4);
	}
	| ID LP RP {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp LB Exp RB {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 4, $1, $2, $3, $4);
	}
	| Exp DOT ID {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| ID {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| INT {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| FLOAT {
		$$ = createNode(NT_Exp, @$.first_line);
		addChildNode($$, 1, $1);
	}
	| Exp ASSIGNOP error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"=\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	/*| Exp error ASSIGNOP Exp {
		printSyntaxError(@2.first_line, "expecting correct expression"); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 4, $1, errNode, $3, $4);
	}*/
	| Exp AND error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"&&\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp OR error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"||\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp RELOP error {
		printSyntaxError(@3.first_line, "expecting complete expression"); //TODO
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp PLUS error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"+\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp MINUS error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"-\"(SUB)"); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp STAR error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"*\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| Exp DIV error {
		printSyntaxError(@3.first_line, "expecting complete expression after \"/\""); 
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| LP error RP {
		printSyntaxError(@2.first_line, "expecting complete expression between \"(\" and \")\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3);
	}
	/*| error RP {
		printSyntaxError(@2.first_line, "expecting \"(\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @1.first_line);
		addChildNode($$, 2, errNode, $2);
	}*/
	| LP Exp error {
		printSyntaxError(@3.first_line, "expecting \")\" or correct expression between \"(\" and \")\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	| MINUS error {
		printSyntaxError(@2.first_line, "expecting complete expression after \"-\"(NEG)");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	| NOT error {
		printSyntaxError(@2.first_line, "expecting complete expression after \"!\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 2, $1, errNode);
	}
	| ID LP error {
		printSyntaxError(@3.first_line, "expecting \")\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	/*| ID LP Args error {
		printSyntaxError(@4.first_line, "expecting \")\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @4.first_line);
		addChildNode($$, 4, $1, $2, $3, errNode);
	}*/
	| Exp LB error {
		printSyntaxError(@3.first_line, "expecting \"]\"");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}
	/*| Exp DOT error {
		printSyntaxError(@3.first_line, "expecting correct field name");
		$$ = createNode(NT_Exp, @$.first_line);
		//printf("1ERROR: %d\n", (int)$3);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}*/
	;
Args : 
	Exp COMMA Args {
		$$ = createNode(NT_Args, @$.first_line);
		addChildNode($$, 3, $1, $2, $3);
	}
	| Exp error COMMA Args {
		printSyntaxError(@2.first_line, "expecting \",\" or correct parameters");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @2.first_line);
		addChildNode($$, 3, $1, errNode, $3, $4);
	}
	/*| Exp COMMA error {
		printSyntaxError(@3.first_line, "expecting correct parameters when calling a function");
		$$ = createNode(NT_Exp, @$.first_line);
		TreeNode* errNode = createNode(NT_ERROR, @3.first_line);
		addChildNode($$, 3, $1, $2, errNode);
	}*/
	| Exp {
		$$ = createNode(NT_Args, @$.first_line);
		addChildNode($$, 1, $1);
	}
	;

%%

void printSyntaxError(int lineno, char* msg) {
	if(lineno <= prevCfmErrorLine)
		return;
	prevCfmErrorLine = lineno;
	syntaxErrorNum++;
	char* str = strtok(errorText, "\n");
	if(str) {
		errorText = str;
	}	
	fprintf(stderr, "Error type B at Line %d: syntax error near \"%s\", %s.\n", 
			lineno, errorText, msg);
}

void yyerror(char* msg) {
	if(prevCfmErrorLine < prevErrorLine) {
		char* str = strtok(errorText, "\n");
		if(str) {
			errorText = str;
		}
		fprintf(stderr, "Error type B at Line %d: syntax error near \"%s\", undefined error, maybe expecting \"}\" or correct if-else statements.\n", 
				prevErrorLine, errorText);
		prevCfmErrorLine = prevErrorLine;
	}
	stdSyntaxErrorNum++;
	prevErrorLine = yylineno;
	errorText = yytext;
#ifdef DEBUGGING
	fprintf(stderr,"[bison]Error type B at Line %d: %s, near \"%s\"\n", yylineno, msg, yytext);
#endif
}
