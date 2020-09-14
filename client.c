/*
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/time.h>
#define PORT "21236" // the port client will be connecting to
#define MAXDATASIZE 256 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:

typedef struct control_packet{
    //define mode 0; send control data
    char control_datagram[256];
    char file_name[256];
    long int file_size;
    long int block_size_request;
    struct timeval ts;
}control_packet;


typedef struct ack_packet{
    long int sequence_number;
    int received;
}ack_packet;

typedef struct data_packet{
    //sequence number
    long int sequence_no;
    //timestamp structure
    //struct timestamp ts;
    //block_size (data size)
    long int block_size;
    //data
}data_packet;

void assemble_control(struct control_packet *ctrl){
    //control packet size: [1 ctrl bit][15 filename][256 blocksize][256 filesize], total 528

    char tempFileSize[256];
    char tempBlockSize[256];
    char buffer[528];
    sprintf(tempFileSize, "%ld", ctrl->file_size);
    sprintf(tempBlockSize, "%ld", ctrl->block_size_request);

    char control_datagram[528];
    bzero(control_datagram, 528);
    control_datagram[strlen(control_datagram)]=0;
    strcpy(control_datagram,"0");

    if(strlen(ctrl->file_name) <10){
      int addZeros=10-strlen(ctrl->file_name);
      int i=0;
      for (i=0; i<addZeros; i++){
        strcat(control_datagram,"0");
      }
    }
    strcat(control_datagram, ctrl->file_name);

    if(strlen(tempFileSize) <10){
      int addZeros=10-strlen(tempFileSize);
      int i=0;
      for (i=0; i<addZeros; i++){
        strcat(control_datagram,"0");
      }
    }
    strcat(control_datagram, tempFileSize);

  //  strcat(control_datagram, '\0');
  if(strlen(tempBlockSize) <10){
    int addZeros=10-strlen(tempBlockSize);
    int i=0;
    for (i=0; i<addZeros; i++){
      strcat(control_datagram,"0");
    }
  }
    strcat(control_datagram, tempBlockSize);
    // strcat(control_datagram, '\0');
    printf("Assembled: %s\nSize: %lu\n",control_datagram,strlen(control_datagram));
     strcpy(ctrl->control_datagram, control_datagram);
    //printf("%s\n", ctrl->control_datagram);
    return;
}

void assemble_data(long int sequence_no, char *datagram, long int block_size, char *fileBuf){
    //control packet size: [1 ctrl bit][15 filename][256 blocksize][256 filesize], total 528

    char sequence_no_str[256];
    char tempBlockSize[256];

    sprintf(sequence_no_str, "%ld", sequence_no);
    sprintf(tempBlockSize, "%ld", block_size);


    strcpy(datagram,"1");

    //left pad the sequence number
    if(strlen(sequence_no_str) <10){
      int addZeros=10-strlen(sequence_no_str);
      int i=0;
      for (i=0; i<addZeros; i++){
        strcat(datagram,"0");
      }
    }
    strcat(datagram,sequence_no_str);

  //left pad the blocksize data
  if(strlen(tempBlockSize) <10){
    int addZeros=10-strlen(tempBlockSize);
    int i=0;
    for (i=0; i<addZeros; i++){
      strcat(datagram,"0");
    }
  }
  strcat(datagram, tempBlockSize);
    strcat(datagram, fileBuf);
    // strcat(control_datagram, '\0');
    printf("Assembled: %s\nSize: %lu\n",datagram,strlen(datagram));
    //printf("%s\n", ctrl->control_datagram);
    return;
}

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int talk(char* buffer, struct ack_packet ack[]){

  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  strcpy(buf,buffer);

  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
        perror("client: socket");
        continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        perror("client: connect");
        continue;
      }
      break;
    }
    if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    s, sizeof s);
    printf("The client is up and running\n");
    freeaddrinfo(servinfo); // all done with this structure
      buf[strlen(buf)]= '\0';
  //  char buffer[MAXDATASIZE];

      strcpy(buf, buffer);
      printf("Let's send this: %s\n",buf);
      if (send (sockfd,buf,MAXDATASIZE, 0)==-1)
      perror("send");

      printf("Client sent greetings to the server\n");

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
      perror("recv");

    }
    buf[numbytes] = '\0';
    printf("Get reply from '%s'\n",buf);
    printf("Sequence Number %d\n", atoi(buf));
    ack[atoi(buf)].received=1;
    printf("Sequence %ld set to %d.\n",ack[atoi(buf)].sequence_number, ack[atoi(buf)].received);

    close(sockfd);
  return 0;
}

int main(int argc, char *argv[])
{
  char filename[256];
  strcpy(filename, argv[1]);

  char file_size[256];
  struct control_packet ctrl;
  FILE *fp;
  char *fileBuf;
  //open FILE
  fp = fopen("data.bin", "r");





  fseek(fp, 0, SEEK_END);
  ctrl.file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  //populate the control structure
  ctrl.block_size_request =atoi(argv[2]);
  strcpy(ctrl.file_name, filename);
  fileBuf = (char*)calloc(ctrl.file_size, sizeof(char));
  fread(fileBuf, sizeof(char), ctrl.file_size, fp);
  printf("File contains this\n%s\n", fileBuf);
  printf("----------EOF---------------\n");

  printf("Check datagram structure-----\nName: %s\nFile Size: %ld\nBlocks Requested: %ld\n",ctrl.file_name, ctrl.file_size, ctrl.block_size_request);
  assemble_control(&ctrl);
  printf("Control Datagram:\n%s\n", ctrl.control_datagram );
  // struct ack_packet control_ack;
  // control_ack.received = 0;

  int block_size_int = ctrl.block_size_request;
  int index =0;
  long int numblocks;
  int caboose = ctrl.file_size % block_size_int;
  printf("Caboose %d\n",caboose);
  numblocks = ceil((double) ctrl.file_size / block_size_int);
  printf("Num of blocks %ld\n",numblocks);
  struct ack_packet arr_ack[numblocks+1];
  int i=0;
  for(i=0; i<= numblocks;i++){
    arr_ack[i].sequence_number=i;
    arr_ack[i].received=0;
  }


  talk(ctrl.control_datagram, arr_ack);




  //initiate file transfer

  index=0;
  int start=0;
  int end = block_size_int;





  for(index=0; index<numblocks; index++){
    char datagram[256];
    char payload[block_size_int];
    char ts_buf[256];
    memcpy( payload, fileBuf+(index*block_size_int), block_size_int);;
    payload[block_size_int]='\0';

    printf("Block %d: \n%s\nPayload Size: %ld\n",index+1, payload,strlen(payload));


    assemble_data(index+1, datagram, strlen(payload), payload);

    //take time stamp and assemble into a packet
    if(index==0){

      char seq[256];
      char numbytes_buf[256];
      char numbytes_temp[256];
      talk(datagram, arr_ack);
      gettimeofday(&ctrl.ts, NULL);
      printf("Timestamp %d\n",ctrl.ts.tv_usec);
      sprintf(seq, "%d", ctrl.ts.tv_usec);
      //sprintf(numbytes_temp, "%ld",numblocks+1);
      strcpy(ts_buf, "2");
      // if(strlen(numbytes_temp) <10){
      //   int addZeros=10-strlen(numbytes_temp);
      //   int y=0;
      //   for (y=0; y<addZeros; y++){
      //     strcat(numbytes_buf,"0");
      //   }
      // }
      // strcat(numbytes_buf,numbytes_temp);
      // strcat(ts_buf, numbytes_buf);
      if(strlen(seq) <10){
        int addZeros=10-strlen(seq);
        int y=0;
        for (y=0; y<addZeros; y++){
          strcat(ts_buf,"0");
        }
      }
      strcat(ts_buf, seq);



      printf("Timestamp Packet: %s\nSize: %ld\n", ts_buf, strlen(ts_buf));
      arr_ack[numblocks+1].sequence_number = numblocks+1;
      talk(ts_buf, arr_ack);
    }else{
      talk(datagram, arr_ack);
    }


  }

  int allAck = 0;
  int j;
  //printf("Size of ACK array: %lu", sizeof(arr_ack));
  //arr_ack[19].received =0; //use this to fake test no loss acksnowledgement
  while(allAck ==0){
    allAck=1;
    for (j=0; j<= numblocks; j++){
      if(arr_ack[j].received ==0){
        allAck=0;
        printf("Sequence %d not received. Retransmitting...\n",j );
        char datagram[256];
        char payload[block_size_int];
        memcpy( payload, fileBuf+((j-1)*block_size_int), block_size_int);;
        payload[block_size_int]='\0';

        printf("Block %d: \n%s\nPayload Size: %ld\n",j, payload,strlen(payload));

        assemble_data(j, datagram, strlen(payload), payload);
        talk(datagram, arr_ack);
      }
    }

  }

  return 0;
  }
