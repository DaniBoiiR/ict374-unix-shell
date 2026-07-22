#include "execute.h"

// Executes external Non-Built in Unix commands usign execp 
void executeCommand(Command *command){
  int pid = fork(); // All external commands will be handled by child processes 
  
  if(pid == 0){ // In Child 
    runProgram(command);
  }
  
  if(pid < 0){ // Fork failure 
    perror("fork");
    return;
  }

  // In Parent
  // Parent Handling Background processes and & operator
  else{
    if(strcmp(command->sep, "&") != 0){
      // Sequential Handling. Shell has to wait until child process is complete
      // After child is complete, shell returns to main loop 
      waitpid(pid, NULL, 0);
    }
  }
}

// Conducts the process of running the program inside of child fork 
void runProgram(Command *command){
  // Input Redirection
  if (command->stdin_file != NULL) {
    redirectstdin(command->stdin_file);
  }

  // Output Redirection
  if (command->stdout_file != NULL) {
    redirectstdout(command->stdout_file, command->stdout_mode);
  }

  // Error Redirection
  if (command->stderr_file != NULL) {
    redirectstderr(command->stderr_file, command->stderr_mode);
  }
  
  execvp(command->argv[0], command->argv); // Run command in child process 
  perror("execv failed");
  exit(1); 
} 

// Execute Pipe Unix commands 
int executePipe(Command commands[], int first, int last){
  int p[2]; // 0 for read | 1 for write
  pid_t pid; 
  int input = STDIN_FILENO; // stores read end of previous pipe 

  // Fork each command into a seperate child process 
  for(int n = first; n <= last; n++){
    
    // Create pipes for each command 
    if(n != last){
      if(pipe(p) < 0){
        perror("Pipe Call"); return -1; 
      }
    }

    pid = fork();

    if(pid < 0){
      perror("fork"); 
      return -1;
    }

    // Process for each pipes 
    if(pid == 0){
      // Connect previous pipe to input 
      if(input != STDIN_FILENO){
        dup2(input, STDIN_FILENO);
        close(input);
      }

      // Connect stdout to the next pipe 
      if(n != last){
        dup2(p[1], STDOUT_FILENO);
        close(p[0]);
        close(p[1]);
      }

      runProgram(&commands[n]);
    }

    // In Parent, Close pipe ends 
    if(pid > 0){
      if(input != STDIN_FILENO){
        close(input);
      }

      // Save Input 
      if(n != last){
        close(p[1]); // Close write end
        input = p[0]; // Save input 
      }
    }
  }

  // Wait for the pipeline commands to finish 
  for(int n = first; n <= last; n++){
    wait(NULL);
  }

  return 0;
}

// Executes Built in Commands 
int executeBuiltIn(Command *command, char prompt[], FILE *historyfile, char *inputLine, int *reenactingHistory){
  // Null protection 
  if(command->argv == NULL || command->argv[0] == NULL) return 0;
  char *cmd = command->argv[0]; 

  // Exits the program after exit is input. 
  if (strcmp(cmd, "exit") == 0) {
    printf("Exiting shell...\n");
    fclose(historyfile);
    exit(0);
  }

  if (command->stdin_file != NULL) {
    redirectstdin(command->stdin_file);
  }

  if (command->stdout_file != NULL) {
    redirectstdout(command->stdout_file, command->stdout_mode);
  }

  if (command->stderr_file != NULL) {
    redirectstderr(command->stderr_file, command->stderr_mode);
  }

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
    change_prompt(prompt, command->argv, command->last);
    return 1; 
  }

  if (strcmp(cmd, "history") == 0) {
    history(historyfile);
    return 1; 
  }

  // Prints the Debug code
  if(strcmp(cmd, "debug") == 0 && command->argv[1] != NULL){
    if(strcmp(command->argv[1], "on") == 0){
      return 3; // Turn Debug mode on 
    }

    if(strcmp(command->argv[1], "off") == 0){
      return 4; // Turn Debug mode off 
    }

    return 0;  
  }

  // Executing line of code from history
  if (strncmp(cmd, "!", 1) == 0) {

    // Executing previous command 
    if (strcmp(cmd, "!!") == 0) {
      getLineOfHistory(historyfile, -1, inputLine);
      *reenactingHistory = 1;
    }

    // Executes line of code from history with corresponding history index 
    else if (isdigit(cmd[1])) { // Extract the number after the !
      int strnum = 0;
      for (int i = 1; i < strlen(cmd) && isdigit(cmd[i]); i++) { // Iterate over the characters after !
        if (isdigit(cmd[i])) { // If the character is a digit
          char digit[2] = {cmd[i], '\0'}; // Create a C string with just that digit
          strnum = strnum * 10 + atoi(digit); // Convert the C string to a number and add it to strnum
        }
      }
      getLineOfHistory(historyfile, strnum, inputLine);
      *reenactingHistory = 1;
    }

    // Executes the last line of code from history beginning with a certain substring
    else if (isalpha(cmd[1])) {
        char strsearch[strlen(cmd)];
        for (int i = 1; i < strlen(cmd) && isalpha(cmd[i]); i++) {
            if (isalpha(cmd[i])) {
                strsearch[i - 1] = (char)cmd[i]; // Add a character from the cmd string to a temporary string
                strsearch[i] = '\0'; // Terminate the string with a null character
            }
        }
        getLineOfHistoryByString(historyfile, strsearch, inputLine);
        *reenactingHistory = 1;
    }
    return 2;
  }

  return 0; // If command is not a Built in shell comamnd 
}


