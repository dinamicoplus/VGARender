/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__PIC24E__)
    	#include <p24Exxxx.h>
    #elif defined (__PIC24F__)||defined (__PIC24FK__)
	#include <p24Fxxxx.h>
    #elif defined(__PIC24H__)
	#include <p24Hxxxx.h>
    #endif
#endif

#include <stdint.h>          /* For uint32_t definition */
#include <stdbool.h>         /* For true/false definition */

#include "user.h"            /* variables/params used by user.c */
#include "system.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

/* TODO Initialize User Ports/Peripherals/Project here */
unsigned int j=0;
struct struct_test {
    unsigned char x;
    unsigned char y;
    signed char dirx;
};
struct struct_test player;

void InitApp(void)
{
    player.x = 0;
    player.y = 0;
    player.dirx = 1;
    player.dirx = 0;
}

void MainApp(void)
{
    j++;
    if (j==0x0FFF) { 
        pix[player.y][player.x] = 0;
        if (player.x==PIX_W) {
            player.dirx = -1;
            player.y++;
        }
        if (player.x==0) {
            player.dirx = 1;
            player.y++;
        }
        player.x += player.dirx;
        j=0;
    }
}
