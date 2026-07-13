/** 
* File: token.c 
* Description: Soruce code for tokenizing commands from user 
* Date: 22/6/2026
*/ 

#include <stdio.h>
#include <string.h> 
#include "token.h"

int tokenize(char* inputLine, char *token[]){
  char *tk; // Current token
  int n = 0; // Current token count/position. 

  // strtok = string tokenizer for C++ | String then Delimiter
  // MAX_NUM_TOKENS anmd delimiters are defined in token.h 
  tk = strtok(inputLine, delimiters);
  token[n] = tk;

  while(tk != NULL){
    // Pre-increment n. 
    ++n; 
    
    // if the current number of seperated tokens exceeds max. Count becomes -1 and returns. 
    if(n >= MAX_NUM_TOKENS){
      n = -1; 
      break;
    }

    // Continues tokenizing from where it left off last. NULL does NOT mean the string passed is null. 
    // Loop ends when current token is null. 
    tk = strtok(NULL, delimiters);
    token[n] = tk;
  }
  
  return n; 
}
