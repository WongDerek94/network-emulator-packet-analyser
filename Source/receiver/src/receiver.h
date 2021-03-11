/*-----------------------------------------------------------------------------------------------------------------------------------
 * HEADER FILE:              receiver.h
 *
 * FUNCTION PROTOTYPES:      void flushBuffer(struct packet* buffer, long long *nextSeqNum, long long *newWindowSeqNum, int windowSize)
 *                           sendACK(int sd, struct packet *pkt, int pktSize, struct sockaddr_in *transmitter, socklen_t transmitterLen)
 *                           void saveData(char *data)
 *
 * DATE:                     December 3rd, 2020
 *
 * REVISIONS:                N/A
 *
 * DESIGNER:                 Maksym Chumak
 *
 * PROGRAMMER:               Maksym Chumak
 *
 * NOTES:
 * Header file containing constants and function prototypes for receiver.c
 * -----------------------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

/*------------------------------------------------- Default Strings ---------------------------------------------------------------------*/
#define OUTPUT_FILE_PATH	"./data/message.txt"

/*------------------------------------------------- Funtion Prototypes ------------------------------------------------------------------*/
void sendACK(int sd, struct packet* pkt, int pktSize, struct sockaddr_in* transmitter, socklen_t transmitterLen);
void saveData(char* data);
void flushBuffer(struct packet* buffer, long long* nextSeqNum, long long* newWindowSeqNum, int windowSize);