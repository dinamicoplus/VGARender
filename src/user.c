/**
    User, user.c
    Purpose: This file contains all the user functions. They implement a
    brick breaker game on the 150 x 96 screen, played with the potentiometer
    of the Curiosity board as the paddle controller.

    The game runs at 60 frames per second, synchronised with the video
    frame through the scan line counter of the interrupt. Only what moves is
    redrawn (ball, paddle, broken bricks), so it uses very little CPU.

    @author Marcos Brito
    @version 0.3
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

#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define WHITE   7

/******************************************************************************/
/* Playfield                                                                  */
/******************************************************************************/
#define FP          7               // ball position/speed: 1/128 pixel units
#define TOP_WALL    7               // ceiling line, under the score bar
#define BRICK_TOP   10              // first brick row
#define BRICK_ROWS  6
#define BRICK_COLS  10
#define BRICK_W     15              // 14 pixels + 1 gap (10 x 15 = 150)
#define BRICK_H     4               // 3 pixels + 1 gap
#define PADDLE_Y    90
#define PADDLE_W    20
#define BALL_W      2               // 2 x 1 pixels, about square on screen
#define MAX_SPEED   120             // < 128: at most 1 pixel per frame
#define LIVES       3

// Brick colours, top to bottom. Higher rows score more.
static const unsigned char row_colour[BRICK_ROWS] = {
    RED, MAGENTA, YELLOW, GREEN, CYAN, BLUE
};

/******************************************************************************/
/* Font: 3 x 5 pixels, drawn twice as wide so it looks right on screen        */
/******************************************************************************/
static const char font_chars[] = "0123456789AEGLMORV";
static const uint8_t font[][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7},
    {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1},
    {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},                       // 0 - 9
    {7, 5, 7, 5, 5}, {7, 4, 7, 4, 7}, {7, 4, 5, 5, 7},      // A E G
    {4, 4, 4, 4, 7}, {5, 7, 7, 5, 5}, {7, 5, 5, 5, 7},      // L M O
    {6, 5, 6, 5, 5}, {5, 5, 5, 5, 2},                       // R V
};

/******************************************************************************/
/* State                                                                      */
/******************************************************************************/
enum game_states {
    LEVEL_START, SERVE, PLAY, GAME_OVER
};

static enum game_states state;
static uint16_t timer;              // frames left in the current state
static uint16_t bricks[BRICK_ROWS]; // bit c set: brick in column c
static uint8_t bricks_left;
static int16_t ball_x, ball_y;      // top left of the ball (Q.7)
static int16_t vel_x, vel_y;        // pixels per frame (Q.7)
static int16_t speed;               // vertical speed for this level (Q.7)
static uint8_t paddle_x;            // left edge of the paddle
static uint8_t lives;
static uint8_t level;
static uint16_t score;
static uint16_t rng = 1;

#if !defined(__XC16__)
uint16_t host_pot = 512;            // potentiometer value for PC simulation
#endif

/******************************************************************************/
/* Hardware helpers                                                           */
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
 * Waits until the next video frame starts, using the scan line counter of
 * the interrupt. Drawing right after this happens while the beam is still at
 * the top of the screen, so the moving objects hardly ever tear.
 */
static void wait_frame(void)
{
#if defined(__XC16__)
    volatile int *scan_line = &lines;

    while (*scan_line != 0);
    while (*scan_line == 0);
#endif
}

static uint16_t next_random(void)
{
    rng = rng * 25173u + 13849u;
    return rng;
}

/******************************************************************************/
/* Drawing                                                                    */
/******************************************************************************/

static void fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      unsigned char c)
{
    uint8_t i, j;

    for (j = y; j < y + h; j++) {
        for (i = x; i < x + w; i++) {
            pix[j][i] = c;
        }
    }
}

static void draw_char(uint8_t x, uint8_t y, char ch, unsigned char c)
{
    uint8_t k, row, bit;

    for (k = 0; font_chars[k] != '\0' && font_chars[k] != ch; k++);
    if (font_chars[k] == '\0') {
        return;                                 // space or unknown
    }
    for (row = 0; row < 5; row++) {
        for (bit = 0; bit < 3; bit++) {
            if (font[k][row] & (4 >> bit)) {
                pix[y + row][x + 2 * bit] = c;
                pix[y + row][x + 2 * bit + 1] = c;
            }
        }
    }
}

/**
 * Draws a string, 8 pixels per character. x = 255 centres it.
 */
static void draw_text(uint8_t x, uint8_t y, const char *s, unsigned char c)
{
    uint8_t n = 0;

    while (s[n] != '\0') {
        n++;
    }
    if (x == 255) {
        x = (PIX_W - (8 * n - 2)) / 2;
    }
    while (*s) {
        draw_char(x, y, *s++, c);
        x += 8;
    }
}

static void draw_number(uint8_t x, uint8_t y, uint16_t value, uint8_t digits,
                        unsigned char c)
{
    char text[6];

    text[digits] = '\0';
    while (digits--) {
        text[digits] = '0' + value % 10;
        value /= 10;
    }
    draw_text(x, y, text, c);
}

static void draw_score(void)
{
    fill_rect(PIX_W - 32, 1, 30, 5, BLACK);
    draw_number(PIX_W - 32, 1, score, 4, WHITE);
}

static void draw_lives(void)
{
    uint8_t k;

    fill_rect(0, 2, 20, 3, BLACK);
    for (k = 0; k < lives; k++) {
        fill_rect(2 + 6 * k, 3, 4, 1, CYAN);    // little paddles
    }
}

static void draw_bricks(void)
{
    uint8_t r, c;

    for (r = 0; r < BRICK_ROWS; r++) {
        for (c = 0; c < BRICK_COLS; c++) {
            fill_rect(c * BRICK_W, BRICK_TOP + r * BRICK_H,
                      BRICK_W - 1, BRICK_H - 1,
                      (bricks[r] & (1u << c)) ? row_colour[r] : BLACK);
        }
    }
}

static void draw_paddle(unsigned char c)
{
    fill_rect(paddle_x, PADDLE_Y, PADDLE_W, 2, c);
}

static void draw_ball(unsigned char c)
{
    uint8_t x = ball_x >> FP, y = ball_y >> FP;

    pix[y][x] = c;
    pix[y][x + 1] = c;
}

static void clear_field(void)
{
    fill_rect(0, TOP_WALL + 1, PIX_W, PIX_H - TOP_WALL - 1, BLACK);
}

/******************************************************************************/
/* Game logic                                                                 */
/******************************************************************************/

/**
 * If pixel (x, y) is part of a brick, breaks it, scores it and returns true.
 */
static bool hit_brick(int16_t x, int16_t y)
{
    uint8_t r, c;

    if (y < BRICK_TOP || y >= BRICK_TOP + BRICK_ROWS * BRICK_H ||
        x < 0 || x >= PIX_W) {
        return false;
    }
    r = (y - BRICK_TOP) / BRICK_H;
    c = x / BRICK_W;
    if ((y - BRICK_TOP) % BRICK_H == BRICK_H - 1 || x % BRICK_W == BRICK_W - 1 ||
        !(bricks[r] & (1u << c))) {
        return false;                           // gap or broken brick
    }
    bricks[r] &= ~(1u << c);
    bricks_left--;
    fill_rect(c * BRICK_W, BRICK_TOP + r * BRICK_H, BRICK_W - 1, BRICK_H - 1,
              BLACK);
    score += BRICK_ROWS - r;
    draw_score();
    return true;
}

/**
 * Moves the paddle to the potentiometer position. A 1 pixel dead band keeps
 * the ADC noise from making it shake.
 */
static void move_paddle(void)
{
    uint8_t target = ((uint32_t)read_pot() * (PIX_W - PADDLE_W)) >> 10;

    if (target > paddle_x + 1 || target + 1 < paddle_x) {
        draw_paddle(BLACK);
        paddle_x = target;
        draw_paddle(CYAN);
    }
}

static void start_level(void)
{
    uint8_t r;

    for (r = 0; r < BRICK_ROWS; r++) {
        bricks[r] = (1u << BRICK_COLS) - 1;
    }
    bricks_left = BRICK_ROWS * BRICK_COLS;
    speed = 64 + 8 * level;                     // 0.5 px/frame and faster
    if (speed > MAX_SPEED - 20) {
        speed = MAX_SPEED - 20;
    }
    clear_field();
    draw_text(255, 46, "LEVEL", WHITE);
    draw_number(255, 56, level, 1 + (level > 9), YELLOW);
    timer = 120;
    state = LEVEL_START;
}

static void new_game(void)
{
    fill_rect(0, 0, PIX_W, PIX_H, BLACK);
    fill_rect(0, TOP_WALL, PIX_W, 1, BLUE);     // ceiling
    score = 0;
    lives = LIVES;
    level = 1;
    draw_score();
    draw_lives();
    start_level();
}

static void serve(void)
{
    draw_paddle(CYAN);
    ball_x = (paddle_x + PADDLE_W / 2 - 1) << FP;
    ball_y = (PADDLE_Y - 1) << FP;
    draw_ball(WHITE);
    timer = 60;
    state = SERVE;
}

/**
 * Moves the ball one frame, one axis at a time, bouncing on walls, bricks
 * and the paddle. Speeds are below 1 pixel per frame, so nothing is skipped.
 */
static void move_ball(void)
{
    int16_t nx, ny, px, py;
    bool hit;

    draw_ball(BLACK);

    // Horizontal
    nx = ball_x + vel_x;
    px = nx >> FP;
    py = ball_y >> FP;
    if (px < 0 || px + BALL_W > PIX_W) {
        vel_x = -vel_x;
    } else {
        hit = hit_brick(px, py);
        hit |= hit_brick(px + 1, py);
        if (hit) {
            vel_x = -vel_x;
        } else {
            ball_x = nx;
        }
    }

    // Vertical
    px = ball_x >> FP;
    ny = ball_y + vel_y;
    py = ny >> FP;
    if (py <= TOP_WALL) {
        vel_y = -vel_y;
    } else if (vel_y > 0 && py == PADDLE_Y &&
               px + BALL_W > paddle_x && px < paddle_x + PADDLE_W) {
        // Paddle: the further from the centre, the sharper the angle.
        int16_t offset = (px + 1) - (paddle_x + PADDLE_W / 2);
        vel_x = offset * 12;
        if (vel_x == 0) {
            vel_x = (next_random() & 1) ? 8 : -8;
        }
        if (vel_x > MAX_SPEED) vel_x = MAX_SPEED;
        if (vel_x < -MAX_SPEED) vel_x = -MAX_SPEED;
        if (speed < MAX_SPEED - 20) {
            speed++;                            // every return a bit faster
        }
        vel_y = -speed;
    } else if (py >= PIX_H) {
        // Lost the ball.
        lives--;
        draw_lives();
        if (lives == 0) {
            draw_paddle(BLACK);
            draw_text(255, 46, "GAME OVER", RED);
            timer = 240;
            state = GAME_OVER;
        } else {
            draw_paddle(BLACK);
            serve();
        }
        return;
    } else {
        hit = hit_brick(px, py);
        hit |= hit_brick(px + 1, py);
        if (hit) {
            vel_y = -vel_y;
        } else {
            ball_y = ny;
        }
    }

    draw_ball(WHITE);
    draw_paddle(CYAN);                          // in case the ball touched it

    if (bricks_left == 0) {
        draw_ball(BLACK);
        level++;
        start_level();
    }
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
    rng = read_pot() | 1;
    paddle_x = (PIX_W - PADDLE_W) / 2;
    new_game();
}

/**
 * This function has all the user logic. It is called in an endless loop and
 * runs one frame of the game per call.
 */
void MainApp(void)
{
    wait_frame();

    switch (state) {
    case LEVEL_START:
        if (--timer == 0) {
            clear_field();
            draw_bricks();
            serve();
        }
        break;

    case SERVE:
        // The ball rides on the paddle until it is launched.
        draw_ball(BLACK);
        move_paddle();
        ball_x = (paddle_x + PADDLE_W / 2 - 1) << FP;
        draw_ball(WHITE);
        if (--timer == 0) {
            vel_x = (int16_t)(next_random() % 97) - 48;
            vel_y = -speed;
            state = PLAY;
        }
        break;

    case PLAY:
        move_paddle();
        move_ball();
        break;

    case GAME_OVER:
        if (--timer == 0) {
            new_game();
        }
        break;
    }
}
