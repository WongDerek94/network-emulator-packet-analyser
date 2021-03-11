/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
--	HEADER FILE:				transmitter.h
--
--	FUNCTIONS PROTOTYPES:		long delay(struct timeval t1, struct timeval t2);
--								void appendToUnACKs(struct node** headRef, int seqNum);
--								void deleteFromUnACKs(struct node** headRef, int seqNum);
--								int getUnACKCount(struct node* head);
--								void freeUnACKs(struct node** headRef);
--								void printUnACKs(struct node* node);
--								void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen);
--								void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT);
--
--	DATE:			December 3, 2020
--
--	REVISIONS:		N/A

--
--	DESIGNERS:		Derek Wong
--
--	PROGRAMMERS:	Derek Wong
--
--	NOTES:
-- Header file containing constants and function prototypes for transmitter.c
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>


/*-------------------------------------------------------------------------------------Enums--------------------------------------------------------------------------------------------*/
enum State { SendingPackets, WaitForACKs, AllACKsReceived, AllPacketsSent };

/*-------------------------------------------------------------------------------Symbolic Constants-------------------------------------------------------------------------------------*/
#define MAX_BUF_LEN				65000   // Maximum Buffer length
#define MAX_TIMEOUT_INTERVAL	5000	// Maximum Timeout interval in ms
#define DEFAULT_ESTIMATED_RTT	1000	// Default estimated round trip time in ms
#define DEFAULT_DEV_RTT			250		// Default deviation in round trip time in ms
#define DEFAULT_RTT_ALPHA		0.125	// Default constant value used to determine the estimatedRTT
#define DEFAULT_RTT_BETA		0.25	// Default constant value used to determine the deviation in sample RTT
#define DEFAULT_READ_TIMEOUT	300		// Default recvfrom timeout value in us (prevents indefinite blocking)

/*----------------------------------------------------------------------------------Default Strings-------------------------------------------------------------------------------------*/
#define DATA_FILE_PATH		"./resource/message.txt"

/*------------------------------------------------------------------------------------Structs-------------------------------------------------------------------------------------------*/
struct node
{
	int data;
	struct node* next;
};

/*---------------------------------------------------------------------------------Function Prototypes----------------------------------------------------------------------------------*/
long delay(struct timeval t1, struct timeval t2);
void appendToUnACKs(struct node** headRef, int seqNum);
void deleteFromUnACKs(struct node** headRef, int seqNum);
int getUnACKCount(struct node* head);
void freeUnACKs(struct node** headRef);
void printUnACKs(struct node* node);
void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen);
void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT);