#include "history.h"

void history(FILE *historyfile) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
  // Move the cursor to the beginning of the file in case it is at the end
  fseek(historyfile, 0, SEEK_SET);

  while (fgets(line, sizeof(line), historyfile)) {
      printf("%d  %s", lineNumber, line);
      lineNumber++;
  }

  fseek(historyfile, 0, SEEK_SET); // Dont close 
}

FILE *initializeHistory() {
  FILE *historyfile;
  char historypath[4096];
  getcwd(historypath, sizeof(historypath));
  if (historypath == NULL) {
    perror("getcwd failed");
    exit(0);
  }
  strcat(historypath, "/");
  strcat(historypath, HISTORY_FILE);
  historyfile = fopen(historypath, "a+");
  return historyfile;
}

void saveHistory(char *inputLine, FILE *historyfile){
  // First checks if fputs failed 
  if(fputs(inputLine, historyfile) == EOF ||fputs("\n", historyfile) == EOF){
    perror("Failed to save history");
    return; 
  }
  
  int flush = fflush(historyfile);
  //printf("%d", flush); // Uncomment for debugging
}

// lineNumberToReenact == -1 refers to the last line of history 
void getLineOfHistory(FILE* historyfile, int lineNumberToGet, char* lineToReturnTo) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
    
  fseek(historyfile, 0, SEEK_SET);
  while (fgets(line, sizeof(line), historyfile)) {
    if (lineNumber == lineNumberToGet) {
        strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
        fseek(historyfile, 0, SEEK_SET);
        return;
    }
    lineNumber++;
  }

  fseek(historyfile, 0, SEEK_SET);

  if (lineNumberToGet > lineNumber) {
    strcpy(lineToReturnTo, ""); // Copy an empty string to the input line, which will be ignored in the next iteration
    printf("event not found\n");
    fseek(historyfile, 0, SEEK_SET);
    return;
  }

  if (lineNumberToGet == -1) {
    // If it reaches here, the cursor will already be pointing at the last line
    strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
    fseek(historyfile, 0, SEEK_SET);
  }
}

void getLineOfHistoryByString(FILE* historyfile, const char* substringToSearchFor, char* lineToReturnTo) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
  int found = 0;

  fseek(historyfile, 0, SEEK_SET);
  while (fgets(line, sizeof(line), historyfile)) {
    if (strncmp(line, substringToSearchFor, strlen(substringToSearchFor)) == 0) {
        strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
        printf("%d: \n", lineNumber);
        found = 1;
    }
    lineNumber++;
  }
  if (found == 0) {
    strcpy(lineToReturnTo, "");
    printf("event not found\n");
  } 
  fseek(historyfile, 0, SEEK_SET);
}
