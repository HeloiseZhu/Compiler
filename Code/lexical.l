%{	
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "SyntaxTree.h"
	#include "syntax.tab.h"

	#define YY_USER_ACTION \
		yylloc.first_line = yylloc.last_line = yylineno; \
		yylloc.first_column = yycolumn; \
		yylloc.last_column = yycolumn + yyleng - 1; \
		yycolumn += yyleng;
	#ifdef DEBUGGING
	#define LEX_DEBUG(x) \
		fprintf(stderr, "[%3d] %15s: %s\n", yylineno, #x, yytext);
	#else
	#define LEX_DEBUG(x) \
		while(0);
	#endif

	int yycolumn = 1;
	int lexErrorNum = 0;
	void printLexError(char* msg);
%}
%option yylineno

digit		[0-9]
letter		[_a-zA-Z]
SEMI		";"
COMMA		","
ASSIGNOP	"="
RELOP		">"|"<"|">="|"<="|"=="|"!="
PLUS 		"+"
MINUS 		"-"
STAR 		"*"
DIV 		"/"
AND 		"&&"
OR 			"||"
DOT 		"."
NOT 		"!"
TYPE 		int|float
LP 			"("
RP 			")"
LB 			"["
RB 			"]"
LC 			"{"
RC 			"}"
STRUCT 		struct
RETURN 		return
IF 			if
ELSE 		else
WHILE 		while
INT_ERROR	0+(0|[1-9][0-9]*)
INT			0|[1-9][0-9]*
FLOAT		{digit}+\.{digit}+|({digit}+|{digit}+\.|\.{digit}+|{digit}+\.{digit}+)[Ee][+-]?{digit}+
FLOAT_ERROR	{digit}+\.|\.{digit}+|({digit}+|{digit}+\.|\.{digit}+|{digit}+\.{digit}+)[Ee][+-]?({digit}*\.{digit}*)?|\.[Ee][+-]{digit}+
ID			{letter}({letter}|{digit})*
HEX_ERROR	(0x|0X)?[a-fA-F0-9]+
WHITESPACE  [ \t\r]+

%%
{SEMI}			{	yylval.type_node = createNode(NT_SEMI, yylineno);
					LEX_DEBUG(SEMI)
					return SEMI;
				}
{COMMA}			{	yylval.type_node = createNode(NT_COMMA, yylineno);
					LEX_DEBUG(COMMA)
					return COMMA;
				}
{ASSIGNOP}		{	yylval.type_node = createNode(NT_ASSIGNOP, yylineno);
					LEX_DEBUG(ASSIGNOP)
					return ASSIGNOP;
				}
{RELOP}			{	yylval.type_node = createNode(NT_RELOP, yylineno);
					strcpy(yylval.type_node->nodeValue.textVal, yytext);
					LEX_DEBUG(RELOP)
					return RELOP;
				}
{PLUS} 			{	yylval.type_node = createNode(NT_PLUS, yylineno);
					LEX_DEBUG(PLUS)
					return PLUS;
				}
{MINUS} 		{	yylval.type_node = createNode(NT_MINUS, yylineno);
					LEX_DEBUG(MINUS)
					return MINUS;
				}
{STAR} 			{	yylval.type_node = createNode(NT_STAR, yylineno);
					LEX_DEBUG(STAR)
					return STAR;
				}
{DIV} 			{	yylval.type_node = createNode(NT_DIV, yylineno);
					LEX_DEBUG(DIV)
					return DIV;
				}
{AND} 			{	yylval.type_node = createNode(NT_AND, yylineno);
					LEX_DEBUG(AND)
					return AND;
				}
{OR} 			{	yylval.type_node = createNode(NT_OR, yylineno);
					LEX_DEBUG(OR)
					return OR;
				}
{DOT} 			{	yylval.type_node = createNode(NT_DOT, yylineno);
					LEX_DEBUG(DOT)
					return DOT;
				}
{NOT} 			{	yylval.type_node = createNode(NT_NOT, yylineno);
					LEX_DEBUG(NOT)
					return NOT;
				}
{TYPE} 			{	yylval.type_node = createNode(NT_TYPE, yylineno);
					strcpy(yylval.type_node->nodeValue.textVal, yytext);
					LEX_DEBUG(TYPE)
					return TYPE;
				}
{LP} 			{	yylval.type_node = createNode(NT_LP, yylineno);
					LEX_DEBUG(LP)
					return LP;
				}
{RP} 			{	yylval.type_node = createNode(NT_RP, yylineno);
					LEX_DEBUG(RP)
					return RP;
				}
{LB} 			{	yylval.type_node = createNode(NT_LB, yylineno);
					LEX_DEBUG(LB)
					return LB;
				}
{RB} 			{	yylval.type_node = createNode(NT_RB, yylineno);
					LEX_DEBUG(RB)
					return RB;
				}
{LC} 			{	yylval.type_node = createNode(NT_LC, yylineno);
					LEX_DEBUG(LC)
					return LC;
				}
{RC} 			{	yylval.type_node = createNode(NT_RC, yylineno);
					LEX_DEBUG(RC)
					return RC;
				}
{STRUCT} 		{	yylval.type_node = createNode(NT_STRUCT, yylineno);
					LEX_DEBUG(STRUCT)
					return STRUCT;
				}
{RETURN} 		{	yylval.type_node = createNode(NT_RETURN, yylineno);
					LEX_DEBUG(RETURN)
					return RETURN;
				}
{IF} 			{	yylval.type_node = createNode(NT_IF, yylineno);
					LEX_DEBUG(IF)
					return IF;
				}
{ELSE} 			{	yylval.type_node = createNode(NT_ELSE, yylineno);
					LEX_DEBUG(ELSE)
					return ELSE;
				}
{WHILE} 		{	yylval.type_node = createNode(NT_WHILE, yylineno);
					LEX_DEBUG(WHILE)
					return WHILE;
				}
{INT_ERROR}		{	printLexError("illegal decimal number");
					yylval.type_node = createNode(NT_INT, yylineno);
					yylval.type_node->nodeValue.intVal = 0;
					//sscanf(yytext, "%d", &(yylval.type_node->nodeValue.intVal));
					LEX_DEBUG(INT)
					return INT;
				}
{INT}			{	yylval.type_node = createNode(NT_INT, yylineno);
					sscanf(yytext, "%d", &(yylval.type_node->nodeValue.intVal));
					LEX_DEBUG(INT)
					return INT;
				}
{FLOAT}			{	yylval.type_node = createNode(NT_FLOAT, yylineno);
					sscanf(yytext, "%f", &(yylval.type_node->nodeValue.floatVal));
					LEX_DEBUG(FLOAT)
					return FLOAT;
				}
{FLOAT_ERROR}	{	printLexError("illegal float number");
					yylval.type_node = createNode(NT_FLOAT, yylineno);
					yylval.type_node->nodeValue.floatVal = 0;
					//sscanf(yytext, "%f", &(yylval.type_node->nodeValue.floatVal));
					LEX_DEBUG(FLOAT)
					return FLOAT;
				}
{ID}			{	yylval.type_node = createNode(NT_ID, yylineno);
					strcpy(yylval.type_node->nodeValue.textVal, yytext);
					LEX_DEBUG(ID)
					return ID;
				}
{HEX_ERROR}		{	printLexError("illegal decimal number");
					yylval.type_node = createNode(NT_INT, yylineno);
					yylval.type_node->nodeValue.intVal = 0;
					//sscanf(yytext, "%d", &(yylval.type_node->nodeValue.intVal));
					LEX_DEBUG(INT)
					return INT;
				}
\n				{	yycolumn = 1;}
{WHITESPACE} 	{	}
. 				{	printLexError("");}

%%

void printLexError(char* msg) {
	lexErrorNum++;
	if(strlen(msg) == 0) {
		printf("Error type A at Line %d: Mysterious characters \"%s\".\n",
			   yylineno, yytext);
	}
	else {
		printf("Error type A at Line %d: %s \"%s\".\n",
			   yylineno, msg, yytext);
	}
}