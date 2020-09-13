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
#include <math.h>

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

  n = write(sockfd,"Block Size Receieved\n",256);
  return atoi(buffer);
}

void block_read(int sockfd, int buffer_size){
  int n;
  char buffer[buffer_size];
  bzero(buffer, buffer_size);
  n=read(sockfd,buffer,buffer_size);
  if (n<0) error("ERROR reading from socket");
  printf("%s\n", buffer);
  return;
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

     FILE *fp;


     while(1){
       newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       // if (newsockfd < 0)
       //      error("ERROR on accept");
       // pid = fork();
       // if(pid<0)
       //      error("ERROR on fork");
    //  if(pid ==0){


        int block_size;
        char file_size[256];
        int numblocks;
        int caboose;

        block_size= getBlockSize(newsockfd);

        bzero(file_size,256);
        n = read (newsockfd, file_size, 256);
        printf("File Size Received: %s\n", file_size);
        numblocks = atoi(file_size);
        caboose = numblocks % block_size;
        printf("Caboose %d\n",caboose);
        numblocks = ceil((double) numblocks / block_size);


        printf("Expecting %d blocks\n",numblocks);
        char readBuf[block_size];
        int i=0;
        for(i=0; i<numblocks; i++){
          // if(i== numblocks -1) block_size = caboose;
            fp = fopen("output.txt", "a");

            bzero(readBuf, block_size);
            n=read(newsockfd,readBuf ,block_size);
            readBuf[block_size]=0;
            if (n<0) error("ERROR reading from socket");
            fputs(readBuf, fp);
            printf("Received Message: %s\n", readBuf);
            fclose(fp);
        }
        //while(1) block_read(newsockfd, block_size);




      //  exit(0);
      //}
      //else

        close(newsockfd);
     }
     fclose(fp);
close(sockfd);

     return 0;
}
