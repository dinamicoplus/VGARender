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
#include "pixels.h"
/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

/* TODO Initialize User Ports/Peripherals/Project here */
unsigned long int time=0;
enum {
    SCAN, PRINT
};
char state = SCAN;
    
void InitApp(void)
{
//    int i, j;
//    for (i=0; i<PIX_W; i++)
//    {
//        for (j=0; j<PIX_H; j++)
//        {
//            pix[j][i]=0;
//        }
//    }
//    pix[1][1] = 7;
//    pix[2][2] = 7;
//    pix[2][3] = 7;
//    pix[3][1] = 7;
//    pix[3][2] = 7;
}

void MainApp(void)
{
    int i, j;
    char count;
    time++;
    if (time==0x03FF) {
        if (state==SCAN) {
            for (i=1; i<PIX_W-1; i++)
            {
                for (j=1; j<PIX_H-1; j++)
                {
                    count = 0;
                    if ((pix[j-1][i-1]&7)==7)   count++;
                    if ((pix[j-1][i]&7)==7)     count++;
                    if ((pix[j-1][i+1]&7)==7)   count++;
                    if ((pix[j][i-1]&7)==7)     count++;            
                    if ((pix[j][i+1]&7)==7)     count++;
                    if ((pix[j+1][i-1]&7)==7)   count++;            
                    if ((pix[j+1][i]&7)==7)     count++;
                    if ((pix[j+1][i+1]&7)==7)   count++;
                    if ((pix[j][i]&7)==7)
                    {
                        switch(count) {
                            case 0:
                            case 1:
                            case 4:
                            case 5:
                            case 6:
                            case 7:
                            case 8:
                                pix[j][i]&=0b0111;     
                        } switch(count) {
                            case 2:
                            case 3:
                                pix[j][i]|=0b1000;    
                        }
                    } else {
                        switch(count) {
                            case 3:
                                pix[j][i]|=0b1000;
                        }
                    }
                }
            }
            state = PRINT;
        } else if (state==PRINT) {
            for (i=1; i<PIX_W-1; i++)
            {
                for (j=1; j<PIX_H-1; j++)
                {
                    if ((pix[j][i]&0b1000)>>3) {
                        pix[j][i]=7;
                    } else {
                        pix[j][i]=0;
                    }
                }
            }
            state = SCAN;
        }
        time=0;
    }
}
