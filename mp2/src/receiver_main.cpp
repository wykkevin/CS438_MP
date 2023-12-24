#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

using namespace std;

#define MAXLINE 1400
#define TIMEOUT_MS 30

typedef struct {
    int type;
    int sequence;
    int size;
    char content[MAXLINE];
} TCPMsg;

// the highest in-order sequence received
int highestAccSequence = -1;

struct sockaddr_in servaddr, cliaddr;

FILE *fp;

void diep(char *s) {
    perror(s);
    exit(1);
}

// receiver more like a server
void reliablyReceive(unsigned short int myUDPport, char *destinationFile) {
    int sockfd;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(myUDPport);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    fp = fopen(destinationFile, "w");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    socklen_t len;
    len = sizeof(cliaddr);  //len is value/result

    // While still have more data to receive
    int numbytes;
    while (1) {
        TCPMsg *tcpMsg = new TCPMsg;

        numbytes = recvfrom(sockfd, tcpMsg, sizeof(*tcpMsg), 0, (struct sockaddr *) &cliaddr, &len);
        if (numbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            cout << "receive timeout" << endl;
            break;
        } else if (numbytes == 0) {
            break;
        } else if (numbytes == -1) {
            perror("recv");
            exit(1);
        }

        int type = tcpMsg->type;
        int sequence = tcpMsg->sequence;
        cout << "Received type " << type << " sequence " << sequence<<endl;
        if (type == 1) {
            cout << "End message received" << endl;
            int ackSeq = -1;
            sendto(sockfd, &ackSeq, sizeof(ackSeq), 0, (const struct sockaddr *) &cliaddr, len);
            // Set timeout to end the connection
            struct timeval tv;
            tv.tv_sec = 0; // Timeout in seconds
            tv.tv_usec = TIMEOUT_MS * 2 * 1000; // Timeout in microseconds
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
                diep("set timeout");
            }
        } else {
            if (sequence == highestAccSequence + 1) { // in-order msg
                highestAccSequence++;
                // receive the final msg in order
                fwrite(tcpMsg->content, sizeof(char), tcpMsg->size, fp);
                cout << "send ack" << sequence << endl;
                int ackSeq = htonl(sequence);
                sendto(sockfd, &ackSeq, sizeof(ackSeq), 0, (const struct sockaddr *) &cliaddr, len);
            } else if (sequence > highestAccSequence + 1 || sequence == highestAccSequence) {
                cout << "send duplicate ack" << highestAccSequence << " received seq is " << sequence << endl;
                int ackSeq = htonl(highestAccSequence);
                sendto(sockfd, &ackSeq, sizeof(ackSeq), 0, (const struct sockaddr *) &cliaddr, len);
            }
        }

    }
    fclose(fp);

    close(sockfd);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char **argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

