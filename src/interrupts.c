/**
    Interrupts, interrupts.c
    Purpose: This file contains all the interruptions handling.

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

#include <stdint.h>        /* Includes uint16_t definition */
#include <stdbool.h>       /* Includes true/false definition */
#include "system.h"        /* System funct/params, like osc/peripheral config */

/******************************************************************************/
/* Interrupt Vector Options                                                   */
/******************************************************************************/
/*                                                                            */
/* Refer to the C30 (MPLAB C Compiler for PIC24F MCUs and dsPIC33F DSCs) User */
/* Guide for an up to date list of the available interrupt options.           */
/* Alternately these names can be pulled from the device linker scripts.      */
/*                                                                            */
/* For alternate interrupt vector naming, simply add 'Alt' between the prim.  */
/* interrupt vector name '_' and the first character of the primary interrupt */
/* vector name.  There are no Alternate or 'Alt' vectors for the 24E family.  */
/*                                                                            */
/* For example, the vector name _ADC2Interrupt becomes _AltADC2Interrupt in   */
/* the alternate vector table.                                                */
/*                                                                            */
/* Example Syntax:                                                            */
/*                                                                            */
/* void __attribute__((interrupt,auto_psv)) <Vector Name>(void)               */
/* {                                                                          */
/*     <Clear Interrupt Flag>                                                 */
/* }                                                                          */
/*                                                                            */
/* For more comprehensive interrupt examples refer to the C30 (MPLAB C        */
/* Compiler for PIC24 MCUs and dsPIC DSCs) User Guide in the                  */
/* <compiler installation directory>/doc directory for the latest compiler    */
/* release.                                                                   */
/*                                                                            */
/******************************************************************************/
/* Interrupt Routines                                                         */
/******************************************************************************/

/**
    This routine will handle the generation of one horizontal scanline.
    The interrupt is trigger once per 508 cycles (31.75 us ~ 31.777 us).
    The behaviour depends on which vertical line are we.
    The first 2 lines are for vertical syncing, that means V_SYNC and H_SYNC are
    set to 0 (63.5 us ~ 65.555 us).
    Then we have the Vertical Back Porch where for 33 lines V_SYNC and H_SYNC
    are active but we have no video lines (1.04775 ms ~ 1.04865 ms).
    The interesting part is thenext 480 lines, where video is displayed.
    Each line is processed to generate 150 pixels horizontally as it checks the
    current scanline to assure that 96 pixels are displayed vertically
    (15.24 ms ~ 15.25 ms).
    Last, we have 10 lines of Vertical Front Porch and the scanline counter will
    reset after 525 lines (16.668 ms ~ 16.683 ms whole frame).
*/
void __attribute__((__interrupt__,no_auto_psv)) _CCP1Interrupt(void)
{
    IFS3bits.CCP1IF = 0;            // Clear the interrupt flag.

    // SYNC PULSE
    if (lines<SYNC_LINES)
    {
        CCP1CON2Hbits.OCAEN = 0;    // H_SYNC will not generate during V_SYNC.
        __asm__("bclr LATB, #9");   // V_SYNC activated.
    }

    // VISIBLE AREA
    else if (lines>SUM(SYNC_LINES,BACKPORCH_LINES,0) &&
             lines<SUM(SYNC_LINES,BACKPORCH_LINES,VIDEO_LINES))
    {
        __asm__("bset LATB, #9");   // V_SYNC disabled.
        CCP1CON2Hbits.OCAEN = 1;    // H_SYNC starts generating.
        int i;
        int k = ((lines-35)/5);     // Each 5 lines group is a vertical pixel.
        for (i=0; i<25; i++) __asm__("nop"); // Wait until start displaying
                                             // That will keep the picture
                                             // centered in the screen.
        if(k<PIX_H) {
            line_render(k);         // Video starts generating.
        }
        __asm__("clr LATA");        // We clear video for the rest of the line.
    }

    // FRONT & BACK PORCH
    else {
        CCP1CON2Hbits.OCAEN = 1;    // H_SYNC starts generating in back and
                                    // front porch.
        __asm__("bset LATB, #9");   // V_SYNC is still disabled.
    }
    lines++;
    if (lines > 525)
    {
        lines = 0;                  // Reset once we reach 525 lines
    }
}
