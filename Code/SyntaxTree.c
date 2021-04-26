#include <stdarg.h>     // Uncertain parameters
#include <stdlib.h>
#include "SyntaxTree.h"

TreeNode* createNode(NodeType type, int lineno) {
#ifdef DEBUGGING
    fprintf(stderr, "[SYN DEGUB] NEW NODE: %s\n", NT2STR(type));
#endif    
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->nodeType = type;
    node->parent = NULL;
    node->children = NULL;
    node->lineno = lineno;
    node->childrenNum = 0;
}

void addChildNode(TreeNode* parent, int count, ...) {
    if(count <= 0)
        return;
    va_list argList;
    va_start(argList, count);
    TreeNode* currentChild = NULL;
    parent->children = (TreeNode**)malloc(count * sizeof(TreeNode*));
    for (int i = 0; i < count; i++) {
        currentChild = va_arg(argList, TreeNode*);
        currentChild->parent = parent;
        (parent->children)[i] = currentChild;
    }
    va_end(argList);
    parent->lineno = parent->children[0]->lineno;
    parent->childrenNum = count;
}

void printSyntaxTree(TreeNode* root, int depth) {
    // Pre-order
    if(root == NULL || depth < 0)
        return;
    // parent
    if(root->nodeType <= NT_ID) {
        // Lex
        printf("%*s", depth * 2, "");
        switch(root->nodeType){
            case NT_ID: 
                printf("%s: %s\n", NT2STR(root->nodeType), SVAL(root)); 
                break;
            case NT_TYPE: 
                printf("%s: %s\n", NT2STR(root->nodeType), SVAL(root)); 
                break;
            case NT_INT: 
                printf("%s: %d\n", NT2STR(root->nodeType), IVAL(root)); 
                break;
            case NT_FLOAT:
                printf("%s: %f\n", NT2STR(root->nodeType), FVAL(root)); 
                break;
            default:
                printf("%s\n", NT2STR(root->nodeType)); 
                break;
        }
    }
    else if(root->nodeType < NT_ERROR) {
        // Syntax
        if(root->childrenNum != 0) {
            printf("%*s", depth * 2, "");
            printf("%s (%d)\n", NT2STR(root->nodeType), root->lineno);
            // children
            for(int i = 0; i < root->childrenNum; i++) {
                printSyntaxTree(root->children[i], depth + 1);
            }
        }
    }
}