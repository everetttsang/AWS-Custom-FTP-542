#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    //open FILE
    fp = fopen("test.txt", "r");


    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fileBuf = (char*)calloc(file_size, sizeof(char));
    fread(fileBuf, sizeof(char), file_size, fp);
    printf("File contains this\n\n%s\n", fileBuf);



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
    // printf("Writing file\n");
    // bzero(buffer,256);
    //fgets(buffer,255,stdin);

    // fgets(readBuff, 256, (FILE*)fp);
    // printf("Message: %s\n", readBuff);
    // printf("Size: %d\n", file_size);



    printf("Filename: text.txt   |   Size: %d\n---------------------------\n ", file_size);
    printf("Please enter desired blocksize: ");
    bzero(block_size,256);
    fgets(block_size,255,stdin);
    printf("\nDesired Blocksize: %d\n",atoi(block_size));
    char temp[256];

    n = write(sockfd,block_size ,256);

    n = write(sockfd,fileBuf,strlen(fileBuf));
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
         error("ERROR reading from socket");
   printf("%s\n",buffer);

fclose(fp);
    free(fileBuf);
    return 0;
}
