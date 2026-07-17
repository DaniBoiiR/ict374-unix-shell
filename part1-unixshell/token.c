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

// BUG: potential overflow with over 500 files if found. add edge case check 
int expandWildCard(char *token[], char *expandedTokens[], char expandedStorage[][COMMAND_LINE_SIZE], int tokenSize){
  int count = 0; 

  // Exit if empty token 
  if(tokenSize == 0) return 0;

  for(int n = 0; n < tokenSize; n++){
    
    // If wild card is found 
    if((strchr(token[n], '*')) != NULL || strchr(token[n], '?') != NULL){
      
      // results store Count of matched paths, List of matched pathnames, and Slots reserve in gl_pathv
      glob_t results; 
      int status = glob(token[n], 0, NULL, &results); // Glob searches matching patterns 

      // If glob returns successful/matches found, start token expansion 
      if(status == 0){
        for(int i = 0; i < results.gl_pathc; i++){
            
          // Handle overflow 
          if(count >= MAX_NUM_TOKENS -1){
            globfree(&results);
            return -1;
          }

          // Copy into expanded tokens array 
          strcpy(expandedStorage[count], results.gl_pathv[i]);
          expandedTokens[count] = expandedStorage[count]; // gl_pathv stores file names 
          count++; 
        }
      } 
       
      // If no matches found, keep original token
      else {
        strcpy(expandedStorage[count], token[n]);
        expandedTokens[count] = expandedStorage[count];
        count++; 
      }
      
      globfree(&results); // Frees dynamically allocated memory 
    } 

    // Normal Tokens 
    else{
      if(count >= MAX_NUM_TOKENS -1){
        return -1;
      }
      strcpy(expandedStorage[count], token[n]);
      expandedTokens[count] = expandedStorage[count];
      count++; 
    }
  }

  expandedTokens[count] = NULL; // null terminator 
  return count; 
}
