/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
--	SOURCE FILE:	transmitter.c
--
--	PROGRAM:		transmitter
--
--	FUNCTIONS:		long delay(struct timeval t1, struct timeval t2);
--					void appendToUnACKs(struct node** headRef, int seqNum);
--					void deleteFromUnACKs(struct node** headRef, int seqNum);
--					int getUnACKCount(struct node* head);
--					void freeUnACKs(struct node** headRef);
--					void printUnACKs(struct node* node);
--					void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen);
--					void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT);
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
-- The program will establish a TCP connection to a user specifed network emulator and file.
-- The server can be specified using an IP address.  File has to be specified with full path.
-- With no arguments, the server will default configurations, as with the file.
-- The program will transmit a file's contents in packets windows.  Then wait for ACKs.
-- If all ACKs in a window arrive before the calculated timeout interval value, 
--	send new window with adjusted timeout values and data
-- If not, transmitter will selectively retransmit all DATA packets that haven't been ACKed
-- Once the file contents is successfully received, send EOT packet to terminate connection
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#include "../../common.h"
#include "../../logger.h"
#include "transmitter.h"

 /*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       main
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      int main (int argc, char **argv)
 *
 * RETURNS:        int
 *
 * NOTES:
 * Main entrypoint into transmitter application
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
	const char* const programName = argv[0];
	const char* host = NULL;
	char const* fileName = DATA_FILE_PATH;

	FILE* dataFP = NULL;

	int	port = NETWORK_EMULATOR_PORT;
	int windowSize = INITIAL_WINDOW_SIZE, seqNum = INITIAL_SEQ_NUM, packetSize = sizeof(struct packet);
	int timeoutInterval = DEFAULT_ESTIMATED_RTT + 4 * DEFAULT_DEV_RTT, estimatedRTT = DEFAULT_ESTIMATED_RTT, devRTT = DEFAULT_DEV_RTT, sampleRTT = 0;
	int	socketFileDescriptor =	0;

	struct node* unACKHead = NULL;

	struct hostent* hp;
	struct sockaddr_in receiver, transmitter;
	struct timeval start, end, readTimeout;
	readTimeout.tv_sec = 0;
	readTimeout.tv_usec = DEFAULT_READ_TIMEOUT;

	struct packet arrPackets[MAX_READ_SIZE];
	struct packet* arrPacketsPtr;
	arrPacketsPtr = &arrPackets[0];

	struct packet* ACKPacketPtr = malloc(packetSize);

	socklen_t receiverLen;

	// Get user parameters
	switch (argc)
	{
		case 1: // User doesn't specify any arguments
			// Get receiver IP using config file
			host = NETWORK_EMULATOR_IP;
			if ((hp = gethostbyname(host)) == NULL)
			{
				logToFile(ERROR, NULL, "Unknown server address: %s", host);
				exit(1);
			}
			logToFile(INFO, NULL, "Host found: %s", host);
			break;
		case 2: // User specifies one argument
			// Get receiver IP either using FQDN or IP address
			host = argv[1];
			if ((hp = gethostbyname(host)) == NULL)
			{
				logToFile(ERROR, NULL, "Unknown server address: %s", host);
				exit(1);
			}
			logToFile(INFO, NULL, "Host found: %s", host);
			break;
		case 3: // User specifies two arguments
			// Get receiver IP either using FQDN or IP address
			host = argv[1];
			if ((hp = gethostbyname(host)) == NULL)
			{
				logToFile(ERROR, NULL, "Unknown server address: %s", host);
				exit(1);
			}
			logToFile(INFO, NULL, "Host found: %s", host);

			// Verify file is valid
			dataFP = fopen(argv[2], "r");
			if (dataFP != NULL)
			{
				fclose(dataFP);
				fileName = argv[2];
			}
			else
			{
				logToFile(ERROR, NULL, "File: %s could not be opened", argv[2]);
				exit(1);
			}
			break;
		default:
			logToFile(ERROR, NULL, "Usage: %s [hostName] [fileName]", programName);
			exit(1);
	}

	// Create a datagram socket
	if ((socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		logToFile(ERROR, NULL, "Can't create a socket\n");
		exit(1);
	}

	// Store receiver's information
	bzero((char*)&receiver, sizeof(receiver));
	receiver.sin_family = AF_INET;
	receiver.sin_port = htons(port);
	receiverLen = sizeof(receiver);

	logToFile(INFO, NULL, "The network emulator's port is: %d", port);
	bcopy(hp->h_addr, (char*)&receiver.sin_addr, hp->h_length);

	// Set Socket Options
	if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, &readTimeout, sizeof(readTimeout)) < 0)
	{
		logToFile(ERROR, NULL, "setsockopt failed");
		exit(1);
	}

	// Bind local address to the socket on transmitter
	bzero((char*)&transmitter, sizeof(transmitter));
	transmitter.sin_family = AF_INET;
	transmitter.sin_port = htons(TRANSMITTER_PORT);
	transmitter.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socketFileDescriptor, (struct sockaddr*)&transmitter, sizeof(transmitter)) == -1)
	{
		logToFile(ERROR, NULL, "Can't bind name to socket");
		exit(1);
	}

	logToFile(INFO, NULL, "Sending data in file path: %s", DATA_FILE_PATH);
	if (sizeof(arrPackets) > MAX_BUF_LEN)
	{
		logToFile(ERROR, NULL, "Loaded Data is larger than buffer size");
		exit(1);
	}

	// Store all the file data in an array of structs
	int lineCounter = 0;
	int totalLines = 0;

	dataFP = fopen(fileName, "r");
	while (fgets(arrPackets[lineCounter].data, PAYLOAD_LEN, dataFP))
	{
		lineCounter++;
	}

	totalLines = lineCounter;
	lineCounter = 0;

	logToFile(INFO, NULL, "Number of lines in the file are: %d", totalLines);

	// Send a window of packets and wait for ACKs before creating new window
	enum State state = SendingPackets;
	while (state != AllPacketsSent)
	{
		switch (state)
		{
			case SendingPackets:
				logToFile(INFO, NULL, "Current window size: %d", windowSize);
				// Create a window of packets to send and transmit datagrams to the receiver
				for (int windowCounter = 0; windowCounter < windowSize; ++windowCounter, lineCounter++)
				{
					// Generate a list of unACK packets containing sequence numbers
					appendToUnACKs(&unACKHead, seqNum);

					// Initialize remaining packet fields
					arrPackets[lineCounter].packetType = DATA;
					arrPackets[lineCounter].seqNum = seqNum++;
					arrPackets[lineCounter].windowSize = windowSize;
					arrPackets[lineCounter].ackNum = INVALID_ACK_NUM;
					arrPackets[lineCounter].retransmit = false;

					// Send to receiver
					if (sendto(socketFileDescriptor, arrPacketsPtr++, packetSize, 0, (struct sockaddr*)&receiver, receiverLen) == -1)
					{
						logToFile(ERROR, NULL, "sendto failure");
						exit(1);
					}
					logToFile(INFO, arrPacketsPtr-1, "Sent DATA (seqNum: %d)", arrPackets[lineCounter].seqNum);
					
					// If last data packet is sent, update line counter immediately, stop sending and immediately wait for ACKs
					if (lineCounter + 1 == totalLines)
					{
						lineCounter += 1;
						state = WaitForACKs;
						gettimeofday(&start, NULL);
						break;
					}
				}

				// Start delay measure for timeout events
				gettimeofday(&start, NULL);

				logToFile(INFO, NULL, "Window of packets sent, waiting for ACKs");
				state = WaitForACKs;
				break;
			case WaitForACKs:
				// Check if all ACKs have been received
				if (getUnACKCount(unACKHead) == 0)
				{
					logToFile(INFO, NULL, "All ACKs received\n");
					state = AllACKsReceived;
					break;
				}
				
				// Check for timeout events, with end delay measure 
				gettimeofday(&end, NULL);
				logToFile(DEBUG, NULL, "Current delay = %ld ms.\n", delay(start, end));

				if (delay(start, end) >= timeoutInterval && getUnACKCount(unACKHead)> 0)
				{
					logToFile(INFO, NULL, "RTT (%ld) >= Timeout Interval (=%d), packet loss event detected", delay(start, end), timeoutInterval);
					logToFile(INFO, NULL, "Retransmitting %d unACKs...", getUnACKCount(unACKHead));
					if (DEFAULT_LOGGER_LEVEL == DEBUG) printUnACKs(unACKHead);

					// Retransmit unACKed packets
					retransmitUnACKs(socketFileDescriptor, arrPackets, unACKHead, packetSize, &receiver, receiverLen);

					// Update Timeout Interval based
					updateTimeoutInterval(&timeoutInterval, &sampleRTT, &start, &end, &estimatedRTT, &devRTT);

					// Reset delay measure for timeout events
					gettimeofday(&start, NULL);

					// Reduce window size by half
					windowSize /= 2;
				}

				// Receive data from the receiver (non-blocking)
				if (recvfrom(socketFileDescriptor, ACKPacketPtr, packetSize, 0, (struct sockaddr*)&receiver, &receiverLen) >= 0)
				{
					logToFile(DEBUG, NULL, "Size of unACKs list: %d", getUnACKCount(unACKHead));
					logToFile(INFO, ACKPacketPtr, "Received ACK (ackNum: %d)", ACKPacketPtr->ackNum);

					// Update Timeout Interval based on sampleRTT
					updateTimeoutInterval(&timeoutInterval, &sampleRTT, &start, &end, &estimatedRTT, &devRTT);

					// Check to see if data from receiver contains ACK we haven't received yet
					struct node* current = unACKHead;
					while (current != NULL)
					{
						// Matching ACK found
						if (current->data == ACKPacketPtr->ackNum)
						{
							logToFile(DEBUG, NULL, "ACK found: %d, removing now...", ACKPacketPtr->ackNum);
							deleteFromUnACKs(&unACKHead, ACKPacketPtr->ackNum);
							if (DEFAULT_LOGGER_LEVEL == DEBUG) printUnACKs(unACKHead);

							// Increase window size by one
							if(windowSize!=MAX_WINDOW_SIZE)	windowSize++;
							break;
						}
						// No match found, continue to next node
						current = current->next;
					}
				}
				break;
			case AllACKsReceived:
				logToFile(DEBUG, NULL, "Line Counter %d", lineCounter);
				logToFile(DEBUG, NULL, "Total lines %d", totalLines);
				state = (lineCounter == totalLines) ? AllPacketsSent : SendingPackets;
				freeUnACKs(&unACKHead);
				break;
			default:
				logToFile(ERROR, NULL, "Unknown state: %d", state);
				freeUnACKs(&unACKHead);
				free(ACKPacketPtr);
				exit(1);
		}
	}

	logToFile(INFO, NULL, "Completed Data Transfer");
	logToFile(INFO, NULL, "Sending EOT Packet");
	struct packet* EOTPacket = malloc(packetSize);
	makePacket(EOTPacket, EOT);

	// Ensure EOT delivery
	for (int i = 0; i < 10; ++i) 
	{
		// Send EOT to receiver 
		if (sendto(socketFileDescriptor, EOTPacket, packetSize, 0, (struct sockaddr*)&receiver, receiverLen) == -1)
		{
			logToFile(ERROR, NULL, "sendto failure");
			exit(1);
		}
	}

	logToFile(INFO, NULL, "Terminating Transmitter...");

	free(EOTPacket);
	freeUnACKs(&unACKHead);
	free(ACKPacketPtr);
	close(socketFileDescriptor);
	return(0);
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       updateTimeoutInterval
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Update the timeout interval with each sample data RTT
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT)
{
	*sampleRTT = delay(*start, *end);
	*estimatedRTT = (1 - DEFAULT_RTT_ALPHA) * (*estimatedRTT) + DEFAULT_RTT_ALPHA * (*sampleRTT);
	*devRTT = (1 - DEFAULT_RTT_BETA) * (*devRTT) + DEFAULT_RTT_BETA * abs(*sampleRTT - *estimatedRTT);
	*timeoutInterval = (MAX_TIMEOUT_INTERVAL > (*estimatedRTT + 4 * (*devRTT))) ? *estimatedRTT + 4 * (*devRTT) : MAX_TIMEOUT_INTERVAL;
	logToFile(INFO, NULL, "Updating timeout interval: %d", *timeoutInterval);
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       appendToUnACKs
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void appendToUnACKs(struct node** headRef, int seqNum)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Given a reference (pointer to pointer) to the head of a list and an seqNum, append a new node at the end
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void appendToUnACKs(struct node** headRef, int seqNum)
{
	// Allocate node
	struct node* newNode = (struct node*)malloc(sizeof(struct node));
	struct node* last = *headRef;

	// Put in data
	newNode->data = seqNum;
	// New node is going to be last node
	newNode->next = NULL;
	// If linked list is empty, make new node as head
	if (*headRef == NULL)
	{
		*headRef = newNode;
		return;
	}
	// Else traverse till the last node
	while (last->next != NULL)
	{
		last = last->next;
	}
	// Change next of last node
	last->next = newNode;
	return;
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       deleteFromUnACKs
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void deleteFromUnACKs(struct node** headRef, int seqNum)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Given a reference (pointer to pointer) to the head of a list and an seqNum, delete first occurence of seqNum in list
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void deleteFromUnACKs(struct node** headRef, int seqNum)
{
	// Store head node
	struct node* temp = *headRef, * prev = NULL;

	// If head node holds seqNum to be deleted
	if (temp != NULL && temp->data == seqNum)
	{
		*headRef = temp->next;
		free(temp);
		temp = NULL;
	}

	// Search for seqNum to be deleted, keeping track of previous node
	while (temp != NULL && temp->data != seqNum)
	{
		prev = temp;
		temp = temp->next;
	}

	// If seqNum not present in linked list
	if (temp == NULL) return;

	// Unlink node from linked list
	prev->next = temp->next;
	free(temp);
}

// Get number of nodes in linked list
int getUnACKCount(struct node* head)
{
	int count = 0;
	struct node* current = head;
	while (current != NULL)
	{
		count++;
		current = current->next;
	}
	return count;
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       freeUnACKs
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void freeUnACKs(struct node** headRef)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Given the reference (point to pointer) to the head of a list, delete entire linked list
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void freeUnACKs(struct node** headRef)
{
	// Deref headRef to get real head
	struct node* current = *headRef;
	struct node* next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	// Deref headRef to affect the real head back in caller
	*headRef = NULL;
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       printUnACKs
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void printUnACKs(struct node* node)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Print out the sequence numbers of nodes in linked list
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void printUnACKs(struct node* node)
{
	printf("Sequence nums: ");
	while (node != NULL)
	{
		printf(" %d ", node->data);
		node = node->next;
	}
	printf("\n");
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       retransmitUnACKs
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong
 *
 * PROGRAMMER:     Derek Wong
 *
 * INTERFACE:      void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Resend all currently unACKed packets based on their sequence numbers
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen)
{
	struct node* current = head;
	while (current != NULL)
	{
		struct packet* arrPacketsPtr;
		arrPacketsPtr = &arrPackets[current->data - 1];
		arrPacketsPtr->retransmit = true;
		if (sendto(socketFileDescriptor, arrPacketsPtr, packetSize, 0, (struct sockaddr*)receiver, receiverLen) == -1)
		{
			perror("sendto retransmit failure");
			exit(1);
		}
		current = current->next;
	}
}