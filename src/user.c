/**
    User, user.c
    Purpose: This file contains all the user functions. They draw an endless
    zoom into the Mandelbrot set, on the 150 x 96 screen. The camera flies
    down the antenna on the real axis until a tiny copy of the whole set
    fills the screen.

    Every frame is drawn in two passes: a quick 3 x 3 block preview and then
    the full resolution picture. Between frames the colours are cycled, and
    the potentiometer sets for how long. Once the zoom reaches the limit of
    the fixed point precision it turns around and zooms back out.

    @author Marcos Brito
    @version 0.2
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

/******************************************************************************/
/* Video memory                                                               */
/******************************************************************************/
// One byte per pixel, colour in bits 0-2 (R, G, B). It starts black.
unsigned char pix[PIX_H][PIX_W];

/******************************************************************************/
/* Fixed point                                                                */
/******************************************************************************/
// The iteration runs in Q3.13 (16 bits) so every product is a single
// hardware 16 x 16 -> 32 bit multiplication. The view (centre, step) is kept
// in Q7.24 (32 bits) so the zoom can be smooth even when one pixel is only a
// couple of Q3.13 units wide.
#define FRAC        13
#define WIDE        24
#define TO_FRAC(v)  ((int16_t)(((v) + (1L << (WIDE - FRAC - 1))) >> (WIDE - FRAC)))

#if defined(__XC16__)
    #define MULSS(a, b) __builtin_mulss((a), (b))
#else
    #define MULSS(a, b) ((int32_t)(int16_t)(a) * (int32_t)(int16_t)(b))
#endif

/******************************************************************************/
/* The journey                                                                */
/******************************************************************************/
#define TARGET_X    (-29720838L)    // -1.7715 the destination (Q7.24)
#define TARGET_Y    (        0L)    //  0.0
#define START_X     (-10066330L)    // -0.6  centre of the first frame
#define START_Y     (        0L)    //  0.0
#define STEP0       (   357914L)    //  3.2 / 150 : width of the first frame
#define ZOOM_LEVELS 31              // each level zooms x 8/7; 31 levels ~ x63,
                                    // a pixel is still > 2 Q3.13 units wide
#define BLOCK       3               // preview block size (150 and 96 / 3)

// A pixel is about 1.6 times taller than wide (5 scan lines high, 2 cycles
// wide), so the vertical step is 51/32 of the horizontal one.
#define ASPECT_NUM  51
#define ASPECT_SHIFT 5

// Colours for the escape time bands, as RGB bits: blue, magenta, red,
// yellow, green, cyan, white. Black (0) is the inside of the set.
static const unsigned char palette[7] = {4, 5, 1, 3, 2, 6, 7};
static unsigned char next_colour[8];    // palette rotated by one step

/******************************************************************************/
/* State                                                                      */
/******************************************************************************/
enum stage_codes {
    PREVIEW, DETAIL, CYCLE
};

static enum stage_codes stage = PREVIEW;
static uint8_t row = 0;           // next row (or block row) to draw
static uint8_t level = 0;         // zoom level, 0 .. ZOOM_LEVELS
static int8_t direction = 1;      // +1 zooming in, -1 zooming out
static uint8_t phase = 0;         // colour cycling phase, 0 .. 6
static uint8_t cycles_left = 0;   // colour cycling steps before next frame
static uint8_t max_iter;          // iteration limit for this frame
static int32_t step_x, step_y;    // size of one pixel (Q7.24)
static int32_t left_x, top_y;     // coordinates of pixel (0, 0) (Q7.24)

#if !defined(__XC16__)
uint16_t host_pot = 512;          // potentiometer value for PC simulation
uint32_t host_iterations = 0;     // iteration counter for PC simulation
#endif

/******************************************************************************/
/* Helpers                                                                    */
/******************************************************************************/

/**
 * Reads the potentiometer on AN8 (RB12), 0 .. 1023. The ADC is configured in
 * Configuration() for manual sampling; clearing SAMP starts the conversion.
 */
static uint16_t read_pot(void)
{
#if defined(__XC16__)
    volatile uint8_t d;
    uint16_t timeout = 1000;

    AD1CON1bits.SAMP = 1;
    for (d = 0; d < 20; d++);                   // sampling time
    AD1CON1bits.SAMP = 0;                       // start the conversion
    while (!AD1CON1bits.DONE && --timeout);
    return ADC1BUF0;
#else
    return host_pot;
#endif
}

/**
 * Waits for n complete video frames, using the scan line counter of the
 * interrupt as a clock.
 */
static void wait_frames(uint8_t n)
{
#if defined(__XC16__)
    volatile int *scan_line = &lines;

    while (n--) {
        while (*scan_line != 0);
        while (*scan_line == 0);
    }
#else
    (void)n;
#endif
}

/**
 * Escape time of the point c = cx + i cy (Q3.13). Returns max_iter for
 * points inside the set. A point escapes when z leaves the Q3.13 range
 * (|Re z| or |Im z| >= 4), which never overflows the 32 bit products.
 */
static uint8_t escape_time(int16_t cx, int16_t cy)
{
    int32_t y2 = MULSS(cy, cy);
    int16_t x = cx, y = cy;
    int16_t saved_x = cx, saved_y = cy;
    uint8_t count = 0, period = 4;
    uint8_t i;

    // Main cardioid and period 2 bulb: inside, no need to iterate.
    if (cx > -6554 && cx < 4096 && cy > -5734 && cy < 5734) {   // near them
        int16_t xm = cx - (1 << (FRAC - 2));                    // x - 1/4
        int16_t q = (int16_t)((MULSS(xm, xm) + y2) >> FRAC);
        if (MULSS(q, q + xm) * 4 < y2) {
            return max_iter;
        }
    }
    if (cx > -10650 && cx < -5734) {
        int16_t xp = cx + (1 << FRAC);                          // x + 1
        if (MULSS(xp, xp) + y2 < (1L << (2 * FRAC - 4))) {      // < 1/16
            return max_iter;
        }
    }

    for (i = 1; i < max_iter; i++) {
        int32_t x2 = MULSS(x, x);
        int32_t nx = ((x2 - MULSS(y, y)) >> FRAC) + cx;
        int32_t ny = (MULSS(x, y) >> (FRAC - 1)) + cy;

#if !defined(__XC16__)
        host_iterations++;
#endif
        if (nx > INT16_MAX || nx < INT16_MIN ||
            ny > INT16_MAX || ny < INT16_MIN) {
            return i;
        }
        x = (int16_t)nx;
        y = (int16_t)ny;

        // Periodicity check (Brent): in fixed point, the orbit of a point
        // inside the set falls into an exact cycle, so if z comes back to a
        // saved value it will never escape. The saved value is refreshed at
        // doubling intervals, so cycles of any length are caught.
        if (x == saved_x && y == saved_y) {
            return max_iter;
        }
        if (++count == period) {
            count = 0;
            period <<= 1;
            saved_x = x;
            saved_y = y;
        }
    }
    return max_iter;
}

/**
 * Colour of a point: black inside the set, otherwise a palette band chosen
 * by the escape time and the current colour cycling phase.
 */
static unsigned char colour(int32_t x, int32_t y)
{
    uint8_t n = escape_time(TO_FRAC(x), TO_FRAC(y));

    if (n >= max_iter) {
        return 0;
    }
    return palette[(uint8_t)(n + phase) % 7];
}

/**
 * Prepares the view of the current zoom level. The step and the offset from
 * the target both shrink by 7/8 per level, so the first frame shows the whole
 * set and the centre glides towards the destination as we zoom.
 */
static void setup_frame(void)
{
    int32_t step = STEP0;
    int32_t off_x = START_X - TARGET_X;
    int32_t off_y = START_Y - TARGET_Y;
    uint8_t k;

    for (k = 0; k < level; k++) {
        step -= step >> 3;
        off_x -= off_x >> 3;
        off_y -= off_y >> 3;
    }
    step_x = step;
    step_y = (step * ASPECT_NUM) >> ASPECT_SHIFT;
    left_x = TARGET_X + off_x - (PIX_W / 2) * step_x;
    top_y = TARGET_Y + off_y + (PIX_H / 2) * step_y;
    max_iter = 24 + 2 * level;
}

/**
 * Preview: computes the top left pixel of each 3 x 3 block of one block row
 * and fills the whole block with its colour.
 */
static void draw_preview_row(uint8_t block_row)
{
    uint8_t j0 = block_row * BLOCK;
    int32_t y = top_y - (int32_t)j0 * step_y;
    int32_t x = left_x;
    int32_t block_step = BLOCK * step_x;
    uint8_t i, j, k;

    for (i = 0; i < PIX_W; i += BLOCK) {
        unsigned char c = colour(x, y);
        for (j = j0; j < j0 + BLOCK; j++) {
            for (k = i; k < i + BLOCK; k++) {
                pix[j][k] = c;
            }
        }
        x += block_step;
    }
}

/**
 * Detail: computes every pixel of one row, except the ones the preview has
 * already computed exactly.
 */
static void draw_detail_row(uint8_t j)
{
    int32_t y = top_y - (int32_t)j * step_y;
    int32_t x = left_x;
    bool preview_row = (j % BLOCK) == 0;
    uint8_t i;

    for (i = 0; i < PIX_W; i++) {
        if (!(preview_row && (i % BLOCK) == 0)) {
            pix[j][i] = colour(x, y);
        }
        x += step_x;
    }
}

/**
 * Colour cycling: moves every band one step along the palette.
 */
static void cycle_colours(void)
{
    unsigned char *p = &pix[0][0];
    unsigned char *end = p + (uint16_t)PIX_H * PIX_W;

    while (p < end) {
        *p = next_colour[*p & 7];
        p++;
    }
    phase = (phase == 6) ? 0 : phase + 1;
}

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/**
 * This function has all the user variables initialization. It will only execute
 * once per reset.
 */
void InitApp(void)
{
    uint8_t k;

    next_colour[0] = 0;                         // the inside stays black
    for (k = 0; k < 7; k++) {
        next_colour[palette[k]] = palette[(k + 1) % 7];
    }
    setup_frame();
}

/**
 * This function has all the user logic. It is called in an endless loop and
 * does one small piece of work (a row, or a colour step) per call.
 */
void MainApp(void)
{
    switch (stage) {
    case PREVIEW:
        draw_preview_row(row);
        if (++row == PIX_H / BLOCK) {
            row = 0;
            stage = DETAIL;
        }
        break;

    case DETAIL:
        draw_detail_row(row);
        if (++row == PIX_H) {
            row = 0;
            // The knob sets how long the colours flow before the next zoom.
            cycles_left = 4 + (read_pot() >> 5);
            stage = CYCLE;
        }
        break;

    case CYCLE:
        if (cycles_left > 0) {
            wait_frames(4);
            cycle_colours();
            cycles_left--;
        } else {
            if ((direction > 0 && level == ZOOM_LEVELS) ||
                (direction < 0 && level == 0)) {
                direction = -direction;
            }
            level += direction;
            setup_frame();
            stage = PREVIEW;
        }
        break;
    }
}
