/**
    System, system.h
    Purpose: This file contains all the system functions that handle
    system configuration and initialization.

    @author Marcos Brito
    @version 0.1 15/05/20
*/

#define SYNC_LINES 2
#define BACKPORCH_LINES 33
#define FRONTPORCH_LINES 10
#define VIDEO_LINES 480
#define SUM(a,b,c) a+b+c
#define PIX_W 150
#define PIX_H 96

extern int lines;
extern unsigned char pix[PIX_H][PIX_W];
void line_render(int lines);

void Configuration(void); /* Handles clock switching/osc initialization */
