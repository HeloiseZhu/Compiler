#include "SemanticAnalysis.h"
#include "Translate.h"
#include "Assembly.h"

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
extern int smtcErrorNum;
extern TreeNode* root;
extern int yylex();
extern int yyparse();
extern void yyrestart(FILE*);
extern void yyerror(char* msg);


int main(int argc, char** argv) {
	if (argc != 3) 
		return 1;
	FILE* in = fopen(argv[1], "r");
	if (!in) {
		perror(argv[1]);
		return 1;
	}
	//yydebug = 1;
	yyrestart(in);
	yyparse();
	if(lexErrorNum == 0 && stdSyntaxErrorNum == 0) {
	#ifdef SMTC_DEBUG
		printSyntaxTree(root, 0);
		fprintf(stderr, "[SEM DEGUB] Semantic Analysis begins\n");
	#endif
		smtcProgram(root);
		if(smtcErrorNum == 0) {
			ICNode* icnode = translateProgram(root);
			if(translateErrorNum == 0) {
			#ifdef SMTC_DEBUG
				fprintf(out, "[SEM DEGUB] IR before optimizing:\n");
				printInterCodes(icnode, out);
				fprintf(out, "[SEM DEGUB] END\n\n");
			#endif
				FILE* out = fopen(argv[2], "w");
				if (!out) {
					perror(argv[2]);
					return 1;
				}
				// Lab3
				// printInterCodes(optimize(icnode), out);
				// Lab4
				AsmCode* asmCodes = ir2asm(icnode);
				printAsmCodes(asmCodes, out);
				fclose(out);
			}
		}
	}
	else if(stdSyntaxErrorNum != 0) {
	#ifdef DEBUGGING
		fprintf(stderr, "[SYN DEGUB] Miss %d Syntax Error.\n", stdSyntaxErrorNum-syntaxErrorNum);
	#endif
		yyerror("");
	}

	fclose(in);

	return 0;
}