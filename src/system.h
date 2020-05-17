/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */
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

