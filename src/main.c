/**
    Main, main.c
    Purpose: This file contains all the main functions where the program starts.

    @author Marcos Brito
    @version 0.1 15/05/20
*/

/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#endif

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/
int16_t main(void)
{
    Configuration();    // First we call the uC configuration (osc, timer, ...).
    InitApp();          // Then we call the user init function.
    while(1)
    {
        MainApp();      // Then we call the infinite loop for the application
                        // function.
    }
}
