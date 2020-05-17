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
unsigned long int period = 0xFFF;

unsigned char scan_state();
unsigned char print_state();

unsigned char (* state[])(void) = { scan_state, print_state};
enum state_codes {
    SCAN, PRINT
};
        
enum state_codes state_transitions[][2] = {
    [SCAN] = {PRINT},
    [PRINT] = {SCAN}
};

enum state_codes cur_state = SCAN;
unsigned char (* state_fun)(void);


void InitApp(void)
{
}

void MainApp(void)
{
    AD1CON1bits.SAMP = 1; // start sampling...
    if (time>period) {
        state_fun = state[cur_state];
        unsigned char ret_code = state_fun();
        cur_state = state_transitions[cur_state][ret_code];
        time=0;
    }
    time++;
    AD1CON1bits.SAMP = 0; // start converting
    //period = PORTBbits.RB12;
}

unsigned char scan_state() {
    int i, j;
    char count;
    for (i=1; i<PIX_W-1; i++) {
        for (j=1; j<PIX_H-1; j++) {
            count = 0;
            if ((pix[j-1][i-1]&7)>0)   count++;
            if ((pix[j-1][i]&7)>0)     count++;
            if ((pix[j-1][i+1]&7)>0)   count++;
            if ((pix[j][i-1]&7)>0)     count++;            
            if ((pix[j][i+1]&7)>0)     count++;
            if ((pix[j+1][i-1]&7)>0)   count++;            
            if ((pix[j+1][i]&7)>0)     count++;
            if ((pix[j+1][i+1]&7)>0)   count++;
            if ((pix[j][i]&7)>0) {
                switch(count) {
                    case 0:
                    case 1:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        pix[j][i]&=0b0111;    
                        break;
                } switch(count) {
                    case 2:
                    case 3:
                        pix[j][i]|=0b1000;    
                        break;
                }
            } else {
                switch(count) {
                    case 3:
                        pix[j][i]|=0b1000;
                        break;
                }
            }
        }
    }
    period = ADC1BUF0<<6;
    return 0;
}

unsigned char print_state() {
    int i, j;
    for (i=1; i<PIX_W-1; i++)
    {
        for (j=1; j<PIX_H-1; j++)
        {
            if ((pix[j][i]&0b1000)>>3) {
                switch(pix[j][i]&0b0111) {
                    case 0b0000:
                        pix[j][i] = 0b0111;
                        break;
                    case 0b0111:
                        pix[j][i] = 0b0110;
                        break;
                    case 0b0110:
                    case 0b0100:
                        pix[j][i] = 0b0100; 
                        break;
                }
            } else {
                pix[j][i]=0;
            }
        }
    }    
    period = ADC1BUF0<<6;
    return 0;
}
