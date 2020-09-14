#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

typedef struct timestamp{
    int send_time;
    int receive_time;
}timestamp;

typedef struct control_packet{
    //define mode 0; send control data
    char control_datagram[256];
    char file_name[256];
    long int file_size;
    long int block_size_request;
    char source_addr[256];
    char port_no[256];
}control_packet;


typedef struct data_packet{
    //sequence number
    long int sequence_no;
    //timestamp structure
    struct timestamp ts;
    //block_size (data size)
    long int block_size;
    //data
}data_packet;

char* padLeft(char* str, int size){
  int numZeros = size-strlen(str);
  printf("Number of zeros %d\n", numZeros);
  char *buffer="";
  //buffer[strlen(buffer)]=0;
  strcpy(buffer, "0");
  int i;
  for (i = numZeros; i> 1; i--){
    strcat(buffer ,"0");
  }
  strcat(buffer, str);
   //strcat(buffer, '\0');
  printf("%s\n", buffer);
  buffer[256]=0;
  return buffer;
}
void assemble_control(struct control_packet *ctrl){
    //control packet size: [1 ctrl bit][10 filename][10 blocksize][10 filesize][15 Source_Addr][5 Port_No], total 548

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
    printf("Assembled: %s\nSize: %d\n",control_datagram,strlen(control_datagram));
     strcpy(ctrl->control_datagram, control_datagram);
    //printf("%s\n", ctrl->control_datagram);
    return;
}

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{

    int sockfd, portno, n;
    FILE *fp;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char readBuff[256]; //used for reading form file
    char buffer[256];
    char *fileBuf;
    char block_size[256];
    struct control_packet ctrl;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    //open FILE
    fp = fopen("data.bin", "r");


    fseek(fp, 0, SEEK_END);
    ctrl.file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fileBuf = (char*)calloc(ctrl.file_size, sizeof(char));
    fread(fileBuf, sizeof(char), ctrl.file_size, fp);
    printf("File contains this\n%s\n", fileBuf);



    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");


    printf("Please enter output file name: ");
    bzero(ctrl.file_name,256);
    fgets(ctrl.file_name,255,stdin);
    char *p;
    if ((p = strchr(ctrl.file_name, '\n')) != NULL)
      *p = '\0';




    char file_size_str[256];
    sprintf(file_size_str, "%d\n", ctrl.file_size);
    printf("Filename: text.txt   |   Size: %d\n---------------------------\n ", ctrl.file_size);
    printf("Please enter desired blocksize: ");
    bzero(block_size,256);
    fgets(block_size,255,stdin);
    printf("\nDesired Blocksize: %d\n",atoi(block_size));
    ctrl.block_size_request=atoi(block_size);
    int block_size_int = atoi(block_size);
    char temp[256];

    printf("Check datagram structure-----\nName: %s\nFile Size: %ld\nBlocks Requested: %ld\n",ctrl.file_name, ctrl.file_size, ctrl.block_size_request);

     //strcpy(ctrl.control_datagram, assemble_control);
     bzero(ctrl.control_datagram, strlen(ctrl.control_datagram));
     assemble_control(&ctrl);
     printf("Control Datagram:\n%s\n", ctrl.control_datagram );

    //send blocksize ---> send control data
    n = write(sockfd,ctrl.control_datagram ,31);
    if (n<0) error("ERROR writing to socket");

    printf("Beigin writing file...\n");

    //systime
    struct timeval tv;


    int index =0;
    int numblocks;
    int caboose = ctrl.file_size % block_size_int;
    printf("Caboose %d\n",caboose);
    numblocks = ceil((double) ctrl.file_size / block_size_int);
    printf("Num of blocks %d\n",numblocks);
    index=0;
    int start=0;
    int end = block_size_int;
    for(index=0; index<numblocks; index++){
      // if(index == numblocks-1)
      //   block_size_int=caboose;

      char sendBuff[block_size_int];
      //printf("Start %d, End %d, \n", start, end);
    //  bzero(sendBuff,strlen(sendBuff));
      gettimeofday(&tv, NULL);
      printf("%d\n", tv.tv_usec);
      memcpy( sendBuff, fileBuf+(index*block_size_int), block_size_int);;
      sendBuff[block_size_int]=0;

      printf("Block %d: \n%s\n",index, sendBuff);
      n = write(sockfd, sendBuff, block_size_int);
      if (n<0) error("ERROR writing to socket");
      //file_size = file_size-block_size_int;
      //start=start+ block_size_int;




    }


    // bzero(buffer,256);
    // n = read(sockfd,buffer,255);
    // if (n < 0)
    //      error("ERROR reading from socket");
    // printf("%s\n",buffer);


    fclose(fp);
    free(fileBuf);
    return 0;
}
