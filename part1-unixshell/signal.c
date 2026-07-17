#include "signal.h"

// Ignore Ctrl+C, Ctrl+Z and Ctrl+ 
void ignore_interrupts() {
  
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGQUIT);
  sigaddset(&sigs, SIGTSTP);
  sigprocmask(SIG_BLOCK, &sigs, NULL);
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
  act.sa_flags = SA_NOCLDSTOP | SA_RESTART; // Not catch sopped children 
  sigaction(SIGCHLD, &act, NULL); // When a zombie signal is found, claim children is fired by the signal handler 
}
