/**
    User, user.c
    Purpose: This file contains all the user functions. With these functions we
    are going to implement the famous Conway's Game of Life as a demonstration 
    of the uC capacity. The system will evolve from an initial state without
    end.

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

#include <stdint.h>          /* For uint32_t definition */
#include <stdbool.h>         /* For true/false definition */

#include "user.h"            /* variables/params used by user.c */
#include "system.h"          /* System funct/params, like osc/peripheral conf*/
#include "pixels.h"          /* initial definition of VRAM */
/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
unsigned long int time=0;  // Sets the timer variable. Increments each execution
unsigned long int period = 0xFFF; // Sets the period of the timer. This period
                                  // is not precise and only valid for informal
                                  // counter. A timer is a more elegant approach

// Two state functions. We define two states in our application, one for scanning
// the current map and do the calculations, and other for printing in the screen.
// We do this as we do not have enough memory to store a buffer. We have to rely
// on the states and one memory trick, as showed forward.
unsigned char scan_state();
unsigned char print_state();

// The array that stores our function pointers and an auxiliary enum definition.
// These two have to be in line.
unsigned char (* state[])(void) = { scan_state, print_state};
enum state_codes {
    SCAN, PRINT
};
        
// The definition of the transitions from one state to another. This is an easy 
// example as they only switch from one to another. But scalable for more 
// complex behavior.
enum state_codes state_transitions[][2] = {
    [SCAN] = {PRINT},
    [PRINT] = {SCAN}
};

// Two variables to work with the current state 
enum state_codes cur_state = SCAN;
unsigned char (* state_fun)(void);

/**
 * This function has all the user variables initialization. It will only execute
 * once per reset.
 */
void InitApp(void)
{
}

/**
 * This function has all the user logic. It's an infinite loop that will execute
 * as soon as the interrupts frees the CPU.
 */
void MainApp(void)
{
    AD1CON1bits.SAMP = 1; // start sampling...
    
    // This is where all the State Machine logic occurs. This is meant to be
    // reliable and to not be touched anymore.
    if (time>period) {
        state_fun = state[cur_state];
        unsigned char ret_code = state_fun();
        cur_state = state_transitions[cur_state][ret_code];
        time=0;
    }
    time++;
    AD1CON1bits.SAMP = 0; // start converting
}

/**
 * Conway's Game of Life is a 0-player game in which a grid of cells (represen-
 * ted by pixels) evolve following a set of rules:
 *  - Each cell is alive or dead.
 *  - If a cell is surrounded by 2 or 3 cells it will survive
 *  - If an empty space is surrounded by exactly 3 cells, a new cell is born
 *  - In any other situation the cell will die
 * 
 * This function will scan the board and calculate which cells will survive or
 * be dead in the next turn. Since we do not have available memory to have a
 * buffer to store the state of our board, we will use the most significant bit
 * of the pixel variable as it is free to use. With this "trick" we can mark 
 * which cells will be alive in the next generation.
 */
unsigned char scan_state() {
    int i, j;
    char count;
    for (i=1; i<PIX_W-1; i++) {
        for (j=1; j<PIX_H-1; j++) {
            count = 0;
            
            /* We scan the surroundings of each cell (pixel). 
             */
            if ((pix[j-1][i-1]&7)>0)   count++;
            if ((pix[j-1][i]&7)>0)     count++;
            if ((pix[j-1][i+1]&7)>0)   count++;
            if ((pix[j][i-1]&7)>0)     count++;            
            if ((pix[j][i+1]&7)>0)     count++;
            if ((pix[j+1][i-1]&7)>0)   count++;            
            if ((pix[j+1][i]&7)>0)     count++;
            if ((pix[j+1][i+1]&7)>0)   count++;
            
            /* If the cell is alive then...
             */
            if ((pix[j][i]&7)>0) {
                switch(count) {
                    case 0:
                    case 1:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        /* ... in any of these cases the cell dies.
                         */
                        pix[j][i]&=0b0111;    
                        break;
                } switch(count) {
                    case 2:
                    case 3:
                        /* Otherwise it lives.
                         */
                        pix[j][i]|=0b1000;    
                        break;
                }
                
            } else {
            /* If the cell is dead then...
             */
                switch(count) {
                    case 3:
                        pix[j][i]|=0b1000; // Then a cell is born
                        break;
                }
            }
        }
    }
    period = ADC1BUF0<<6; // We get the value of the analog input and set the 
                          // period
    return 0; // We return the code of the next state. TBD.
}

/**
 * This function will print the board once we have the cells marked. The color
 * of the cell depends on the longevity of it. If its white it has only been
 * alive for one turn, if it´s cyan then it has been alive for 2 turns. For 3 or
 * more turns the cell will be blue.
 */
unsigned char print_state() {
    int i, j;
    for (i=1; i<PIX_W-1; i++)
    {
        for (j=1; j<PIX_H-1; j++)
        {
            // If marked as alive...
            if ((pix[j][i]&0b1000)>>3) {
                switch(pix[j][i]&0b0111) {
                    case 0b0000:
                        pix[j][i] = 0b0111; // ...we set the cell white
                        break;
                    case 0b0111:
                        pix[j][i] = 0b0110; // If white, then cyan
                        break;
                    case 0b0110:
                    case 0b0100:
                        pix[j][i] = 0b0100; // If cyan, then blue
                        break;
                }
            } else {
            // If marked as dead we paint it black
                pix[j][i]=0;
            }
        }
    }    
    period = ADC1BUF0<<6; // We get the value of the analog input and set the
                          // period
    return 0;   // We return the code of the next state. TBD.
}
