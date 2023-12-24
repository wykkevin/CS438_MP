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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <unordered_map>

using namespace std;

#define MAXLINE 1400
#define TIMEOUT_MS 30 // RTT=20ms. Add some extra time based on that.
#define MIN_WINDOW_SIZE 16

typedef struct {
    int type; // 0 - normal msg; 1 - end msg; 2 - end msg confirmed
    int sequence;
    int size;
    char content[MAXLINE];
} TCPMsg;

struct sockaddr_in servaddr;
int sockfd;
int sequence = 0;

// accumulate ack number received from now on
int accAck = -1;
// the number received one ack -> three repeated ack -> resend
unordered_map<int, int> recvAckNum;

// slow start or linear increase
int threshold = 512;
int windowSize = 1;
int sentButNotAck = 0; // the number of message sent but not get the ack

// already transferred
unsigned long long int transferredBytes = 0;
unsigned long long int bytesToTransfer = 0;
char fileData[MAXLINE];
FILE *fp;

unordered_map<int, TCPMsg *> packetMap;

int isTimingOut = 0;

int shouldSendFin = 0;

void diep(char *s) {
    perror(s);
    exit(1);
}

int sendMsg() {
    // resend the pack -> read from the hashmap
    if (packetMap.count(sequence) != 0) {
        TCPMsg *tcpMsg = packetMap[sequence];

        cout << "Sending recorded sequence " << sequence << endl;
        sequence++;

        if (sendto(sockfd, tcpMsg, sizeof(*tcpMsg), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
            perror("send");
        }
        return 1;
    } else if (shouldSendFin == 0) {
        if (transferredBytes >= bytesToTransfer) {
            cout << "transferredBytes: " << transferredBytes << endl;
            shouldSendFin = 1;
            return 0;
        }
        TCPMsg *tcpMsg = new TCPMsg;
        tcpMsg->type = 0;
        memset((char *) tcpMsg, 0, sizeof(*tcpMsg));

        int bytesToRead = MAXLINE;
        if (bytesToTransfer - transferredBytes <= MAXLINE) {
            bytesToRead = bytesToTransfer - transferredBytes;
        }

        int bytesRead;
        if ((bytesRead = fread(tcpMsg->content, sizeof(char), bytesToRead, fp)) <= 0) {
            cout << "Reach the end of the file" << endl;
            shouldSendFin = 1;
            return 0;
        }

        transferredBytes += bytesRead;

        tcpMsg->sequence = sequence;
        tcpMsg->size = bytesRead;

        // Store the message in a hashmap to avoid reread the file
        packetMap[sequence] = tcpMsg;

        cout << "Sending new sequence " << sequence << endl;
        sequence++;

        if (sendto(sockfd, tcpMsg, sizeof(*tcpMsg), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
            perror("send");
        }
        return 1;
    } else { // shouldSendFin == 0
        TCPMsg *tcpMsg = new TCPMsg;
        tcpMsg->type = 1;
        cout << "Sending end message" << endl;
        if (sendto(sockfd, tcpMsg, sizeof(*tcpMsg), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
            perror("send");
        }
    }
    return 0;
}

void resetTimeout(timeval tv) {
    tv.tv_sec = 0; // Timeout in seconds
    tv.tv_usec = TIMEOUT_MS * 1000; // Timeout in microseconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        diep("set timeout");
    }
}

void listenToAck() {
    int numbytes;
    socklen_t len;
    len = sizeof(servaddr);

    struct timeval tv;
    resetTimeout(tv);

    while (1) {
        int receivedData;
        cout << "checkpoint1" << endl;
        numbytes = recvfrom(sockfd, &receivedData, sizeof(receivedData), 0, (struct sockaddr *) &servaddr, &len);
        cout << "checkpoint2" << endl;
        if (numbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Timeout, resend the msg with sequence number accAck+1
            cout << "checkpoint3" << endl;
            //if (isTimingOut == 0) {
            //isTimingOut = 1;
            threshold = windowSize / 2;
            windowSize = MIN_WINDOW_SIZE;
            if (threshold < MIN_WINDOW_SIZE) {
                threshold = MIN_WINDOW_SIZE;
            }
            cout << "Timeout is found current accAck is " << accAck << " threshold changed to " << threshold << endl;
            
            sequence = accAck + 1;
            sendMsg();
            sentButNotAck = 1;
            //}

            cout << "checkpoint4" << endl;
        } else if (numbytes == -1) {
            cout << "checkpoint5" << endl;
            diep("error recv");
            break;
        } else {
            cout << "checkpoint6" << endl;
            isTimingOut = 0;
            int ackSeq = ntohl(receivedData);
            // Receiver received the final
            if (ackSeq == -1) {
                cout << "Final Ack received" << endl;
                break;
            }
            cout << "Ack message : " << ackSeq << endl;
            // receive the expected ack
            if (ackSeq > accAck) {
                cout << "checkpoint7" << endl;
                recvAckNum.clear();
                recvAckNum[ackSeq]++;
                int receivedMsgNum = ackSeq - accAck;
                cout << "sentButNotAck change from " << sentButNotAck << " to " << (sentButNotAck - receivedMsgNum)
                     << endl;
                sentButNotAck -= receivedMsgNum;
                accAck = ackSeq;

                if (windowSize < threshold) {
                    cout << "checkpoint8" << endl;
                    // slow start: send two msg for one received msg
                    for (int i = 0; i < receivedMsgNum; i++) {
                        windowSize++;
                        if (sendMsg() == 1) {
                            sentButNotAck++;
                        }
                        if (sendMsg() == 1) {
                            sentButNotAck++;
                        }
                        cout << "In slow start, window size is " << windowSize << " sentButNotAck is " << sentButNotAck << endl;
                    }
                    cout << "checkpoint9" << endl;
                } else {
                    // cout << "Maybe linear increase, window size is " << windowSize << " sentButNotAck is " << sentButNotAck << endl;
                    // linear increase: send one msg for one received msg. Send one more when reach window size
                    // if (windowSize < sentButNotAck) {
                    cout << "checkpoint10" << endl;
                    for (int i = 0; i < receivedMsgNum; i++) {
                        if (sendMsg() == 1) {
                            sentButNotAck++;
                        }
                    }
                    cout << "checkpoint11" << endl;
                    // }
                    if (sentButNotAck == windowSize) {
                        windowSize++;
                        cout << "In linear increase, window size is " << windowSize << endl;
                        if (sendMsg() == 1) {
                            sentButNotAck++;
                        }
                    }
                }
            } else if (ackSeq == accAck) { // duplicate ack
                cout << "checkpoint13" << endl;
                cout << "duplicate ack received : " << ackSeq << endl;
                recvAckNum[ackSeq]++;
                // three duplicate ack
                if (recvAckNum[ackSeq] == 3) {
                    // Resend the msg with sequence number ackSeq + 1
                    threshold = windowSize / 2;
                    windowSize = windowSize / 2;
                    if (windowSize < MIN_WINDOW_SIZE) {
                        windowSize = MIN_WINDOW_SIZE;
                    }
                    if (threshold < MIN_WINDOW_SIZE) {
                        threshold = MIN_WINDOW_SIZE;
                    }
                    // change sequence number
                    sequence = ackSeq + 1;
                    cout << "In triplicate acks, change threshold to : " << threshold << " windowSize to : "
                         << windowSize << " sequence to : " << sequence << endl;
                    sendMsg();
                }
                cout << "checkpoint14" << endl;
            }
        }
        resetTimeout(tv);
    }
}


// sender more like a client
void reliablyTransfer(char *hostname, unsigned short int hostUDPport, char *filename) {
    //Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(hostUDPport);
    servaddr.sin_addr.s_addr = inet_addr(hostname);
    if (inet_aton(hostname, &servaddr.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // the initial window size must be 1
    for (int m = 0; m < MIN_WINDOW_SIZE; m++) {
        sendMsg();
        sentButNotAck = 1;
    }

    listenToAck();

    fclose(fp);
    close(sockfd);
    return;
}

/*
 *
 */
int main(int argc, char **argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);


    bytesToTransfer = numBytes;
    reliablyTransfer(argv[1], udpPort, argv[3]);


    return (EXIT_SUCCESS);
}
