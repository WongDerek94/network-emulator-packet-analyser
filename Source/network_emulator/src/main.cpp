/*----------------------------------------------------------------------------------------------------------------------------
 * SOURCE FILE:    main.cpp
 *
 * FUNCTIONS:      int main(int argc, char *argv[])
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * NOTES:
 * The program is an entry point to Network Emulator application
 * ----------------------------------------------------------------------------------------------------------------------------*/

#include "networkemulator.h"
#include <QApplication>

/*-------------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       main
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      int main(int argc, char *argv[])
 *
 * RETURNS:        int
 *
 * NOTES:
 * Launches Network Emulator application
 * ------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NetworkEmulator w;
    w.show();
    return a.exec();
}
