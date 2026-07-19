// File: command.c 
// Description; Separates a list of tokens into a sequence of commands.
// Assumptions:		any two successive commands in the list of tokens are separated 
//			by one of the following command separators:
//				"|"  - pipe to the next command
//				"&"  - shell does not wait for the proceeding command to terminate
//				";"  - shell waits for the proceeding command to terminate
// Date: 23/6/2026

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "command.h"

// Return 1 if the token is a command separator
// Return 0 otherwise
// This function is used for understanding where a command ends and a new command begins  
int separator(char *token){
  int i=0;
  // Specific seperators defined in header file 
  char *commandSeparators[] = {pipeSep, conSep, seqSep, NULL};

  // Checks if the token is a seperator. Returns 1 if it is. 
  while (commandSeparators[i] != NULL) {
      if (strcmp(commandSeparators[i], token) == 0) {
          return 1; 
      } 
      ++i;
  }
  
  return 0;
}

// Initialize the command struct before use. 
void initializeCommandStructure(Command *cp){
  cp->first = 0;
  cp->last = 0;
  cp->sep = 0; 
  cp->argv = NULL;
  cp->stdin_file = NULL;
  cp->stdout_file = NULL; 
  cp->stderr_file = NULL; 
  cp->stdout_mode = '\0'; 
  cp->stderr_mode = '\0'; 
}

// Filles the command struct with command details 
void fillCommandStructure(Command *cp, int first, int last, char *sep){
  cp->first = first; // index to the first token in the command 
  cp->last = last - 1; // index to the last token in the command 
  cp->sep = sep;  // command seperator (| or & or ;)
}

// Takes an Array of tokens and a pointer to a single command structure (cp)
// Scans the array from token[cp->first] to token[cp->last]
// Finds redirection symbols for input and output. Assigns next token as file name 
// Only 1 < and > per command 
void searchRedirection(char *token[], Command *cp){
  int index;

  // Scans token array to find input redirection symbol 
  for(index = cp->first; index <= cp->last; index++){
    // Find Input redirect
    if((strcmp(token[index], "<")) == 0){
      cp->stdin_file = token[index + 1];
    }

    // Find output (write) redirect 
    if((strcmp(token[index], ">")) == 0){
      cp->stdout_file = token[index + 1];
      cp->stdout_mode = 'w';
    }

    // Find error (write) redirect 
    if((strcmp(token[index], "2>")) == 0){
      cp->stderr_file = token[index + 1];
      cp->stderr_mode = 'w';
    }

    // Find output (append) redirect 
    if((strcmp(token[index], ">>")) == 0){
      cp->stdout_file = token[index + 1];
      cp->stdout_mode = 'a';
    }

    // Find error (append) redirect 
    if((strcmp(token[index], "2>>")) == 0){
      cp->stderr_file = token[index + 1];
      cp->stderr_mode = 'a';
    }
  }

  return;
}

// Builds a command line argument vector for execvp function 
void buildCommandArgumentArray(char *token[], Command *cp){
  int n = (cp->last - cp->first) + 1; // NUm of tokens in command. 
  
  // Reallocate memory for argument vector
  cp->argv = (char **) realloc(cp->argv, sizeof(char *) * (n + 1));
  if(cp->argv == NULL){
    perror("realloc");
    exit(1);
  }

  // Building the Argument vector 
  int i; // Input index for reading the token array 
  int k = 0; // Output index for inserting into argv array 
  
  for(int i = cp->first; i <= cp->last; i++){
    if(strcmp(token[i], ">") == 0 || strcmp(token[i], "<") == 0 || strcmp(token[i], ">>") == 0 || strcmp(token[i], "2>") == 0 || strcmp(token[i], "2>>") == 0){
      ++i; // Skips the input/output redirection 
    } else{ 
      cp->argv[k] = token[i]; // Insert current element to argv command array 
      ++k; 
    }
  }

  cp->argv[k] = NULL; // Add null terminator 
  return; 
}

// Seperates the list of tokens into a sequence of commands
// All commands are stored in a commands array 
// token[] contains tokenized command line but each command line
// command is a command struct array which contains the full commands. 
int separateCommands(char *token[], Command command[]){
  int i; // Token array index 
  int nTokens; // Total Number of tokens 

  // find out the number of tokens until null terminator 
  i = 0;
  while (token[i] != NULL) ++i; 
  nTokens = i;

  // if empty command line
  if (nTokens == 0) 
    return 0;

  // Checks if the first token is a seperator. Returns negative int (error). 
  if (separator(token[0]))
    return -3;

  // Checks if the last token is not a seperator. 
  // If token is not a seperator, appends the token with semicolon. 
  if (!separator(token[nTokens-1])) {
    token[nTokens] = seqSep;
    ++nTokens;
  }
      
  int first=0;   // points to the first tokens in the current command
  int last;      // points to the last tokens in the current command
  char *sep;     // command separator at the end of current command
  int c = 0;     // command index / No. of commands 
  
  // Scan every token in the token array. 
  // Parse every command inside of the token array 
  for (i=0; i < nTokens; ++i){
    last = i;

    // Checks if the current token (assigned to last variable) is a seperator
    if (separator(token[i])) {
      sep = token[i];

      // Error if two consecutive seperators.
      if (first==last)
        return -2;

      // Command struct is filled for specific command index. 
      // Command information is stored inside of the command array. 
      // Function updates the command array by reference.  
      fillCommandStructure(&(command[c]), first, last, sep);
      ++c;
      first = i+1; 
    }
  }

  // check the last token of the last command  
  if (strcmp(token[last], pipeSep) == 0) { // last token is pipe separator
      return -4; 
  } 

  // calculate the number of commands
  int nCommands = c;

  // Searches redirection for each command before returning no. of commands to user. 
  for(int n = 0; n < nCommands; n++){
    searchRedirection(token, &(command[n]));
    buildCommandArgumentArray(token, &(command[n]));
  }

  return nCommands; 
}


