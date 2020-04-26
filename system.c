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

/* Refer to the device Family Reference Manual Oscillator section for
information about available oscillator configurations.  Typically
this would involve configuring the oscillator tuning register or clock
switching using the compiler's __builtin_write_OSCCON functions.
Refer to the C Compiler for PIC24 MCUs and dsPIC DSCs User Guide in the
compiler installation directory /doc folder for documentation on the
__builtin functions. */
#define BITS2WORD(sfrBitfield) ( *((unsigned int*) &sfrBitfield) )
// Convert a bitfield to a word (unsigned int).
#define BITS2BYTEL(sfrBitfield) ( ((unsigned char*) &sfrBitfield)[0] )
// Return the low byte (as a unsigned char) of a bitfield.
#define BITS2BYTEH(sfrBitfield) ( ((unsigned char*) &sfrBitfield)[1] )
// Return the high byte (as a unsigned char) of a bitfield.


void Configuration(void)
{
    //Oscillator configuration
    unsigned int pllCounter;
    OSCCONBITS OSCCONbitsCopy;
    
    OSCCONbitsCopy = OSCCONbits;
    
    CLKDIVbits.CPDIV = 3;
    CLKDIVbits.PLLEN = 1;
    // Wait for the PLL to stabalise
    for (pllCounter = 0; pllCounter < 600; pllCounter++);
    
    OSCCONbitsCopy.NOSC = 1;
    OSCCONbitsCopy.OSWEN = 1;
    
    __builtin_write_OSCCONH( BITS2BYTEH( OSCCONbitsCopy ) );
    __builtin_write_OSCCONL( BITS2BYTEL( OSCCONbitsCopy ) );

    while (OSCCONbits.COSC != OSCCONbits.NOSC);
    
    CLKDIVbits.RCDIV0 = 0;
    CLKDIVbits.CPDIV = 0;
    CLKDIVbits.PLLEN = 1;
    
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

    //Configure MCCP output for PWM signal
    CCP1CON2Hbits.OCAEN = 1;      // Enable desired output signals (OC1A)
    CCP1CON3Hbits.OUTM = 0b000;   // Set advanced output modes (Standard output)
    CCP1CON3Hbits.POLACE = 1;     //Configure output polarity (Active High)
    CCP1TMRL = 0x0000;            //Initialize timer prior to enable module.
    CCP1PRL = 0x01FD; //0x007F;             //Configure timebase period
    CCP1RA = 0x0002;              // Set the rising edge compare value
    CCP1RB = 0x003F;              // Set the falling edge compare value
    CCP1CON1Lbits.CCPON = 1;      // Turn on MCCP module
    
    IEC3bits.CCP1IE = 1;
    IEC6bits.CCT1IE = 1;
    INTCON2bits.GIE = 1;
    
    TRISA = 0;
    TRISB = 0;
    TRISC = 0;

}

