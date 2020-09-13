/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}
int getBlockSize(int sockfd){ //added dummy function to allow child processes to continue handling messages
  int n;
  char buffer[500];
  bzero(buffer,500);
  n = read(sockfd,buffer,500);
  if (n < 0) error("ERROR reading from socket");
  printf("Block Size: %s\n",buffer);
  return atoi(buffer);
  //n = write(sockfd,"I got your message",18);
  if (n < 0) error("ERROR writing to socket");
  return 0;
}

int main(int argc, char *argv[])
{
    int n;

     //ignore SIGCHLD signal
     signal(SIGCHLD,SIG_IGN);

     int sockfd, newsockfd, portno, clilen, pid;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     while(1){
       newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       if (newsockfd < 0)
            error("ERROR on accept");
       pid = fork();
       if(pid<0)
            error("ERROR on fork");
      if(pid ==0){

        close(sockfd);
        int block_size;
        block_size= getBlockSize(newsockfd);
        printf("Block Size: %d", block_size);
        exit(0);
      }
      else
        close(newsockfd);
     }


     return 0;
}
