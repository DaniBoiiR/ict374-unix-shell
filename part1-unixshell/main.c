/**
* File: main.c 
* Description: Read one line at a time and execute commands
* Date: 6/5/2026
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "token.h"
#include "command.h"
#include "builtin.h"

void saveHistory(char *inputLine, FILE *historyfile);
int executeBuiltIn(Command *cmd, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory);
void executeCommand(Command *command); // Executes external commands (Seperators ; and &)
//int executePipe(Command *command); // Executes Pipes (|)
void claim_children(int sig); // Function to collect zombie processes
void signalHandlerSetup();
  
int main(int argc, char *argv[]){
  // Initialize Variables
  char inputLine[COMMAND_LINE_SIZE];
  char *token[MAX_NUM_TOKENS];
  Command command[MAX_NUM_COMMANDS];
  char prompt[256] = "$ ";

  ignore_interrupts(); // Disables CTRL+C / CTRL+Z / CTRL+\ 
  signalHandlerSetup(); // Register signal handler 
 
  // History initialization 
  FILE *historyfile;
  historyfile = fopen(HISTORY_FILE, "a");
  int reenactingHistory = 0;

  while(1){
    printf("%s", prompt);
     
    if (reenactingHistory == 0) {
      // Breaks out the loop if fgets fails. 
      if(fgets(inputLine, COMMAND_LINE_SIZE, stdin) == NULL) {
        fclose(historyfile);
        break;
      }      
    } else if (reenactingHistory == 1) {
      // reenact_history() sets the input line to the chosen line of history
      reenactingHistory = 0; // allows the user to enter input in the next iteration
    }

    inputLine[strcspn(inputLine, "\n")] = '\0'; // Pattern for removing saved newline
    saveHistory(inputLine, historyfile); // Saves command 
    int tokenSize = tokenize(inputLine, token);
    
    // Checks for error 
    if(tokenSize == -1){
      printf("Too many tokens\n");
      continue; 
    }
    
    // Initalize command array 
    for(int n = 0; n < MAX_NUM_COMMANDS; n++){
      initializeCommandStructure(&command[n]);
    }

    int commandSize = separateCommands(token, command);

    // Skip Empty Command 
    if(commandSize == 0) continue; 

    // Print and execute the commands 
    for(int n = 0; n < commandSize; n++){

      printf("Command %d: ", n+1);

      for(int i = command[n].first; i <= command[n].last; i++){
        printf("%s ", token[i]);
      }
      printf("\n");
      printf("Separator: %s\n", command[n].sep);

      // Execute Built in shell commands. 
      if(executeBuiltIn(&command[n], prompt, historyfile, inputLine, &reenactingHistory)){
        continue; 
      }
      // Execute Unix Shell Commands 
      else{
        executeCommand(&command[n]);
      }
    }
  }
  return 0;
}

// Save History
// TODO: ADD to the built in commands instead of putting this on main 
void saveHistory(char *inputLine, FILE *historyfile){
  // First checks if fputs failed 
  if(fputs(inputLine, historyfile) == EOF ||fputs("\n", historyfile) == EOF){
    perror("Failed to save history");
    return; 
  }
  
  int flush = fflush(historyfile);
  //printf("%d", flush); // Uncomment for debugging
}

// Executes Built in Commands 
int executeBuiltIn(Command *command, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory){
  if(command->argv == NULL || command->argv[0] == NULL) return 0; // Null protection 
  char *cmd = command->argv[0]; 

  // Null protection 
  if(command->argv == NULL || command->argv[0] == NULL) return 0;

  // Exits the program after exit is input. 
  if (strcmp(cmd, "exit") == 0) {
    printf("Exiting shell...\n");
    fclose(historyfile);
    exit(0);
  }

  // Return 1 If commands are Built in 
  if (strcmp(cmd, "pwd") == 0) {
    pwd();
    return 1;
  }

  if (strcmp(cmd, "cd") == 0) {
    cd(command->argv[1]);    
    return 1;
  } 

  if (strcmp(cmd, "walk") == 0) {
    walk(command->argv[1]);
    return 1; 
  } 

  if (strcmp(cmd, "prompt") == 0) {
    change_prompt(prompt, command->argv[1]);
    return 1; 
  }

  if (strcmp(cmd, "history") == 0) {
    history();
    return 1; 
  } 

  if (strcmp(cmd, "clear") == 0) {
    clear(historyfile);
    return 1; 
  } 

  if (strcmp(cmd, "!!") == 0) {
    reenact_history(-1, inputLine, reenactingHistory);
    return 1;
  }

  return 0; // If command is not a Built in shell comamnd 
}

// Executes external Non-Built in Unix commands usign execp 
void executeCommand(Command *command){
  int pid = fork(); // All external commands will be handled by child processes 

  if(pid < 0){ // Fork failure 
    perror("fork");
  }

  if(pid == 0){ // In Child 
    execvp(command->argv[0], command->argv); // Run command in child process 
    perror("execv failed");
    exit(1);
  }

  // In Parent
  // Parent Handling Background processes and & operator
  else{
    if(strcmp(command->sep, "&") == 0){
      return; // Returns back to main loop to execute next command concurrently 
    }

    // Sequential Handling. Shell has to wait until child process is complete
    // After child is complete, shell returns to main loop 
    else{
      waitpid(pid, NULL, 0);
    } 
  }
}

// Claims zombie child processes when child process is finished executing 
void claim_children(int sig){
  pid_t pid = 1; 
  
  // Claim zombied by collecting exit status through waitpid (retrives exit status of specific pid)
  // -1 = Wait for any child process in process group 
  // NULL ignores child exit status
  // WNOHANG = Prevents functions blocking. 
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Sets up signal handler to collect zombie child processes
void signalHandlerSetup(){
  struct sigaction act; 
  act.sa_handler = claim_children; // Assign function pointer for reliable signal
  sigemptyset(&act.sa_mask); // Dont block other signals 
  act.sa_flags = SA_NOCLDSTOP; // Not catch sopped children 
  sigaction(SIGCHLD, &act, NULL); // When a zombie signal is found, claim children is fired by the signal handler 
}
