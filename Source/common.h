/*-----------------------------------------------------------------------------------------------------------------------------------
 * HEADER FILE:              common.h
 *
 * FUNCTIONS:                long delay(struct timeval t1, struct timeval t2)
 *
 * DATE:                     December 3rd, 2020
 *
 * REVISIONS:                N/A
 *
 * DESIGNER:                 Derek Wong
 *
 * PROGRAMMER:               Derek Wong
 *
 * NOTES:
 * Header file containing shared constants and common utility functions
 * ----------------------------------------------------------------------------------------------------------------------------------*/

#ifndef COMMON_H
#define COMMON_H

#include <sys/time.h>

/*------------------------------------------------ Symbolic Constants ---------------------------------------------------------------*/
#define NETWORK_EMULATOR_PORT       50001
#define TRANSMITTER_PORT            50000
#define RECEIVER_PORT               50002
#define PAYLOAD_LEN                 256
#define INITIAL_WINDOW_SIZE         1
#define MAX_WINDOW_SIZE             20
#define INITIAL_SEQ_NUM             1

/*------------------------------------------------- Default Strings ------------------------------------------------------------------*/
#define TRANSMITTER_IP                  "192.168.1.72"
#define NETWORK_EMULATOR_IP             "192.168.1.78"
#define RECEIVER_IP                     "192.168.1.77"

/*---------------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       delay
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      long delay(struct timeval t1, struct timeval t2)
 *
 * RETURNS:        long
 *
 * NOTES:
 * calculates the difference between two time values
 * -------------------------------------------------------------------------------------------------------------------------------------*/
long delay(struct timeval t1, struct timeval t2)
{
    long d;

    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec) / 1000);
    return(d);
}

#endif
