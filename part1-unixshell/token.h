/** 
 * Name: token.h 
 * Description: Header file tokens for token file. Defines global variables 
 * Date: 22/6/2026 
 */ 

#define MAX_NUM_TOKENS 1000
#define delimiters  " \t\n" // Characters that seperate tokens
#define COMMAND_LINE_SIZE 100 // Global variable (BAD) for command line size 
#include <glob.h>

int tokenize(char *inputLine, char *token[]);

int expandWildCard(char *token[], char *expandedTokens[], char expandedStorage[][COMMAND_LINE_SIZE], int tokenSize);
