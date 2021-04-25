#include <stdio.h>
#include "SyntaxTree.h"

extern FILE* yyin;
extern int yylineno;
extern int yycolumn;
extern int yydebug;
extern int lexErrorNum;
extern int syntaxErrorNum;
extern int stdSyntaxErrorNum;
extern int prevErrorLine;
extern int prevCfmErrorLine;
extern TreeNode* root;
extern int yylex();
extern int yyparse();
extern void yyrestart(FILE*);
extern void yyerror(char* msg);


int main(int argc, char** argv) {
	if (argc <= 1) 
		return 1;
	FILE* f;
	for(int i = 1; i < argc; i++) {
		lexErrorNum = 0;
		stdSyntaxErrorNum = 0;
		syntaxErrorNum = 0;
		prevErrorLine = 0;
		prevCfmErrorLine = 0;
		yylineno = 1;
		root = NULL;
		f = fopen(argv[i], "r");
		if (!f) {
			perror(argv[1]);
			return 1;
		}
		if(argc > 2) {
			printf("%s:\n", argv[i]);
		}
		//yydebug = 1;
		yyrestart(f);
		yyparse();
		if(lexErrorNum == 0 && stdSyntaxErrorNum == 0) {
			printSyntaxTree(root, 0);
		}
		else if(stdSyntaxErrorNum != 0) {
			yyerror("");
		}
	#ifdef DEBUGGING
		printf("Miss %d Syntax Error.\n", stdSyntaxErrorNum-syntaxErrorNum);
	#endif
	}

	return 0;
}
