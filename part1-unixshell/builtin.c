#include "builtin.h"

void pwd() {
    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    if (cwd == NULL) {
        perror("getcwd failed");
        return;
    }
    printf("%s\n", cwd);
}

void cd(char * path) {
    if (path == NULL) {
        return;
    } else if (chdir(path) != 0) {
        perror("cd");
    }
    return;
}

void walk(char * path) {
    if (path == NULL) {
        if (chdir(getenv("HOME")) != 0) {
            perror("cd");
        }
        return;
    } else if (chdir(path) != 0) {
        perror("cd");
    }
    return;
}

// Bug: history() will not work if the pwd is not where the history file already is
void history() {
    FILE *historyfile;
    historyfile = fopen(HISTORY_FILE, "r");
    char line[COMMAND_LINE_SIZE];
    int lineNumber = 1;
    while (fgets(line, sizeof(line), historyfile)) {
        printf("%d  %s", lineNumber, line);
        lineNumber++;
    }
    fclose(historyfile);
}

// lineNumberToReenact == -1 refers to the last line of history 
// Only !! is implemented at the moment
void reenact_history(int lineNumberToReenact, char* inputLine, int* reenactingHistory) {
    FILE *historyfile;
    historyfile = fopen(HISTORY_FILE, "r");
    char line[COMMAND_LINE_SIZE];
    int lineNumber = 1;
    while (fgets(line, sizeof(line), historyfile)) {
        if (lineNumber == lineNumberToReenact) {
            strncpy(inputLine, line, COMMAND_LINE_SIZE); // Replace the next iteration's input prompt with a line from the history file
            *reenactingHistory = 1; // Set a flag to mark that a line from history will be executed instead of user input
            fclose(historyfile);
            return;
        }
        lineNumber++;
    }
    if (lineNumberToReenact == -1) { // Last line
        strncpy(inputLine, line, COMMAND_LINE_SIZE); // Replace the next iteration's input prompt with a line from the history file
        *reenactingHistory = 1; // Set a flag to mark that a line from history will be executed instead of user input
    } else { // The parameter was greater than the number of lines in the history file
        
    }
    fclose(historyfile);
}

// There is probably a better way to do this
void clear(FILE *historyfile) {
    // Close and open for writing to erase contents
    fclose(historyfile);
    historyfile = fopen(HISTORY_FILE, "w");
    // Close and open for appending to start recording history again
    fclose(historyfile);
    historyfile = fopen(HISTORY_FILE, "a");
}

// Bug: Prompts with spaces are not processed properly
void change_prompt(char* prompt, char* newPrompt) {
    strcpy(prompt, newPrompt);
    return;
}

void ignore_interrupts() {
  // Ignore Ctrl+C, Ctrl+Z and Ctrl+\
  
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGQUIT);
  sigaddset(&sigs, SIGTSTP);
  sigprocmask(SIG_SETMASK, &sigs, NULL);
}
