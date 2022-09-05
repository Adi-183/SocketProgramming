//filename server_ipaddress portno
#include <stdio.h>// standard input output
#include <stdlib.h>// standard library
#include <string.h>
#include <unistd.h>//provides access to POSIX OS API read(), write(), close()
#include <sys/types.h>// collection of structs
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg){
    perror(msg);//error description
    exit(1);
}
#define MSG_LEN 255
struct message {
    int type;
    char msg[MSG_LEN];
};
int *char_to_bits(char *msg, int *len, int *blen){
	int n = strlen(msg);
	int sz = n*8;
	if(sz%6)
		sz += (6 -sz%6);
	*len = sz/6;
	if(n%3==1)
		(*len)+=2;
	if(n%3==2)
		(*len)+=1;
	*blen = sz;
	int *bits = (int *)malloc(sz*sizeof(int));
	int idx = 0,i,j;
	for(i = 0;i < n;i++){
		int p  = (int)msg[i];
		for(j=0;j<8;j++){
			bits[idx + 7-j] = (p%2);
			p/=2;
		}
		idx += 8;
	}
	for(i=idx;i<sz;i++)
		bits[i] = 0;
	return bits;
}
char mapchar(int i){
    if(i < 26)
        return 'A' + i;
    else if(i < 52)
        return 'a' + i -26;
    else if(i < 62)
        return '0' + i - 52;
    else
        return (i == 62)? '+':'/';
}
char binary_to_char(int *bits, int s,int e){
	int p = 0,i;
	for(i=e;i>=s;i--)
		p = p + bits[i]*(1 << e-i);
	return mapchar(p);
}

void encode(char*data){
	int f,i,j;
	int n = strlen(data);
	char msg[MSG_LEN];
	strcpy(msg, data);

	for(i=0;i<strlen(data);i++){
		if(data[i] == '\r' || data[i] == '\n' || data[i] == '\0'){
			f = i;
			break;
		}
	}
	msg[f] = '\0';

	int encoded_len, bits_len;
	int *bits = char_to_bits(msg, &encoded_len, &bits_len);
	char *encoded_msg = (char*) malloc((encoded_len+n-f+1)*sizeof(char));
	for(i=0;i<bits_len/6;i++)
		encoded_msg[i] = binary_to_char(bits, i*6, i*6 + 5);
	for(i=bits_len/6;i<encoded_len;i++)
		encoded_msg[i] = '=';
	encoded_msg[encoded_len] = '\0';
	int p = encoded_len;
    for(i=p;i<p+(n-f+1);i++)
		encoded_msg[i] = data[f + i-p];
	strcpy(data, encoded_msg);
    free(encoded_msg);
	free(bits);
}
int main(int argc, char *argv[])
{
    //argc: total number of parameters
    //argc=2 for us(file name and port number)
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct message buffer ;//message
    if(argc<3){
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(1);
    }
    portno=atoi(argv[2]);
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        error("Error opening socket");
    }
    server=gethostbyname(argv[1]);//getting host by name from ip address
    if(server==NULL){
        fprintf(stderr,"Server is offline ");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));//clear serv_addr
    serv_addr.sin_family=AF_INET;//server address family is IPv4
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port=htons(portno);
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0){
        error("Error on connect");
    }
    while(1){
        // take input
        bzero(buffer.msg, sizeof(buffer.msg));///clear the buffer
        fgets(buffer.msg, sizeof(buffer.msg), stdin);//reads line from stdin and stores in buffer
        //set type, 1 or 3
        buffer.type = 1;
        int a=-1;
        if((a = strncmp("Bye", buffer.msg, 3))==0){
            buffer.type = 3;
            printf("Client wants to close the communication now\n");
        } else {
            // printf("a=%d", a);
        }
        encode(buffer.msg);
        n=write(sockfd, &buffer, sizeof(buffer) );
        if(n<0){
            error("Error on write");
        }
       
        if(buffer.type==3) break;//client wants to close the communication

        bzero(buffer.msg, sizeof(buffer.msg));//clear the buffer
        n=read(sockfd, &buffer, sizeof(buffer));//read what server writes
        if(n<0){
            error("Error on read");
        }
        printf("Server sends msg type:%d, \n\t\tmsg: %s\n", buffer.type, buffer.msg);
    }
    close(sockfd);
    return 0;
}
