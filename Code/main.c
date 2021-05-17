#include "SemanticAnalysis.h"
#include "Translate.h"

extern FILE* yyin;
extern int yylineno;
extern int yycolumn;
extern int yydebug;
extern int lexErrorNum;
extern int syntaxErrorNum;
extern int stdSyntaxErrorNum;
extern int translateErrorNum;
extern int prevErrorLine;
extern int prevCfmErrorLine;
extern TreeNode* root;
extern int yylex();
extern int yyparse();
extern void yyrestart(FILE*);
extern void yyerror(char* msg);


int main(int argc, char** argv) {
	if (argc != 3) 
		return 1;
	FILE* in;
	FILE* out;
	in = fopen(argv[1], "r");
	out = fopen(argv[2], "w");
	if (!in) {
		perror(argv[1]);
		return 1;
	}
	if (!out) {
		perror(argv[2]);
		return 1;
	}
	//yydebug = 1;
	yyrestart(in);
	yyparse();
	if(lexErrorNum == 0 && stdSyntaxErrorNum == 0) {
	#ifdef SMTC_DEBUG
		//printSyntaxTree(root, 0);
		fprintf(stderr, "[SEM DEGUB] Semantic Analysis begins\n");
	#endif
		smtcProgram(root);
		ICNode* icnode = translateProgram(root);
		if(translateErrorNum == 0) {
		#ifdef SMTC_DEBUG
			fprintf(out, "[SEM DEGUB] IR before optimizing:\n");
			printInterCodes(icnode, out);
			fprintf(out, "[SEM DEGUB] END\n\n");
		#endif
			printInterCodes(optimize(icnode), out);
		}
	}
	else if(stdSyntaxErrorNum != 0) {
	#ifdef DEBUGGING
		fprintf(stderr, "[SYN DEGUB] Miss %d Syntax Error.\n", stdSyntaxErrorNum-syntaxErrorNum);
	#endif
		yyerror("");
	}

	fclose(in);
	fclose(out);

	return 0;
}