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

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define GET_TEMPLATE "GET /%s HTTP/1.1\r\n\
User-Agent: Wget/1.12(linux-gnu)\r\n\
Host: %s:%s\r\n\
Connection: Keep-Alive\r\n\r\n"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void parseUrl(char* input, char* ipAddress, char* portNumber, char* fileName) {
	// input is in the format of http://IP(:PORT)/FILENAME
	int i = 0;
	int isHttpSkipped = 0;
	int isIpSet = 0;
	int isPortSet = 0;
	int positionAfterHttp = 0;
	int positionAfterIp = 0;
	for (i = 0; i < strlen(input); i++) {
		char c = input[i];
		if (isHttpSkipped == 0 && c == '/') {
			i += 2;
			isHttpSkipped = 1;
			positionAfterHttp = i;
		} else if (isHttpSkipped == 1 && isIpSet == 0 && c == ':') {
			strncpy(ipAddress, input + positionAfterHttp, i - positionAfterHttp);
			isIpSet = 1;
			positionAfterIp = i + 1;
		} else if (isIpSet == 0 && c == '/') {
			strncpy(ipAddress, input + positionAfterHttp, i - positionAfterHttp);
			strncpy(portNumber, "80\0", 3);
			strncpy(fileName, input + i + 1, strlen(input) - (i + 1));
			break;
		} else if (isPortSet == 0 && c == '/') {
			strncpy(portNumber, input + positionAfterIp, i - positionAfterIp);
			strncpy(fileName, input + i + 1, strlen(input) - (i + 1));
			break;
		}
	}
	
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Extract information from input
	char ipAddress[100];
	char portNumber[10];
	char fileName[100];
	memset(&ipAddress, 0, sizeof ipAddress);
	memset(&portNumber, 0, sizeof portNumber);
	memset(&fileName, 0, sizeof fileName);

	parseUrl(argv[1], ipAddress, portNumber, fileName);
	printf("Downloading %s from %s:%s\n", fileName, ipAddress, portNumber);

	if ((rv = getaddrinfo(ipAddress, portNumber, &hints, &servinfo)) != 0) {
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
	printf("client: connecting to %s\n", s);

	// Send file request after connected.
	char httpGetMessage[512];
	memset(&httpGetMessage, 0, sizeof httpGetMessage);
	snprintf(httpGetMessage, sizeof(httpGetMessage), GET_TEMPLATE, fileName, ipAddress, portNumber);

	if (send(sockfd, httpGetMessage, strlen(httpGetMessage), 0) == -1) {
		perror("send");
	}
	// Test using http://www.brainjar.com/java/host/test.html

	freeaddrinfo(servinfo); // all done with this structure

	// Keep listening to responses and write to file.
	FILE *fp;
	fp = fopen("output" ,"w");
	// While still have more data to receive
	int isHeaderSkipped = 0;
	while (1) {
		numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
		printf("numbytes is %d\n", numbytes);
		if (numbytes == 0) {
			break;
		}
		if (numbytes == -1) {
		    perror("recv");
		    exit(1);
		}
		buf[numbytes] = '\0';
		printf("Message received: %s\n", buf);
		if (isHeaderSkipped == 0) {
			char* endOfHeader = strstr(buf, "\r\n\r\n");
			if (endOfHeader) {
				isHeaderSkipped = 1;
				fprintf(fp, endOfHeader + 4);
			}
		} else {
			fprintf(fp, buf);
		}
	}
	
	fclose(fp);
	close(sockfd);

	return 0;
}

