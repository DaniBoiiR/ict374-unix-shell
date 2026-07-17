#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void ignore_interrupts();

void claim_children(int sig); // Function to collect zombie processes

void signalHandlerSetup();
