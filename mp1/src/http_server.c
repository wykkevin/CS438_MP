/*
** server.c -- a stream socket server demo
*/

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

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 100

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void parseFileName(char* input, char* fileName) {
	// Skip the "GET /" part
	int i = 5;
	for (i = 5; i < strlen(input); i++) {
		char c = input[i];
		if (c == ' ') {
			strncpy(fileName, input + 5, i - 5);
			break;
		}
	}
}

char* concat(const char *s1, const char *s2, const char *s3)
{
    char *result = malloc(strlen(s1)+strlen(s2)+strlen(s3)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    strcat(result, s3);
    return result;
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	if (argc != 2) {
	    fprintf(stderr,"usage: server portNumber\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
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

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

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

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
		
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			// Receive GET from client
			char buf[1000];
			int numbytes = recv(new_fd, buf, 1000, 0);
			buf[numbytes] = '\0';

			char fileName[100];
			memset(&fileName, 0, sizeof fileName);
			parseFileName(buf, fileName);
			printf("Client wants %s\n", fileName);

			// Read file
			char getResponseMessage[MAXDATASIZE];
			memset(&getResponseMessage, 0, sizeof getResponseMessage);
			if (access(fileName, F_OK) == 0) {
			    // File exists, return 200.
			    FILE *fp;
			    fp = fopen(fileName ,"rb");
				char correctCode[] = "HTTP/1.1 200 OK\r\n\r\n";
				strncpy(getResponseMessage, correctCode, strlen(correctCode));
				if (send(new_fd, getResponseMessage, strlen(getResponseMessage), 0) == -1) {
					perror("send");
				}
				printf("response message is: %s\n", getResponseMessage);
				memset(getResponseMessage, 0, sizeof(getResponseMessage));
				int bytesRead;
				while ((bytesRead = fread(getResponseMessage, sizeof(char), MAXDATASIZE - 1, fp)) > 0) {
					getResponseMessage[MAXDATASIZE] = '\0';
					if (send(new_fd, getResponseMessage, strlen(getResponseMessage), 0) == -1) {
						perror("send");
					}
					printf("response message is: %s\n", getResponseMessage);
					memset(getResponseMessage, 0, sizeof(getResponseMessage));
				}
				fclose(fp);
			} else {
			    // No such file, return 404.
				char errorCode[] = "HTTP/1.1 404 Not Found\r\n\r\n";
				strncpy(getResponseMessage, errorCode, strlen(errorCode));
				if (send(new_fd, getResponseMessage, strlen(getResponseMessage), 0) == -1) {
					perror("send");
				}
				printf("response message is: %s\n", getResponseMessage);
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

