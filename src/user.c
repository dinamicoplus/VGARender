/**
    User, user.c
    Purpose: This file contains all the user functions. They implement a
    CHIP-8 interpreter with a ROM menu, driven by the potentiometer and the
    two buttons of the Curiosity board.

    - Menu: the knob picks a ROM, S2 selects it.
    - Controls: the five physical inputs (knob turned left, knob turned
      right, S1, S2, and S1 + S2 pressed together) can be mapped to any
      CHIP-8 key. S1 moves to the next input, the knob picks the key, S2
      starts the game. Every ROM comes with its own default mapping.
    - Game: hold S1 and S2 together for 1.5 s to go back to the menu.

    The CHIP-8 screen (64 x 32) is drawn 2 pixels wide and 1 or 2 rows tall
    (alternating), which makes its pixels nearly square on the monitor.
    The sound timer lights the blue LED of the RGB LED (RC7).

    @author Marcos Brito
    @version 0.4
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
/* ROM table                                                                  */
/******************************************************************************/
// Quirks: the original COSMAC VIP interpreter and later ones (CHIP-48,
// SUPER-CHIP) differ in a few opcodes. Each ROM says which behaviour it
// expects.
#define Q_VF_RESET  0x01    // 8xy1/8xy2/8xy3 clear vF
#define Q_MEMORY    0x02    // Fx55/Fx65 leave I incremented
#define Q_DISPWAIT  0x04    // Dxyn waits for the next frame (max 60 per s)
#define Q_CLIP      0x08    // sprites are clipped at the edges, not wrapped
#define Q_SHIFT     0x10    // 8xy6/8xyE shift vX in place (ignore vY)
#define Q_JUMP      0x20    // Bxnn jumps to xnn + vX instead of nnn + v0
#define Q_MEMORY_X  0x40    // Fx55/Fx65 leave I + x (CHIP-48), not I + x + 1
#define QUIRKS_VIP     (Q_VF_RESET | Q_MEMORY | Q_DISPWAIT | Q_CLIP)
#define QUIRKS_CHIP48  (Q_CLIP | Q_SHIFT | Q_JUMP | Q_MEMORY_X)
#define QUIRKS_SCHIP   (Q_CLIP | Q_SHIFT | Q_JUMP)

#define KEY_NONE    0xFF
#define INPUTS      5       // knob left, knob right, S1, S2, S1 + S2

struct rom_info {
    const char *name;
    const uint8_t *data;
    uint16_t size;
    uint8_t keys[INPUTS];   // CHIP-8 key for each physical input
    uint8_t quirks;
    uint8_t speed;          // instructions per frame (60 frames per second)
};

#include "roms.h"           // const struct rom_info roms[]; ROM_COUNT

/******************************************************************************/
/* Screen layout                                                              */
/******************************************************************************/
#define C8_W        64
#define C8_H        32
#define SCR_X       11      // first pix column of the CHIP-8 screen
#define SCR_Y       8       // first pix row; 32 rows become 48
#define SCR_COLOUR  WHITE

/******************************************************************************/
/* Font: 3 x 5 pixels, drawn twice as wide so it looks right on screen        */
/******************************************************************************/
static const char font_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-:>+./";
static const uint8_t font[][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7},  // 0-3
    {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1},  // 4-7
    {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},                                    // 8-9
    {7, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {7, 4, 4, 4, 7}, {6, 5, 5, 5, 6},  // A-D
    {7, 4, 7, 4, 7}, {7, 4, 7, 4, 4}, {7, 4, 5, 5, 7}, {5, 5, 7, 5, 5},  // E-H
    {7, 2, 2, 2, 7}, {1, 1, 1, 5, 7}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7},  // I-L
    {5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {7, 5, 5, 5, 7}, {7, 5, 7, 4, 4},  // M-P
    {7, 5, 5, 7, 1}, {6, 5, 6, 5, 5}, {7, 4, 7, 1, 7}, {7, 2, 2, 2, 2},  // Q-T
    {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5},  // U-X
    {5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},                                    // Y-Z
    {0, 0, 7, 0, 0}, {0, 2, 0, 2, 0}, {4, 2, 1, 2, 4}, {0, 2, 7, 2, 0},  // - : > +
    {0, 0, 0, 0, 2}, {1, 1, 2, 4, 4},                                    // . /
};

// CHIP-8 hexadecimal digit sprites, loaded at 0x050.
static const uint8_t c8_font[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,  0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,  0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,  0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  0xF0, 0x80, 0xF0, 0x80, 0x80,
};
#define C8_FONT_ADDR 0x050

/******************************************************************************/
/* CHIP-8 machine                                                             */
/******************************************************************************/
static uint8_t mem[4096];
static uint8_t V[16];
static uint16_t I;
static uint16_t pc;
static uint16_t stack[16];
static uint8_t sp;
static uint8_t delay_timer, sound_timer;
static uint16_t keys;               // bit k set: CHIP-8 key k is down
static uint8_t disp[C8_H][C8_W / 8];
static uint8_t quirks;
static uint8_t speed;
static uint8_t key_wait;            // Fx0A: 0 idle, 1 waiting for release
static uint8_t key_waited;
static uint16_t rng = 1;
static uint16_t bad_opcode;         // opcode that stopped the machine

enum step_results {
    STEP_OK, STEP_DREW, STEP_BLOCKED, STEP_HALTED
};

/******************************************************************************/
/* Application state                                                          */
/******************************************************************************/
enum app_states {
    MENU, CONTROLS, RUN, HALTED
};

static enum app_states state;
static bool redraw = true;          // menu must be drawn again
static uint8_t selected;            // ROM index
static uint8_t keymap[INPUTS];      // current mapping for the selected ROM
static uint8_t cursor;              // CONTROLS: input being edited
static uint8_t pickup;              // CONTROLS: knob position on arrival
static uint8_t exit_hold;           // frames S1 + S2 have been held
static uint8_t idle_buttons;        // button levels at reset (not pressed)
static uint8_t buttons, last_buttons;
static bool knob_left, knob_right;
static uint16_t pot;

#define BTN_S1  0x01
#define BTN_S2  0x02

#if !defined(__XC16__)
uint16_t host_pot = 512;            // inputs for PC simulation
uint8_t host_buttons = 0;
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
 * Raw levels of S1 (RA11) and S2 (RA12) as BTN_S1 | BTN_S2 bits.
 */
static uint8_t read_button_levels(void)
{
#if defined(__XC16__)
    return (PORTAbits.RA11 ? BTN_S1 : 0) | (PORTAbits.RA12 ? BTN_S2 : 0);
#else
    return host_buttons;
#endif
}

static void set_beep(bool on)
{
#if defined(__XC16__)
    LATCbits.LATC7 = on;                        // blue of the RGB LED
#else
    (void)on;
#endif
}

/**
 * Waits until the next video frame starts, using the scan line counter of
 * the interrupt.
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
 * Draws a string, `step` pixels per character (characters are 6 wide).
 * x = 255 centres it. Characters beyond the right edge are dropped.
 */
static void draw_text_step(uint8_t x, uint8_t y, const char *s, unsigned char c,
                           uint8_t step)
{
    uint8_t n = 0;

    while (s[n] != '\0') {
        n++;
    }
    if (x == 255) {
        x = (n * step < PIX_W) ? (PIX_W - (step * n - (step - 6))) / 2 : 0;
    }
    while (*s && x + 6 <= PIX_W) {
        draw_char(x, y, *s++, c);
        x += step;
    }
}

/**
 * Draws a string, 8 pixels per character (18 fit in a row).
 */
static void draw_text(uint8_t x, uint8_t y, const char *s, unsigned char c)
{
    draw_text_step(x, y, s, c, 8);
}

static char hex_digit(uint8_t v)
{
    return (v < 10) ? '0' + v : 'A' + v - 10;
}

static void clear_all(void)
{
    fill_rect(0, 0, PIX_W, PIX_H, BLACK);
}

/******************************************************************************/
/* CHIP-8 interpreter                                                         */
/******************************************************************************/

/**
 * Sets one CHIP-8 pixel on the monitor: 2 columns wide, and 1 or 2 rows tall
 * so that 32 rows fill 48 (every pair of rows is 3 pix rows high).
 */
static void plot(uint8_t x, uint8_t y, bool on)
{
    uint8_t r = SCR_Y + (3 * y) / 2;
    uint8_t end = SCR_Y + (3 * (y + 1)) / 2;
    uint8_t col = SCR_X + 2 * x;
    unsigned char c = on ? SCR_COLOUR : BLACK;

    for (; r < end; r++) {
        pix[r][col] = c;
        pix[r][col + 1] = c;
    }
}

static void c8_clear_screen(void)
{
    uint8_t r, b;

    for (r = 0; r < C8_H; r++) {
        for (b = 0; b < C8_W / 8; b++) {
            disp[r][b] = 0;
        }
    }
    fill_rect(SCR_X, SCR_Y, 2 * C8_W, (3 * C8_H) / 2, BLACK);
}

/**
 * Resets the machine and copies a ROM to 0x200.
 */
static void c8_load(const uint8_t *data, uint16_t size, uint8_t q, uint8_t ipf)
{
    uint16_t a;

    for (a = 0; a < sizeof(mem); a++) {
        mem[a] = 0;
    }
    for (a = 0; a < sizeof(c8_font); a++) {
        mem[C8_FONT_ADDR + a] = c8_font[a];
    }
    if (size > sizeof(mem) - 0x200) {
        size = sizeof(mem) - 0x200;
    }
    for (a = 0; a < size; a++) {
        mem[0x200 + a] = data[a];
    }
    for (a = 0; a < 16; a++) {
        V[a] = 0;
    }
    I = 0;
    pc = 0x200;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    key_wait = 0;
    bad_opcode = 0;
    quirks = q;
    speed = ipf;
    c8_clear_screen();
}

/**
 * Dxyn: XORs an n row sprite from memory at I onto the screen at (vX, vY).
 * vF = 1 if any pixel was switched off.
 */
static void c8_draw(uint8_t x, uint8_t y, uint8_t n)
{
    uint8_t x0 = V[x] & (C8_W - 1);
    uint8_t y0 = V[y] & (C8_H - 1);
    uint8_t row, bit;

    V[0xF] = 0;
    for (row = 0; row < n; row++) {
        uint8_t py = y0 + row;
        uint8_t bits = mem[(I + row) & 0xFFF];

        if (py >= C8_H) {
            if (quirks & Q_CLIP) {
                break;
            }
            py &= C8_H - 1;
        }
        for (bit = 0; bit < 8; bit++) {
            uint8_t px = x0 + bit;
            uint8_t mask;

            if (!(bits & (0x80 >> bit))) {
                continue;
            }
            if (px >= C8_W) {
                if (quirks & Q_CLIP) {
                    break;
                }
                px &= C8_W - 1;
            }
            mask = 0x80 >> (px & 7);
            if (disp[py][px >> 3] & mask) {
                V[0xF] = 1;
            }
            disp[py][px >> 3] ^= mask;
            plot(px, py, disp[py][px >> 3] & mask);
        }
    }
}

/**
 * Executes one instruction.
 */
static enum step_results c8_step(void)
{
    uint16_t op = ((uint16_t)mem[pc & 0xFFF] << 8) | mem[(pc + 1) & 0xFFF];
    uint8_t x = (op >> 8) & 0xF;
    uint8_t y = (op >> 4) & 0xF;
    uint8_t kk = op & 0xFF;
    uint16_t nnn = op & 0xFFF;
    uint8_t flag;
    uint8_t i;

    pc = (pc + 2) & 0xFFF;

    switch (op >> 12) {
    case 0x0:
        if (op == 0x00E0) {                             // CLS
            c8_clear_screen();
        } else if (op == 0x00EE) {                      // RET
            if (sp == 0) {
                goto bad;
            }
            pc = stack[--sp];
        }                                               // 0nnn SYS: ignored
        break;
    case 0x1:                                           // JP nnn
        pc = nnn;
        break;
    case 0x2:                                           // CALL nnn
        if (sp == 16) {
            goto bad;
        }
        stack[sp++] = pc;
        pc = nnn;
        break;
    case 0x3:                                           // SE vX, kk
        if (V[x] == kk) pc = (pc + 2) & 0xFFF;
        break;
    case 0x4:                                           // SNE vX, kk
        if (V[x] != kk) pc = (pc + 2) & 0xFFF;
        break;
    case 0x5:                                           // SE vX, vY
        if ((op & 0xF) != 0) goto bad;
        if (V[x] == V[y]) pc = (pc + 2) & 0xFFF;
        break;
    case 0x6:                                           // LD vX, kk
        V[x] = kk;
        break;
    case 0x7:                                           // ADD vX, kk
        V[x] += kk;
        break;
    case 0x8:
        switch (op & 0xF) {
        case 0x0: V[x] = V[y]; break;                   // LD
        case 0x1:                                       // OR
            V[x] |= V[y];
            if (quirks & Q_VF_RESET) V[0xF] = 0;
            break;
        case 0x2:                                       // AND
            V[x] &= V[y];
            if (quirks & Q_VF_RESET) V[0xF] = 0;
            break;
        case 0x3:                                       // XOR
            V[x] ^= V[y];
            if (quirks & Q_VF_RESET) V[0xF] = 0;
            break;
        case 0x4: {                                     // ADD with carry
            uint16_t sum = (uint16_t)V[x] + V[y];
            V[x] = (uint8_t)sum;
            V[0xF] = sum > 0xFF;
            break;
        }
        case 0x5:                                       // SUB, vF = no borrow
            flag = V[x] >= V[y];
            V[x] = V[x] - V[y];
            V[0xF] = flag;
            break;
        case 0x6: {                                     // SHR
            uint8_t v = (quirks & Q_SHIFT) ? V[x] : V[y];
            V[x] = v >> 1;
            V[0xF] = v & 1;
            break;
        }
        case 0x7:                                       // SUBN
            flag = V[y] >= V[x];
            V[x] = V[y] - V[x];
            V[0xF] = flag;
            break;
        case 0xE: {                                     // SHL
            uint8_t v = (quirks & Q_SHIFT) ? V[x] : V[y];
            V[x] = v << 1;
            V[0xF] = v >> 7;
            break;
        }
        default:
            goto bad;
        }
        break;
    case 0x9:                                           // SNE vX, vY
        if ((op & 0xF) != 0) goto bad;
        if (V[x] != V[y]) pc = (pc + 2) & 0xFFF;
        break;
    case 0xA:                                           // LD I, nnn
        I = nnn;
        break;
    case 0xB:                                           // JP v0, nnn
        pc = (nnn + ((quirks & Q_JUMP) ? V[x] : V[0])) & 0xFFF;
        break;
    case 0xC:                                           // RND vX, kk
        V[x] = (uint8_t)(next_random() >> 8) & kk;
        break;
    case 0xD:                                           // DRW vX, vY, n
        c8_draw(x, y, op & 0xF);
        return STEP_DREW;
    case 0xE:
        if (kk == 0x9E) {                               // SKP vX
            if (keys & (1u << (V[x] & 0xF))) pc = (pc + 2) & 0xFFF;
        } else if (kk == 0xA1) {                        // SKNP vX
            if (!(keys & (1u << (V[x] & 0xF)))) pc = (pc + 2) & 0xFFF;
        } else {
            goto bad;
        }
        break;
    case 0xF:
        switch (kk) {
        case 0x07: V[x] = delay_timer; break;           // LD vX, DT
        case 0x0A:                                      // LD vX, K
            // Like the COSMAC VIP: wait for a key to be pressed and released.
            if (key_wait == 0) {
                for (i = 0; i < 16; i++) {
                    if (keys & (1u << i)) {
                        key_waited = i;
                        key_wait = 1;
                        break;
                    }
                }
            } else if (!(keys & (1u << key_waited))) {
                V[x] = key_waited;
                key_wait = 0;
                break;
            }
            pc = (pc - 2) & 0xFFF;                      // run it again
            return STEP_BLOCKED;
        case 0x15: delay_timer = V[x]; break;           // LD DT, vX
        case 0x18: sound_timer = V[x]; break;           // LD ST, vX
        case 0x1E: I += V[x]; break;                    // ADD I, vX
        case 0x29:                                      // LD F, vX
            I = C8_FONT_ADDR + 5 * (V[x] & 0xF);
            break;
        case 0x33:                                      // LD B, vX
            mem[I & 0xFFF] = V[x] / 100;
            mem[(I + 1) & 0xFFF] = (V[x] / 10) % 10;
            mem[(I + 2) & 0xFFF] = V[x] % 10;
            break;
        case 0x55:                                      // LD [I], v0..vX
            for (i = 0; i <= x; i++) {
                mem[(I + i) & 0xFFF] = V[i];
            }
            if (quirks & Q_MEMORY) I += x + 1;
            if (quirks & Q_MEMORY_X) I += x;
            break;
        case 0x65:                                      // LD v0..vX, [I]
            for (i = 0; i <= x; i++) {
                V[i] = mem[(I + i) & 0xFFF];
            }
            if (quirks & Q_MEMORY) I += x + 1;
            if (quirks & Q_MEMORY_X) I += x;
            break;
        default:
            goto bad;
        }
        break;
    }
    return STEP_OK;

bad:
    bad_opcode = op;
    pc = (pc - 2) & 0xFFF;
    return STEP_HALTED;
}

/**
 * Runs one 60 Hz frame of the machine: up to `speed` instructions, then the
 * timers. With the display wait quirk a draw ends the frame.
 */
static enum step_results c8_frame(void)
{
    uint8_t n;
    enum step_results r = STEP_OK;

    for (n = 0; n < speed; n++) {
        r = c8_step();
        if (r == STEP_HALTED || r == STEP_BLOCKED ||
            (r == STEP_DREW && (quirks & Q_DISPWAIT))) {
            break;
        }
    }
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
    return r;
}

/******************************************************************************/
/* Inputs                                                                     */
/******************************************************************************/

/**
 * Reads the knob and the buttons once per frame. The knob also works as a
 * three position switch (left / centre / right) with some hysteresis.
 */
static void read_inputs(void)
{
    pot = read_pot();
    last_buttons = buttons;
    buttons = read_button_levels() ^ idle_buttons;

    knob_left = knob_left ? (pot < 380) : (pot < 300);
    knob_right = knob_right ? (pot > 644) : (pot > 724);
}

static bool pressed(uint8_t b)
{
    return (buttons & b) && !(last_buttons & b);
}

/**
 * Knob as a selector of n positions, with hysteresis so the choice does not
 * flicker at a boundary.
 */
static uint8_t knob_select(uint8_t n, uint8_t current)
{
    uint16_t pos = (uint32_t)pot * n * 4 / 1024;    // quarter steps
    uint8_t candidate = pos / 4;

    if (candidate >= n) {
        candidate = n - 1;
    }
    if (candidate != current && current < n) {
        uint8_t quarter = pos % 4;
        if ((candidate == current + 1 && quarter == 0) ||
            (candidate + 1 == current && quarter == 3)) {
            return current;                         // too close to the edge
        }
    }
    return candidate;
}

/**
 * Translates the physical inputs into CHIP-8 keys with the current mapping.
 * S1 and S2 pressed together count as a fifth input, not as both keys.
 */
static uint16_t mapped_keys(void)
{
    bool both = (buttons & (BTN_S1 | BTN_S2)) == (BTN_S1 | BTN_S2);
    bool active[INPUTS];
    uint16_t k = 0;
    uint8_t i;

    active[0] = knob_left;
    active[1] = knob_right;
    active[2] = (buttons & BTN_S1) && !both;
    active[3] = (buttons & BTN_S2) && !both;
    active[4] = both;
    for (i = 0; i < INPUTS; i++) {
        if (active[i] && keymap[i] != KEY_NONE) {
            k |= 1u << keymap[i];
        }
    }
    return k;
}

/******************************************************************************/
/* Screens                                                                    */
/******************************************************************************/
#define MENU_ROWS   5
static const char *const input_names[INPUTS] = {
    "KNOB L", "KNOB R", "S1", "S2", "S1+S2"
};
static const char input_labels[INPUTS] = {'L', 'R', '1', '2', '+'};

static void draw_menu(void)
{
    uint8_t first = (selected >= MENU_ROWS) ? selected - MENU_ROWS + 1 : 0;
    uint8_t i;

    clear_all();
    draw_text(255, 1, "CHIP-8", YELLOW);
    for (i = 0; i < MENU_ROWS && first + i < ROM_COUNT; i++) {
        uint8_t r = first + i;
        if (r == selected) {
            draw_char(4, 12 + 8 * i, '>', WHITE);
        }
        draw_text(14, 12 + 8 * i, roms[r].name, (r == selected) ? WHITE : CYAN);
    }
    draw_text(255, 57, "KNOB + S2", BLUE);
}

static void draw_controls(void)
{
    uint8_t i;
    char text[2];

    clear_all();
    draw_text(255, 1, roms[selected].name, YELLOW);
    for (i = 0; i < INPUTS; i++) {
        uint8_t y = 11 + 9 * i;
        if (i == cursor) {
            draw_char(4, y, '>', WHITE);
        }
        draw_text(14, y, input_names[i], (i == cursor) ? WHITE : CYAN);
        text[0] = (keymap[i] == KEY_NONE) ? '-' : hex_digit(keymap[i]);
        text[1] = '\0';
        draw_text(110, y, text, (keymap[i] == KEY_NONE) ? BLUE : YELLOW);
    }
    draw_text(255, 57, "S1 NEXT S2 GO", BLUE);
}

/**
 * Frame and labels around the running game: the ROM name on top and the key
 * of each input at the bottom ("L:4 R:6 1:5 2:- +:F").
 */
static void draw_game_frame(void)
{
    char text[4 * INPUTS];
    uint8_t i, n = 0;

    clear_all();
    fill_rect(SCR_X - 1, SCR_Y - 1, 2 * C8_W + 2, 1, BLUE);
    fill_rect(SCR_X - 1, SCR_Y + 48, 2 * C8_W + 2, 1, BLUE);
    fill_rect(SCR_X - 1, SCR_Y, 1, 48, BLUE);
    fill_rect(SCR_X + 2 * C8_W, SCR_Y, 1, 48, BLUE);
    draw_text(255, 1, roms[selected].name, YELLOW);

    for (i = 0; i < INPUTS; i++) {
        if (i > 0) {
            text[n++] = ' ';
        }
        text[n++] = input_labels[i];
        text[n++] = ':';
        text[n++] = (keymap[i] == KEY_NONE) ? '-' : hex_digit(keymap[i]);
    }
    text[n] = '\0';
    draw_text_step(255, 58, text, BLUE, 7);
}

static void draw_halted(void)
{
    char text[12] = "BAD OP ";
    uint8_t i;

    for (i = 0; i < 4; i++) {
        text[7 + i] = hex_digit((bad_opcode >> (12 - 4 * i)) & 0xF);
    }
    text[11] = '\0';
    fill_rect(SCR_X, SCR_Y + 18, 2 * C8_W, 11, BLACK);
    draw_text(255, SCR_Y + 21, text, RED);
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
#if defined(__XC16__)
    volatile uint16_t d;

    TRISAbits.TRISA11 = 1;                      // S1 as input
    CNPUAbits.CNPUA11 = 1;                      // pull-ups, in case the
    CNPUAbits.CNPUA12 = 1;                      // board has none
    LATCbits.LATC7 = 0;                         // beep LED off
    for (d = 0; d < 1000; d++);                 // let the pull-ups settle
#endif
    // Whatever level the buttons have now is "not pressed": this works with
    // active low and active high buttons alike.
    idle_buttons = read_button_levels();
    pot = read_pot();
    rng = pot | 1;
    selected = 0;
    state = MENU;
    redraw = true;
}

/**
 * This function has all the user logic. It is called in an endless loop and
 * runs one 60 Hz frame per call.
 */
void MainApp(void)
{
    uint8_t i;

    wait_frame();
    read_inputs();
    next_random();                              // keep the generator moving

    switch (state) {
    case MENU: {
        uint8_t s = knob_select(ROM_COUNT, selected);
        if (s != selected || redraw) {
            selected = s;
            draw_menu();
            redraw = false;
        }
        if (pressed(BTN_S2)) {
            for (i = 0; i < INPUTS; i++) {
                keymap[i] = roms[selected].keys[i];
            }
            cursor = 0;
            pickup = knob_select(17, 255);
            state = CONTROLS;
            draw_controls();
        }
        break;
    }

    case CONTROLS: {
        // The knob only changes the key once it has been moved since the
        // cursor arrived, so moving to the next input does not overwrite it.
        uint8_t v = knob_select(17, pickup);
        if (v != pickup) {
            pickup = v;
            keymap[cursor] = (v == 0) ? KEY_NONE : v - 1;
            draw_controls();
        }
        if (pressed(BTN_S1)) {
            cursor = (cursor + 1) % INPUTS;
            pickup = knob_select(17, 255);
            draw_controls();
        }
        if (pressed(BTN_S2)) {
            draw_game_frame();
            c8_load(roms[selected].data, roms[selected].size,
                    roms[selected].quirks, roms[selected].speed);
            exit_hold = 0;
            state = RUN;
        }
        break;
    }

    case RUN:
        keys = mapped_keys();
        if (c8_frame() == STEP_HALTED) {
            draw_halted();
            state = HALTED;
        }
        set_beep(sound_timer > 0);
        break;

    case HALTED:
        set_beep(false);
        break;
    }

    // Holding S1 and S2 for 1.5 s goes back to the menu from anywhere.
    if ((buttons & (BTN_S1 | BTN_S2)) == (BTN_S1 | BTN_S2) && state != MENU) {
        if (++exit_hold >= 90) {
            set_beep(false);
            exit_hold = 0;
            state = MENU;
            redraw = true;
        }
    } else {
        exit_hold = 0;
    }
}
