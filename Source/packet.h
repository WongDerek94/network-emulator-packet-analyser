/*-----------------------------------------------------------------------------------------------------------------------------------
 * HEADER FILE:              packet.h
 *
 * FUNCTIONS:                void makePacket(struct packet* pkt, enum PacketType packetType)
 *                           struct packet copyPacket(struct packet* pkt)
 *                           char* packetTypeToString(int packetType, bool isDropped)
 *                           char* retransmitToString(bool retransmit)
 *
 * DATE:                     December 3rd, 2020
 *
 * REVISIONS:                N/A
 *
 * DESIGNER:                 Maksym Chumak, Derek Wong
 *
 * PROGRAMMER:               Maksym Chumak, Derek Wong
 *
 * NOTES:
 * Header file containing packet struct definition and related helper functions
 * ----------------------------------------------------------------------------------------------------------------------------------*/

#ifndef PACKET_H
#define PACKET_H

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ------------------------------------------------- Enums ----------------------------------------------------------------------------*/
enum PacketType { DATA, ACK, EOT };

/* ------------------------------------------------- Symbolic Constants ---------------------------------------------------------------*/
#define MAX_READ_SIZE   150
#define INVALID_SEQ_NUM 0
#define INVALID_ACK_NUM 0

/* ------------------------------------------------- Enums ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
struct packet
{
    enum PacketType packetType;
    int seqNum;
    char data[PAYLOAD_LEN];
    int windowSize;
    int ackNum;
    bool retransmit;
};
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       makePacket
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void makePacket(struct packet* pkt, enum PacketType packetType)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Creates a packet based on the provided packet type
 * -------------------------------------------------------------------------------------------------------------------------------------*/
void makePacket(struct packet* pkt, enum PacketType packetType)
{
    switch (packetType)
    {
        case ACK:
            pkt->packetType = ACK;
            pkt->ackNum = pkt->seqNum;
            pkt->seqNum = INVALID_SEQ_NUM;
            strcpy(pkt->data, "\0");
            pkt->retransmit = false;
            break;
        case EOT:
            pkt->packetType = EOT;
            pkt->ackNum = INVALID_ACK_NUM;
            strcpy(pkt->data, "\0");
            pkt->seqNum = INVALID_SEQ_NUM;
            pkt->retransmit = false;
            break;
        default:
            perror("Not a valid packet type");
            exit(1);
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       copyPacket
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      struct packet copyPacket(struct packet* pkt)
 *
 * RETURNS:        struct packet
 *
 * NOTES:
 * Creates a shallow copy of a packet
 * -------------------------------------------------------------------------------------------------------------------------------------*/
struct packet copyPacket(struct packet* pkt)
{
    struct packet copyPkt;

    copyPkt.packetType = pkt->packetType;
    copyPkt.seqNum = pkt->seqNum;
    strcpy(copyPkt.data, pkt->data);
    copyPkt.windowSize = pkt->windowSize;
    copyPkt.ackNum = pkt->seqNum;
    copyPkt.retransmit = pkt->retransmit;

    return copyPkt;
}

/*---------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       packetTypeToString
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      packetTypeToString(int packetType, bool isDropped)
 *
 * RETURNS:        char*
 *
 * NOTES:
 * Converts numeric packet type to human readable string
 * -------------------------------------------------------------------------------------------------------------------------------------*/
char* packetTypeToString(int packetType, bool isDropped)
{
    char* type;
    if(isDropped)
    {
        switch (packetType)
        {
        case DATA:
            type = (char *)malloc(15);
            strcpy(type, (char *)"DATA (DROPPED)");
            break;
        case ACK:
            type = (char *)malloc(14);
            strcpy(type, (char *)"ACK (DROPPED)");
            break;
        case EOT:
            type = (char *)malloc(14);
            strcpy(type, (char *)"EOT (DROPPED)");
            break;
        default:
            type = (char *)malloc(8);
            strcpy(type, (char *)"INVALID");
        }
    }
    else
    {
        switch (packetType)
        {
        case DATA:
            type = (char *)malloc(5);
            strcpy(type, (char *)"DATA");
            break;
        case ACK:
            type = (char *)malloc(4);
            strcpy(type, (char *)"ACK");
            break;
        case EOT:
            type = (char *)malloc(4);
            strcpy(type, (char *)"EOT");
            break;
        default:
            type = (char *)malloc(8);
            strcpy(type, (char *)"INVALID");
        }
    }
    return type;
}

/*---------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       retransmitToString
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      char* retransmitToString(bool retransmit)
 *
 * RETURNS:        char*
 *
 * NOTES:
 * Converts bool retransmit value to a human readable string
 * -------------------------------------------------------------------------------------------------------------------------------------*/
char* retransmitToString(bool retransmit)
{
    char* str;
    if (retransmit)
    {
        str = (char *)malloc(5);
        strcpy(str, (char *)"true");
    }
    else
    {
        str = (char *)malloc(6);
        strcpy(str, (char *)"false");
    }

    return str;
}

#endif
