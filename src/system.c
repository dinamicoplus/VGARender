/**
    System, system.c
    Purpose: This file contains all the system functions that handle
    system configuration and initialization.

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

#include "system.h"          /* variables/params used by system.c */

/******************************************************************************/
/* System Level Functions                                                     */
/*                                                                            */
/* Custom oscillator configuration functions, reset source evaluation         */
/* functions, and other non-peripheral microcontroller initialization         */
/* functions get placed in system.c                                           */
/*                                                                            */
/******************************************************************************/


#define BITS2WORD(sfrBitfield) ( *((unsigned int*) &sfrBitfield) )
// Convert a bitfield to a word (unsigned int).
#define BITS2BYTEL(sfrBitfield) ( ((unsigned char*) &sfrBitfield)[0] )
// Return the low byte (as a unsigned char) of a bitfield.
#define BITS2BYTEH(sfrBitfield) ( ((unsigned char*) &sfrBitfield)[1] )
// Return the high byte (as a unsigned char) of a bitfield.
int lines = 0;
// We define the extern lines here, in system, as 0.

/**
    Configure the system:
     - Sets the oscilator to 32 MHz.
     - Sets the I/O.
     - Sets the timer and PWM generation for scanline generation
*/
void Configuration(void)
{
    // Oscillator configuration. We want to make use of the PLL to set the uC to
    // work at 32 MHz (16 MIPS).
    unsigned int pllCounter;
    OSCCONBITS OSCCONbitsCopy;

    OSCCONbitsCopy = OSCCONbits; // We make a copy of the current oscillator
                                 // configuration.

    CLKDIVbits.CPDIV = 3;        // We slow down the uC to change the osc conf.
    CLKDIVbits.PLLEN = 1;        // Enables the PLL.
    // Wait for the PLL to stabalise
    for (pllCounter = 0; pllCounter < 600; pllCounter++);

    OSCCONbitsCopy.NOSC = 1;     // This is the new configuration for the
                                 // oscillator. Fast RC Oscillator with PLL
                                 // module (FRCPLL).
    OSCCONbitsCopy.OSWEN = 1;    // Initiates an oscillator switch to a clock
                                 // source specified by the NOSC<2:0> bits.

    // To limit the possible side effects that could result
    // from the freedom to switch between clock sources during runtime,
    // PIC24F devices have a safeguard lock built into the switching process.
    // It is needed to unlock it to switch to our new configuration.
    __builtin_write_OSCCONH( BITS2BYTEH( OSCCONbitsCopy ) );
    __builtin_write_OSCCONL( BITS2BYTEL( OSCCONbitsCopy ) );

    while (OSCCONbits.COSC != OSCCONbits.NOSC); // We wait until it changes.

    CLKDIVbits.RCDIV0 = 0;
    CLKDIVbits.CPDIV = 0;
    CLKDIVbits.PLLEN = 1;         // Enables the PLL.

    //Configure MCCP
    CCP1CON1Lbits.CCSEL = 0;      // Set MCCP operating mode (OC mode)
    CCP1CON1Lbits.MOD = 0b0100;   // Set mode (Buffered Dual-Compare/PWM mode)

    //Configure MCCP Timebase
    CCP1CON1Lbits.TMR32 = 0;      // Set timebase width (16-bit)
    CCP1CON1Lbits.TMRSYNC = 0;    // Set timebase synchronization (Synchronized)
    CCP1CON1Lbits.CLKSEL = 0b000; // Set the clock source (Tcy)
    CCP1CON1Lbits.TMRPS = 0b00;   // Set the clock pre-scaler (1:1)
    CCP1CON1Hbits.TRIGEN = 0;     // Set Sync/Triggered mode (Synchronous)
    CCP1CON1Hbits.SYNC = 0b00000; // Select Sync/Trigger source (Self-sync)

    //Configure MCCP output for PWM signal that corresponds to H_SYNC.
    CCP1CON2Hbits.OCAEN = 1;      // Enable desired output signals (OC1A)
    CCP1CON3Hbits.OUTM = 0b000;   // Set advanced output modes (Standard output)
    CCP1CON3Hbits.POLACE = 1;     // Configure output polarity (Active High)
    CCP1TMRL = 0x0000;            // Initialize timer prior to enable module.
    CCP1PRL = 0x01FC;             // Configure timebase period. This is a very
                                  // important number. This is the horizontal
                                  // scanline period 0x1FC == 508 cycles that
                                  // correponds to the horizontal period
                                  // (31.75 us ~ 31.7775 us).
    CCP1RA = 0x000A;              // Set the value for the front porch 0xA == 10
                                  // cycles (0.625 us ~ 0.635 us).
    CCP1RB = 0x0047;              // Set the falling edge for the sync pulse
                                  // 0x47 == 71 (10 + 61). Sync pulse will Last
                                  // for 61 cycles (3.8125 us ~ 3.8133 us).
    CCP1CON1Lbits.CCPON = 1;      // Turn on MCCP module

    IEC3bits.CCP1IE = 1;          // Turn the MCCP interruption on
    INTCON2bits.GIE = 1;          // Enables uC interruptions

    TRISA = 0;                    // Sets PORTA as outputs
    TRISC = 0;                    // Sets PORTC as outputs
    TRISAbits.TRISA12 = 1;        // Sets RA12 as input
    TRISBbits.TRISB12 = 1;        // Sets RB12 as input
    ANSBbits.ANSB12 = 1;          // Sets RA12 as an analogic input

    AD1CON1 = 0x0000;             // SAMP bit = 0 ends sampling and starts converting
    AD1CHS = 0x0008;              // Connect AN8 to RA12 input
    AD1CSSL = 0;
    AD1CON3 = 0x0002;             // Manual Sample, Tad = 3Tcy
    AD1CON2 = 0;
    AD1CON1bits.ADON = 1;         // turn ADC ON
}
