/** 
* File: client.c 
* Description: Connects with the server, enters Username and Password. If valid, client sends messages to the server. Exits when "exit" is sent 
* Steps: Establish socket -> Connect with the server address -> Send Messages (Username, Password, Msg) -> Receive concatenated messages 
* Date: 7/7/2026 
*/ 

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in, htons, htonl
#include <netdb.h> // struct hostent, gethostbyname()
#include <unistd.h> // gethostname 
#include <string.h>
#include <stdio.h>
#include "stream.h" // MAX_BLOCK_SIZE, readn(), writen()
#include "client_io.h" // For Raw mode input handling and arrow key usage 
#include "history.h" // For history handling through arrow keys, !!, !, and history command 

#define SERV_TCP_PORT 4005 // server's "well-known" port number
int authenticate_client(int sd); // Header for authentication function 

int main(int argc, char *argv[]){
  int sd, n, nr, nw, i = 0; // SD = Socket Descriptor | nr and nw = Stores number of bytes written/read
  char buf[MAX_BLOCK_SIZE]; // Where messages are written/sent. 
  char host[60]; // Stores server host name 
  struct sockaddr_in serverAddress; // Stores server IP and Port  
  struct hostent *hp; // Stored data from gethostbyname()
  FILE *historyfile = initializeHistory();
  
  // Get server host name 
  if(argc == 1){ 
    gethostname(host, sizeof(host)); // Assume server running on local host 
  } else if (argc == 2){
    strcpy(host, argv[1]); // Use given host name 
  } else { 
    printf("Please follow format: &s [<server host name>]\n", argv[0]); exit(1);
  }

  // Get host information and build server host socket 
  bzero((char *)&serverAddress, sizeof(serverAddress)); // Clears garbage values 
  serverAddress.sin_family = AF_INET; // IPv4 Networking 
  serverAddress.sin_port = htons(SERV_TCP_PORT); // Converts server port to network byte order 
  if((hp = gethostbyname(host)) == NULL){
    printf("Host not found.\n"); exit(1);
  }
  serverAddress.sin_addr.s_addr = *(u_long *)hp->h_addr; // Assign serverAddress IP to host IP from hp 

  // Build TCP Socket for client 
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if(connect(sd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    perror("client connect"); exit(1);
  }

  // Authentication by Username and password 
  if(authenticate_client(sd) == 0){ 
    printf("Incorrect username or password.\n"); 
    close(sd);
    exit(1);
  } 

  printf("Login Successful!\n");

  // Main loop if connected successfully 
  // TODO: Make the input stream and output streak child processes, make parent program send full commands 
  while(1){ 
    char line[MAX_BLOCK_SIZE]; 
    // TODO: CHANGE TO MAX COMMAND SIZE 
    int lineLength = readLine(line, MAX_BLOCK_SIZE, "% ", historyfile);
    if(lineLength <= 0) continue; 

    saveHistory(line, historyfile); // Save Local history 
    
    // Get user input and add null terminator 
    // if(buf[nr-1] == '\n') buf[nr-1] = '\0'; --nr; 

    // Send message if string is not empty 
    if(lineLength > 0){
      // Sends command to server and reads response 
      nw = writen(sd, line, lineLength); // Send actual command 
      char newline = '\n'; 
      writen(sd, &newline, 1);

      while(nr = readn(sd, buf, sizeof(buf)) > 0){
        buf[nr] = '\0';
        printf("%s". buf);

        if(strcmp(buf,))
      }
  
      // TODO: Change this to a proper disconnect code ( -1);
      buf[nr] = '\0';
      if(strcmp(buf, "Good-bye!") == 0){
        printf("Disconnected from Server\n", buf); 
        close(sd);
        exit(0);
      }

      // Display message from server 
      printf("%s", buf);
    }
  }
}

int authenticate_client(int sd){
  int nr, nw; 
  char buf[MAX_BLOCK_SIZE];
  char username[MAX_BLOCK_SIZE];
  char password[MAX_BLOCK_SIZE]; 

  // Get message and send username details 
  if((nr = readn(sd, buf, sizeof(buf))) <= 0){
    return 0; // Connection error. 
  }
  buf[nr] = '\0';
  printf("%s", buf); 
  fgets(username, sizeof(username), stdin); 
  nr = strlen(username);
  if(username[nr-1] == '\n') username[nr-1] = '\0'; --nr; // Remove new line 
  writen(sd, username, strlen(username)); 

  // Get message and send password details 
  if((nr = readn(sd, buf, sizeof(buf))) <= 0){
    return 0; // Connection error. 
  }
  buf[nr] = '\0';
  printf("%s", buf); 
  fgets(password, sizeof(password), stdin); 
  nr = strlen(password);
  if(password[nr-1] == '\n') password[nr-1] = '\0'; --nr; // Remove new line 
  writen(sd, password, strlen(password)); 

  // Get Status code of the login attempt.   
  if((nr = readn(sd, buf, sizeof(buf))) <= 0){
    return 0; // Connection error. 
  }
  buf[nr] = '\0';
  
  // Authenticate client 
  if(buf[0] == '0'){
    return 0; // Fail code 
  }

  return 1; // Success code 
}
