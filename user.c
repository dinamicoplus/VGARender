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
void InitApp(void)
{
    /*int i,j;
    for(i=0; i<PIX_W; i++)
    {
        for(j=0; j<PIX_H; j++)
        {
            pix[j][i] = j*i%8;
            if (i==PIX_W-1 || i==0 || j==PIX_H-1 || j==0) pix[j][i] = 7;
        }
    }*/
}

void MainApp(void)
{
    /*j++;
    if (j==0xFFFF) { 
        //pix[200][20] ^= 1; 
        j=0;
    }*/
    
}
