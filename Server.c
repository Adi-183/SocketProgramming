// filename portno
#include <stdio.h>// standard input output
#include <stdlib.h>// standard library
#include <string.h>
#include <unistd.h>//provides access to POSIX OS API
#include <sys/types.h>// collection of structs
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSG_LEN 255

void error(const char *msg){
    perror(msg);//error description
    exit(1);//unsuccessful
}
struct message{
    int type;
    char msg[MSG_LEN];
};
void decimalToBinary_d(long n, char temp[][6], int I ) {
    int remainder; 
    int j = 5;
    while(n!=0) {
        remainder = n%2;
        n = n/2;
		if(remainder == 0)		temp[I][j] = '0';
		else					temp[I][j] = '1';
		j--;
    }

    int i;
    for(i=0;i<=j;i++)
	temp[I][i] = '0';

}
int binaryToDecimal_d(char *t ) {
    int decimal = 0, i = 0;
    while (i<8) {
        if(t[7-i] == '1')
        	decimal +=(int) (1 << i);
        ++i;
    }
    return decimal;
}
void decode(char *msg){
	char EncodedMessage[100000];
	int f,x;
	for(x=0;x<strlen(msg);x++){
		if(msg[x] == '\r' || msg[x] == '\n' || msg[x] == '\0'){
			f = x;
			break;
		}
	}

	for(x=0;x<f;x++)
		EncodedMessage[x] = msg[x];
	EncodedMessage[f] = '\0';
	int len = strlen(EncodedMessage);
	int sub = 0;
	int alen = len;
	if(EncodedMessage[len-1] == '='){
		if(EncodedMessage[len-2] == '='){
			alen = len-2;
			sub = 4;
		}
		else{
			alen = len-1;
			sub = 2;
		}
	}
	alen = (alen*6 - sub)/8;

    char code[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	int i,j;
	char temp[100000][6];
	char tempf[600000];

	// bzero(tempf, sizeof(char)*600000);

	for(i=0;i<strlen(EncodedMessage);i++)
		for(j=0;j<=63;j++)
			if(EncodedMessage[i]==code[j])
			{
				decimalToBinary_d(j,temp,i);
				break;
			}	

	int remai = strlen(EncodedMessage) * 6  - strlen(temp[0]);

	if(remai == 6)
	{
		int i;
		for(i=0;i<strlen(temp[0])-2;i++) // removed two 0
			tempf[i] = temp[0][i];
	}
	else if(remai == 12)
	{
		int i;
		for(i=0;i<strlen(temp[0])-4;i++)  // removed four 0
			tempf[i] = temp[0][i];
	}
	else
	{
		int i;
		for(i=0;i<strlen(temp[0]);i++)
			tempf[i] = temp[0][i];
	}


	char DecodedMessage[100000];
	int k = 0;

	int flag = strlen(tempf) / 8;
	int a = flag;
	while(flag)
	{
		char x[8];
		int j;
		for(j=0;j<8;j++)
		    x[j] = tempf[(a-flag)*8 + j];
		
		DecodedMessage[k++] = (char) binaryToDecimal_d(x);
		flag--;
	}

	DecodedMessage[alen] = '\0';
	
	int p = strlen(DecodedMessage);
	int n = strlen(msg);
	for(x=p;x<p+(n-f+1);x++)
		DecodedMessage[x] = msg[f + x-p];
	strcpy(msg, DecodedMessage);
	for(i=0;i< 100000;i++)
		DecodedMessage[i] = '\0';
	for(i=0;i< 600000;i++)
		tempf[i] = '\0';
}
int main(int argc, char *argv[])
{
    //argc: total number of parameters
    //argc=2 for us(file name and port number)
    if(argc<2){
        fprintf(stderr ,"Port no. is not provided, program terminated\n");
        exit(1);
    }
    int sockfd, newsockfd, portno, n;
    pid_t childpid;// Child process id
    struct message buffer;//store and send messages in this buffer
    struct sockaddr_in serv_addr, cli_addr;//gives us internet address included in netinet.h
    socklen_t clilen;//32 bit data type in socket.h
    sockfd=socket(AF_INET, SOCK_STREAM, 0);//socket file descripter
    if(sockfd<0){
        //error 
        error("Error opening Socket");
    }
    //socket created
    bzero( (char *) &serv_addr, sizeof(serv_addr) );//clears serv_addr
    portno=atoi(argv[1]);//string to int conversion of portno
    serv_addr.sin_family=AF_INET;//address family for IPv4
    serv_addr.sin_addr.s_addr=INADDR_ANY;//dont want to bind socket to any specific IP.
    serv_addr.sin_port=htons(portno);//host to network short

    //type casting serv_addr from type sockaddr_in to sockaddr
    //sockaddr and sockaddr_in are two different structures
    if(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr) )<0){
    	//if a already used port is provided in argument this error will be thrown
        error("Binding failed");
    }
    if(listen(sockfd, 10)<0){
        perror("Error on listen: ");
    }
    
    
    while(1){
        clilen=sizeof(cli_addr);
        newsockfd=accept(sockfd, (struct sockaddr *)&cli_addr, &clilen );
        if(newsockfd<0){
            error("Error on accept");
        }
        //connection is established now
        // printf("Connection established successfully!\n");
        // printf("newsockfd: %d \t sockfd: %d \n", newsockfd, sockfd);
        printf("Connection accepted from %s :%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
        childpid=fork();
        if(childpid==0){
            close(sockfd);
            //process the request
            {
                bzero(buffer.msg, sizeof(buffer.msg));//clearing buffer
                while(1){
                    n=read(newsockfd, &buffer, sizeof(buffer));
                    if(n<0){
                       // continue;
                        error("Error on read");
                    }
                    printf("Client sent msg type=%d, msg: %s\n", buffer.type, buffer.msg);//displaying what client said
                    decode(buffer.msg);
                    printf("Decoded message is :%s\n", buffer.msg);
                    if(buffer.type==3){
                        printf("Connection closed from %s :%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
                        //printf("Client sent close\n");
                        close(newsockfd);
                        exit(0);
                    }
                    //printf("Ack: Client entired \n%s\n", buffer);
                    buffer.type = 2;
                    sprintf(buffer.msg, "ACK");
                    n=write(newsockfd, &buffer, sizeof(buffer));
                    if(n<0){
                        error("Error on write");
                    } 
                }
            }

        }
        else if(childpid < 0 ){
            error("Error on fork: ");
        }

    }
    close(newsockfd);
    close(sockfd); 
    printf("Connection closed successfully\n");

    return 0;
}
