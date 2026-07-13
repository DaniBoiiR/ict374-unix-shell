/** 
 * Name: token.h 
 * Description: Header file tokens for token file. Defines global variables 
 * Date: 22/6/2026 
 */ 

#define MAX_NUM_TOKENS 1000
#define delimiters  " \t\n" // Characters that seperate tokens

int tokenize(char *inputLine, char *token[]);
