/*
** server.c -- a stream socket server demo
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#define PORT "21236" // the port users will be connecting to
#define BACKLOG 256 // how many pending connections queue will hold
#define MAXDATASIZE 256

typedef struct control_packet{
    //define mode 0; send control data
    char control_datagram[256];
    char file_name[256];
    long int file_size;
    long int block_size_request;
}control_packet;

typedef struct data_packet{
    long int sequence_no;
    long int block_size;
    char payload[256];
}data_packet;

void sigchld_handler(int s)
{
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);
  errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void parse_control(struct control_packet *ctrl, char *buffer){
  printf("Buffer contents: %s\n", buffer);
  int block_size;
  char block_size_buf[256];
  char file_size[256];
  int numblocks;
  int caboose;

  char fileName[256];
  bzero(fileName,256);
  bzero(file_size,256);
  bzero(block_size_buf,256);
  memcpy(fileName, buffer+1, 10);
  memcpy(file_size, buffer+11,10);
  memcpy(block_size_buf, buffer+21,10);

  printf("Filename: %s\nFile Size: %s\nBlock Size Requested: %s\n", fileName,file_size, block_size_buf);
  block_size= atoi(block_size_buf);

  numblocks = atoi(file_size);
  caboose = numblocks % block_size;
  printf("Caboose %d\n",caboose);
  numblocks = ceil((double) numblocks / block_size);

  //remove zero pad
  int i;
  int nameIndex=0;
  for (i=0; i< strlen(fileName); i++){
    if(fileName[i]=='0')nameIndex= nameIndex+1;

  }
  char buffTemp[256];
  memcpy(buffTemp, fileName+nameIndex, strlen(fileName)-nameIndex);
  strcpy(fileName, buffTemp);
  printf("Filename: %s\n", fileName);

  //populate control datagram information
  bzero(ctrl->file_name, strlen(ctrl->file_name));
  bzero(ctrl->control_datagram, strlen(ctrl->control_datagram));
  strcpy(ctrl->control_datagram, buffer);
  strcpy(ctrl->file_name, fileName);
  ctrl->file_size= atoi(file_size);
  ctrl->block_size_request=atoi( block_size_buf);

  //printf("Expecting %d blocks\n",numblocks);
  return;
}

void parse_data   (struct data_packet *data, char *buffer){
  printf("Buffer contents: %s\n", buffer);
  int block_size;
  char block_size_buf[256];
  char sequence_no[256];
  int numblocks;
  int caboose;
  char payload[256];
  int sequence_number;
  int sequence_size;


  bzero(sequence_no,256);
  bzero(block_size_buf,256);
  bzero(payload, strlen(payload));
  memcpy(sequence_no, buffer+1, 10);
  memcpy(block_size_buf, buffer+11,10);
  memcpy(payload, buffer+21,atoi(block_size_buf));

  block_size= atoi(block_size_buf);



  sequence_number = atoi(sequence_no);
  sequence_size = atoi(block_size_buf);

  data->sequence_no = atoi(sequence_no);
  data->block_size = atoi(block_size_buf);
  bzero(data->payload, strlen(data->payload));
  strcpy(data->payload, payload);

  printf("Sequence Number: %ld\nSequence Size: %ld\nPayload: %s\n", data->sequence_no, data->block_size,data->payload);


  return;
}

void assemble_ack(char* ack_packet, long int sequence_number){
  char buf[256];
  char seq[256];
  sprintf(seq, "%ld", sequence_number);
  strcpy(buf, "0");
  if(strlen(seq) <10){
    int addZeros=10-strlen(seq);
    int i=0;
    for (i=0; i<addZeros; i++){
      strcat(buf,"0");
    }
  }

    strcat(buf, seq);
    printf("buf:::::: %s\n",buf);
    strcpy(ack_packet, buf);
  return;
}


int main(int argc, char *argv[])
{
  int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  char name[MAXDATASIZE];
  strcpy(name,argv[1]);
  char client[MAXDATASIZE];
  int getsock_check;
  struct control_packet ctrl;
  char buf[MAXDATASIZE];
  int numbytes;
  FILE *fp;
  struct data_packet arr;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
        perror("server: socket");
        continue;
      }
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("server: bind");
          continue;
        }
        break;
      }
      freeaddrinfo(servinfo); // all done with this structure
      if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
      }
      if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
      }
      sa.sa_handler = sigchld_handler; // reap all dead processes
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_RESTART;
      if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
      }
      printf("server: waiting for connections...\n");

      //need to do this once first
      sin_size = sizeof their_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

      inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s);
        struct data_packet data;
          //receive first greeting from client
          if ((numbytes = recv (new_fd, buf, MAXDATASIZE, 0))== -1){
            //perror ("recv");
            // exit(1);
          }
          else{
          printf("Received Packet\n");
          buf[numbytes] = '\0';
          // printf("Server: received '%s'\n",buf);
          strcpy(client, buf);
          printf("Message: '%s'\n",client);
          }



          if(buf[0]=='0') //if control bit is 0, we've received a control packet
          {
            //parse the control Datagram
            parse_control(&ctrl, client);
            //send ACK
            if (send(new_fd, "0000000000", MAXDATASIZE , 0) == -1)
            perror("send");
            printf("Send ACK to client\n");
          }

          long int numblocks = ceil((double) ctrl.file_size / ctrl.block_size_request);

      struct data_packet store_info[numblocks+1];
      int x;
      for (x =0; x<=numblocks; x++){
        bzero(store_info[x].payload, 256);
      }

      int done_flag=0;
      while(done_flag==0) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s);
          struct data_packet data;
            //receive first greeting from client
            if ((numbytes = recv (new_fd, buf, MAXDATASIZE, 0))== -1){
              //perror ("recv");
              // exit(1);
            }
            else{
            printf("Received Packet\n");
            buf[numbytes] = '\0';
            // printf("Server: received '%s'\n",buf);
            strcpy(client, buf);
            printf("Message: '%s'\n",client);
            }



            if(buf[0]=='0') //if control bit is 0, we've received a control packet
            {
              //parse the control Datagram
              parse_control(&ctrl, client);
              //send ACK
              if (send(new_fd, "0000000000", MAXDATASIZE , 0) == -1)
              perror("send");
              printf("Send ACK to client\n");
            }
            if(buf[0]=='1'){
              printf("Data Packet Received\n");
              //parse and store datagram
              parse_data(&data, client);

              //populate the data storage array
              store_info[data.sequence_no].sequence_no = data.sequence_no;
              store_info[data.sequence_no].block_size = data.block_size;
              bzero(store_info[data.sequence_no].payload, strlen(store_info[data.sequence_no].payload));
              strcpy(store_info[data.sequence_no].payload, data.payload);
              if(store_info[data.sequence_no].sequence_no==20){
                printf("Data Payload %s\n", data.payload);
                printf("StoreInfo Payload %s\n",store_info[data.sequence_no].payload );
              }
              // fp = fopen(ctrl.file_name, "a");
              // fputs(data.payload, fp);
              // fclose(fp);

              char ack_buf[256];
              assemble_ack(ack_buf, data.sequence_no);

              if (send(new_fd, ack_buf, MAXDATASIZE , 0) == -1)
              perror("send");
              printf("Send ACK to client: %s\n", ack_buf);
            }

            //LOOP TO CHECK IF FULLY POPULATED
            int j;
            int populated=0;
            for (j=1; j<=numblocks; j++){
              if(store_info[j].sequence_no==0){
                done_flag=0;
              }
              else{
                populated = populated+1;
              }
            }
            if (populated == numblocks){
              done_flag=1;

              //write
              int i;
              int j;
              for(i=1; i<=numblocks; i++){
                for(j=1; j<=numblocks; j++){
                  //printf("Sequence Number %ld\n",store_info[j].sequence_no);
                  if(store_info[j].sequence_no == i){
                    fp = fopen(ctrl.file_name, "a");
                    char tempBuf[256];
                    strcpy(tempBuf, store_info[j].payload);
                    fputs(tempBuf, fp);
                    fclose(fp);
                    //printf("%s\n", store_info[j].payload);
                  }
                }
              }
              printf("Sequence 20 Payload: %s\n", store_info[20].payload);
            }



            bzero(buf, strlen(buf));
          }



        close (sockfd);
        close(new_fd);
        return 0;
      }
