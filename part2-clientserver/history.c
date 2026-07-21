#include "history.h"

void history(FILE *historyfile) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
  // Move the cursor to the beginning of the file in case it is at the end
  rewind(historyfile);

  while (fgets(line, sizeof(line), historyfile)) {
      printf("%d  %s", lineNumber, line);
      lineNumber++;
  }

  rewind(historyfile); // Dont close 
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

void saveHistory(const char *inputLine, FILE *historyfile){
  // If the command is not related to history or exiting, save it to history
  if (strlen(inputLine) > 0 
      && strncmp(inputLine, "!", 1) != 0 
      && strncmp(inputLine, "exit", 4) != 0 
      && strncmp(inputLine, "history", 7) != 0) 
  {
      // First checks if fputs failed 
      if(fputs(inputLine, historyfile) == EOF) {
        perror("Failed to save history");
        return;
      }

      if(fputs("\n", historyfile) == EOF) {
        perror("Failed to save history");
        return;
      }

      int flush = fflush(historyfile);
      //printf("%d", flush); // Uncomment for debugging
  }  
}

// lineNumberToReenact == -1 refers to the last line of history 
int getLineOfHistory(FILE* historyfile, int lineNumberToGet, char* lineToReturnTo) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
    
  rewind(historyfile);
  while (fgets(line, sizeof(line), historyfile)) {
    if (lineNumber == lineNumberToGet) {
        strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
        rewind(historyfile);
        return lineNumber;
    }
    lineNumber++;
  }

  rewind(historyfile);

  if (lineNumberToGet > lineNumber) {
    strcpy(lineToReturnTo, ""); // Copy an empty string to the input line, which will be ignored in the next iteration
    printf("event not found\n");
    rewind(historyfile);
    return lineNumber;
  }

  if (lineNumberToGet == -1) {
    // If it reaches here, the cursor will already be pointing at the last line
    strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
    rewind(historyfile);
    return lineNumber;
  }
}

void getLineOfHistoryByString(FILE* historyfile, const char* substringToSearchFor, char* lineToReturnTo) {
  char line[COMMAND_LINE_SIZE];
  int lineNumber = 1;
  int found = 0;

  rewind(historyfile);
  while (fgets(line, sizeof(line), historyfile)) {
    if (strncmp(line, substringToSearchFor, strlen(substringToSearchFor)) == 0) {
        strncpy(lineToReturnTo, line, COMMAND_LINE_SIZE);
        found = 1;
    }
    lineNumber++;
  }
  if (found == 0) {
    strcpy(lineToReturnTo, "");
    printf("event not found\n");
  } 
  rewind(historyfile);
}
