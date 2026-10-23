/**
    User, user.c
    Purpose: This file contains all the user functions. They implement
    DUNGEON, a turn based roguelike in the style of the first Zelda games,
    on the 150 x 96 screen.

    Every game builds new floors of rooms at random. The hero walks from
    room to room (the screen scrolls as in Zelda), fights slimes, bats,
    skeletons, wizards and knights, collects keys, gold and hearts, opens
    chests and, behind a locked door, finds the dragon that guards the
    stairs to the next floor. Each floor is bigger and harder than the last.

    Controls (it is turn based, so there is no hurry):
      knob  turns the hero (left, up, right, down along its travel)
      S1    walks one tile, or attacks what is in front; hold it to walk on
      S2    raises the shield for one turn: blocks hits from the front

    The game is drawn with tiles of 10 x 6 pixels, which look square with
    the tall pixels of this video mode. Only the tiles where something
    changed are redrawn, each one composed in a single pass (floor, item,
    sprites), so the moving sprites do not flicker.

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
#include <string.h>          /* For memmove, memset */

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
/* Geometry                                                                   */
/******************************************************************************/
#define TW      10                  // tile width in pixels
#define TH      6                   // tile height (pixels are tall)
#define RW      15                  // room width in tiles (150 pixels)
#define RH      13                  // room height in tiles (78 pixels)
#define ROOM_Y  18                  // the status bar is above the room
#define ROOM_PH (RH * TH)

#define MAP_W   5                   // a floor is a 5 x 5 grid of rooms
#define MAP_H   5
#define ROOMS   (MAP_W * MAP_H)
#define START_ROOM 22               // bottom centre, as in Zelda

#define ANIM_N  8                   // frames of the animation of a turn

enum directions { N, E, S, W };
static const int8_t dxs[4] = {0, 1, 0, -1};
static const int8_t dys[4] = {-1, 0, 1, 0};
#define OPP(d)  ((d) ^ 2)

// Door tile of each wall; the tile inside the room is one step inwards.
static const uint8_t door_x[4] = {7, RW - 1, 7, 0};
static const uint8_t door_y[4] = {0, 6, RH - 1, 6};
#define CX      7                   // centre tile of the room
#define CY      6

/******************************************************************************/
/* Art                                                                        */
/******************************************************************************/
// Pictures are written as text, one character per pixel:
//   .          transparent
//   k r g y b m c w   black red green yellow blue magenta cyan white
//   A B C      colours of the theme of the current floor
// ctab holds colour + 1 for each character, 0 for transparent.
static const uint8_t ctab[128] = {
    ['k'] = 1, ['r'] = 2, ['g'] = 3, ['y'] = 4,
    ['b'] = 5, ['m'] = 6, ['c'] = 7, ['w'] = 8,
    ['A'] = 9, ['B'] = 10, ['C'] = 11,
};
static uint8_t theme[3];

#define ART(a)  (&(a)[0][0])

// ---- terrain -------------------------------------------------------------
static const char floor_art[TH][TW] = {
    "Ckkkkkkkkk",
    "kkkkkkkkkk",
    "kkkkkkkkkk",
    "kkkkkCkkkk",
    "kkkkkkkkkk",
    "kkkkkkkkkk",
};
static const char wall_art[TH][TW] = {
    "AAAABAAAAA",
    "AAAABAAAAA",
    "BBBBBBBBBB",
    "AAAAAAAABA",
    "AAAAAAAABA",
    "BBBBBBBBBB",
};
static const char block_art[TH][TW] = {
    "BBBBBBBBBA",
    "BAAAAAAAAk",
    "BAACAACAAk",
    "BAAAAAAAAk",
    "BAAAAAAAAk",
    "Akkkkkkkkk",
};
static const char water_art[TH][TW] = {
    "bbbbbbbbbb",
    "bccbbbbbbb",
    "bbbbbbccbb",
    "bbbbbbbbbb",
    "bbbccbbbbb",
    "bbbbbbbbcb",
};
static const char door_ns_art[TH][TW] = {
    "ABkkkkkkBA",
    "ABkkkkkkBA",
    "ABkkkkkkBA",
    "ABkkkkkkBA",
    "ABkkkkkkBA",
    "ABkkkkkkBA",
};
static const char door_we_art[TH][TW] = {
    "BBBBBBBBBB",
    "kkkkkkkkkk",
    "kkkkkkkkkk",
    "kkkkkkkkkk",
    "kkkkkkkkkk",
    "BBBBBBBBBB",
};
static const char locked_ns_art[TH][TW] = {
    "AByyyyyyBA",
    "AByyyyyyBA",
    "AByykkyyBA",
    "AByykkyyBA",
    "AByyyyyyBA",
    "AByyyyyyBA",
};
static const char locked_we_art[TH][TW] = {
    "BBBBBBBBBB",
    "yyyyyyyyyy",
    "yyyykkyyyy",
    "yyyykkyyyy",
    "yyyyyyyyyy",
    "BBBBBBBBBB",
};
static const char shut_ns_art[TH][TW] = {
    "ABCkCCkCBA",
    "ABCkCCkCBA",
    "ABCkCCkCBA",
    "ABCkCCkCBA",
    "ABCkCCkCBA",
    "ABCkCCkCBA",
};
static const char shut_we_art[TH][TW] = {
    "BBBBBBBBBB",
    "CCCCCCCCCC",
    "kkkkkkkkkk",
    "CCCCCCCCCC",
    "kkkkkkkkkk",
    "BBBBBBBBBB",
};
static const char stairs_art[TH][TW] = {
    "wwwwwwwwww",
    "kkkkkkkkkk",
    "kwwwwwwwwk",
    "kkkkkkkkkk",
    "kkkwwwwkkk",
    "kkkkkkkkkk",
};
static const char chest_art[TH][TW] = {
    "..........",
    ".rrrrrrrr.",
    ".ryyyyyyr.",
    ".rrrwwrrr.",
    ".ryyyyyyr.",
    ".rrrrrrrr.",
};
static const char open_chest_art[TH][TW] = {
    ".rrrrrrrr.",
    ".rkkkkkkr.",
    ".rrrrrrrr.",
    ".ryyyyyyr.",
    ".ryyyyyyr.",
    ".rrrrrrrr.",
};

// ---- items lying on the floor ---------------------------------------------
static const char heart_art[TH][TW] = {
    "..........",
    "..rr..rr..",
    ".rrrrrrrr.",
    "..rrrrrr..",
    "...rrrr...",
    "....rr....",
};
static const char rupee_art[TH][TW] = {
    "....yy....",
    "...ywyy...",
    "..yywyyy..",
    "..yyyyyy..",
    "...yyyy...",
    "....yy....",
};
static const char rupee5_art[TH][TW] = {
    "....bb....",
    "...bwbb...",
    "..bbwbbb..",
    "..bbbbbb..",
    "...bbbb...",
    "....bb....",
};
static const char key_art[TH][TW] = {
    "..........",
    ".yyy......",
    "y...yyyyyy",
    ".yyy...y.y",
    "..........",
    "..........",
};
static const char container_art[TH][TW] = {
    ".wwww.www.",
    "wrrrrwrrrw",
    "wrwrrrrrrw",
    ".wrrrrrrw.",
    "..wrrrrw..",
    "....ww....",
};
static const char fairy_art[TH][TW] = {
    "cc......cc",
    "ccc.mm.ccc",
    ".ccmmmmcc.",
    "...mmmm...",
    "..cc..cc..",
    "..........",
};
static const char sword_item_art[TH][TW] = {
    "..........",
    ".y........",
    "yyywwwwwww",
    ".y........",
    "..........",
    "..........",
};

enum items {
    I_NONE, I_HEART, I_RUPEE, I_RUPEE5, I_KEY, I_CONTAINER, I_FAIRY, I_SWORD
};
static const char *const item_art[8] = {
    0, ART(heart_art), ART(rupee_art), ART(rupee5_art), ART(key_art),
    ART(container_art), ART(fairy_art), ART(sword_item_art),
};

// ---- hero -----------------------------------------------------------------
static const char hero_down_art[TH][TW] = {
    "...gggg...",
    "..gggggg..",
    "..ykyyky..",
    ".bbggggyy.",
    ".bbgggggg.",
    "...yy.yy..",
};
static const char hero_up_art[TH][TW] = {
    "...gggg...",
    "..gggggg..",
    "..gggggg..",
    ".yggggggb.",
    "..ggggggb.",
    "..yy.yy...",
};
static const char hero_side_art[TH][TW] = {   // facing left
    "....ggggg.",
    "..yyyggggg",
    "..ykyyg...",
    "..ggggggb.",
    "..ggggggb.",
    "..yy..yy..",
};
static const char hero_hold_art[TH][TW] = {
    ".y......y.",
    ".y.gggg.y.",
    ".yykyykyy.",
    "...gggg...",
    "..gggggg..",
    "...yy.yy..",
};

// ---- enemies --------------------------------------------------------------
static const char slime_art[2][TH][TW] = {{
    "..........",
    "...gggg...",
    "..gggggg..",
    ".ggkggkgg.",
    ".gggggggg.",
    "..gggggg..",
}, {
    "..........",
    "..........",
    "..gggggg..",
    ".ggkggkgg.",
    "gggggggggg",
    ".gggggggg.",
}};
static const char bat_art[2][TH][TW] = {{
    "b........b",
    "bb..bb..bb",
    ".bbbwwbbb.",
    "..bbbbbb..",
    "...b..b...",
    "..........",
}, {
    "..........",
    "....bb....",
    "..bbwwbb..",
    ".bbbbbbbb.",
    "bb.b..b.bb",
    "b........b",
}};
static const char skeleton_art[TH][TW] = {
    "...wwww...",
    "..wkwwkw..",
    "...wwww...",
    ".w.wkkw.w.",
    "...wwww...",
    "..ww..ww..",
};
static const char wizard_art[TH][TW] = {
    ".....mm...",
    "...mmmm...",
    "..mmyymm..",
    ".mmmkkmmm.",
    ".mmmmmmmm.",
    "mmmmmmmmmm",
};
static const char knight_art[TH][TW] = {   // facing left
    "...rrrr...",
    "..rrrrrrr.",
    "..kkkkrrr.",
    "wwrrrrrrcc",
    "..rrrrrrcc",
    "..rr..rr..",
};
static const char fire_art[2][TH][TW] = {{
    "...r..r...",
    "..ryrryr..",
    ".ryywyyr..",
    ".ryywwyr..",
    "..ryyyr...",
    "...rrr....",
}, {
    "..r...r...",
    "...ryyr.r.",
    "..ryywyyr.",
    "..ryywwyr.",
    "...ryyyr..",
    "....rrr...",
}};
#define DW  20                      // the dragon takes 2 x 2 tiles
#define DH  12
static const char dragon_art[2][DH][DW] = {{   // facing left
    "......gggg..........",
    "....ggggggg.........",
    "...gggwkgggg....gg..",
    "..gggggggggg...gggg.",
    ".rrggggg.gggggggggg.",
    "rr.ggggg.ggyyyyygggg",
    "....yyyy.gyyyyyygggg",
    ".........gyyyyyyggg.",
    "........gggyyyyggg..",
    ".......gg.gggggg.gg.",
    "......gg..gg..gg..gg",
    "..........w...w...w.",
}, {
    "......gggg..........",
    "....ggggggg.........",
    "...gggwkgggg....gg..",
    "..gggggggggg...gggg.",
    "rrrrrrgg.gggggggggg.",
    "yyrr.ggg.ggyyyyygggg",
    "rrrrrrgg.gyyyyyygggg",
    "..yyyyy..gyyyyyyggg.",
    "........gggyyyyggg..",
    ".......gg.gggggg.gg.",
    "......gg..gg..gg..gg",
    "..........w...w...w.",
}};

// ---- effects ----------------------------------------------------------------
static const char sword_h_art[TH][TW] = {   // pointing left
    "..........",
    "..........",
    "wwwwwwwyky",
    "........y.",
    "..........",
    "..........",
};
static const char sword_v_art[TH][TW] = {   // pointing up
    "....w.....",
    "....w.....",
    "....w.....",
    "....w.....",
    "...yyy....",
    "....k.....",
};
static const char guard_h_art[TH][TW] = {   // shield to the left
    "....cc....",
    "...cwc....",
    "...cwc....",
    "...cwc....",
    "...cwc....",
    "....cc....",
};
static const char guard_v_art[TH][TW] = {   // shield up
    "..........",
    "..cccccc..",
    ".cwwwwwwc.",
    "..........",
    "..........",
    "..........",
};
static const char puff_art[TH][TW] = {
    "w...w...w.",
    "..w.y.w...",
    ".wyyrryyw.",
    "..w.y.w...",
    "w...w...w.",
    "..........",
};
static const char clang_art[TH][TW] = {
    "....w.....",
    "..w.y.w...",
    "...yyy....",
    "..w.y.w...",
    "....w.....",
    "..........",
};

// ---- status bar -------------------------------------------------------------
static const char heart_icon[3][5][7] = {{
    ".rr.rr.",
    "rrrrrrr",
    ".rrrrr.",
    "..rrr..",
    "...r...",
}, {
    ".rr.bb.",
    "rrrrbbb",
    ".rrrbb.",
    "..rrb..",
    "...r...",
}, {
    ".bb.bb.",
    "bbbbbbb",
    ".bbbbb.",
    "..bbb..",
    "...b...",
}};
static const char key_icon[5][6] = {
    "yyyyyy",
    "yy..yy",
    "yyyyyy",
    "..yy..",
    "..yyyy",
};
static const char rupee_icon[5][6] = {
    "..yy..",
    ".ywyy.",
    "yywyyy",
    ".yyyy.",
    "..yy..",
};
static const char fairy_icon[5][8] = {
    "cc....cc",
    "ccc..ccc",
    ".cmmmmc.",
    "..mmmm..",
    ".cc..cc.",
};
static const char sword_icon[5][8] = {
    "......w.",
    ".....w..",
    "....w...",
    ".yyy....",
    "..y.....",
};

/******************************************************************************/
/* Room layouts                                                               */
/******************************************************************************/
// The inside of a room is 13 x 11 tiles: # block, ~ water, . floor.
// Rooms with a chest, the stairs or a dropped key use layouts with a free
// centre.
static const char layouts[][11][13] = {{
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
}, {
    ".............",
    ".............",
    "..#.......#..",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    "..#.......#..",
    ".............",
    ".............",
}, {
    ".............",
    ".#..#...#..#.",
    ".............",
    ".............",
    ".#..#...#..#.",
    ".............",
    ".#..#...#..#.",
    ".............",
    ".............",
    ".#..#...#..#.",
    ".............",
}, {
    ".............",
    ".............",
    "..####.####..",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    "..####.####..",
    ".............",
    ".............",
}, {
    ".............",
    ".............",
    "...~~~.~~~...",
    "...~.....~...",
    "...~.....~...",
    ".............",
    "...~.....~...",
    "...~.....~...",
    "...~~~.~~~...",
    ".............",
    ".............",
}, {
    ".............",
    ".#.........#.",
    "..#.......#..",
    "...#.....#...",
    ".............",
    ".............",
    ".............",
    "...#.....#...",
    "..#.......#..",
    ".#.........#.",
    ".............",
}, {
    "###.......###",
    "##.........##",
    "#...........#",
    ".............",
    ".............",
    ".............",
    ".............",
    ".............",
    "#...........#",
    "##.........##",
    "###.......###",
}, {
    ".............",
    "..#.#...#.#..",
    ".............",
    "..#.#...#.#..",
    ".............",
    ".............",
    ".............",
    "..#.#...#.#..",
    ".............",
    "..#.#...#.#..",
    ".............",
}, {
    ".............",
    ".............",
    ".............",
    "....~~~~~....",
    "...~~~~~~~...",
    "...~~~~~~~...",
    "...~~~~~~~...",
    "....~~~~~....",
    ".............",
    ".............",
    ".............",
}, {
    ".............",
    ".###.###.###.",
    ".............",
    ".#.#######.#.",
    ".#.........#.",
    ".............",
    ".#.........#.",
    ".#.#######.#.",
    ".............",
    ".###.###.###.",
    ".............",
}, {
    "~~~.......~~~",
    "~~.........~~",
    ".............",
    "...#.....#...",
    ".............",
    ".............",
    ".............",
    "...#.....#...",
    ".............",
    "~~.........~~",
    "~~~.......~~~",
}, {
    ".............",
    ".~~~~...~~~~.",
    ".~.........~.",
    ".~..#...#..~.",
    ".............",
    ".............",
    ".............",
    ".~..#...#..~.",
    ".~.........~.",
    ".~~~~...~~~~.",
    ".............",
}};
#define LAYOUTS (sizeof(layouts) / sizeof(layouts[0]))

/******************************************************************************/
/* Font: 3 x 5 pixels                                                         */
/******************************************************************************/
static const char font_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!-.+*/'";
static const uint8_t font[][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7},
    {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1},
    {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},                           // 0 - 9
    {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6},
    {7, 4, 6, 4, 7}, {7, 4, 6, 4, 4}, {3, 4, 5, 5, 3}, {5, 5, 7, 5, 5},
    {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7},
    {5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {2, 5, 5, 5, 2}, {6, 5, 6, 4, 4},
    {2, 5, 5, 6, 3}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2},
    {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5},
    {5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},                           // A - Z
    {0, 2, 0, 2, 0}, {2, 2, 2, 0, 2}, {0, 0, 7, 0, 0}, {0, 0, 0, 0, 2},
    {0, 2, 7, 2, 0}, {0, 5, 2, 5, 0}, {1, 1, 2, 4, 4}, {2, 2, 0, 0, 0},
};

// Bigger letters for the title, 5 x 5 pixels.
static const char logo_chars[] = "DUNGEO";
static const uint8_t logo_font[][5] = {
    {0x1E, 0x11, 0x11, 0x11, 0x1E}, {0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x19, 0x15, 0x13, 0x11}, {0x0F, 0x10, 0x13, 0x11, 0x0F},
    {0x1F, 0x10, 0x1E, 0x10, 0x1F}, {0x0E, 0x11, 0x11, 0x11, 0x0E},
};

/******************************************************************************/
/* State                                                                      */
/******************************************************************************/
enum terrain {
    T_FLOOR, T_WALL, T_BLOCK, T_WATER, T_DOOR, T_LOCKED, T_SHUT, T_STAIRS,
    T_CHEST, T_OPEN
};

// A map byte: terrain in bits 0-3, item in bits 4-6, bit 7 = tile must be
// redrawn.
#define TERRAIN(t)  ((t) & 0x0F)
#define ITEM(t)     (((t) >> 4) & 7)
#define DIRTY       0x80
static uint8_t map[RH][RW];
static uint16_t reach[RH];          // tiles the hero can walk to (bit x)

enum room_kinds { K_NORMAL, K_START, K_KEY, K_CHEST, K_BOSS, K_TREASURE };
#define R_USED      0x01
#define R_SEEN      0x02
#define R_CLEAR     0x04            // no enemies left
#define R_LOOTED    0x08            // its chest, key or heart was taken
#define R_SHUTTER   0x10            // doors close until it is cleared

struct room {
    uint8_t flags;
    uint8_t doors;                  // bit d: door in wall d
    uint8_t locks;                  // bit d: that door is locked
    uint8_t kind;
    uint8_t layout;
    uint8_t item;                   // content of the chest
};
static struct room rooms[ROOMS];
static uint8_t cur;                 // current room

// What a sprite looks like on the screen now. Comparing it with what it
// should look like tells which tiles must be redrawn.
struct view {
    const char *art;                // 0: hidden
    int16_t x, y;
    uint8_t w, h, flags;
};
#define F_MIRROR    0x01
#define F_FLIP      0x02
#define F_WHITE     0x04

enum ent_types {
    E_NONE, E_HERO, E_SLIME, E_BAT, E_SKELETON, E_WIZARD, E_KNIGHT, E_FIRE,
    E_DRAGON
};
struct ent {
    uint8_t type;
    uint8_t x, y;                   // tile (top left for the dragon)
    uint8_t ox, oy;                 // tile at the start of the turn
    int8_t hp;
    uint8_t dir;
    uint8_t timer;                  // depends on the type
    uint8_t flash;                  // frames left of the hit flash
    uint8_t lunge;                  // attacking: moves half a tile and back
    uint8_t stun;                   // knocked back: skips its turn
    struct view v;
};
#define MAX_ENTS 12
static struct ent ents[MAX_ENTS];
#define hero (ents[0])

struct fx {
    const char *art;
    int16_t x, y;
    uint8_t flags, t;
    struct view v;
};
#define MAX_FX  6
static struct fx fxs[MAX_FX];

enum game_states {
    ST_TITLE, ST_INTRO, ST_PLAY, ST_ANIM, ST_SCROLL, ST_ITEM, ST_DESCEND,
    ST_DYING, ST_OVER
};
static enum game_states state;
static uint16_t timer;              // frames left in the current state
static uint16_t frame;
static uint8_t anim_t = ANIM_N;     // frame of the turn animation
static bool shield;                 // the hero raised the shield this turn
static bool hero_hidden;
static bool hud_dirty;
static uint8_t scroll_dir, scroll_k;
static uint8_t knob_dir;            // 0 - 3 along the knob travel

static uint8_t max_hp, keys, sword, fairy, floor_no, best_floor;
static uint16_t gold;
static bool sword_given;
static uint16_t rng = 0xACE1;

static uint8_t idle_buttons;        // button levels at reset (not pressed)
static uint8_t buttons, last_buttons;
static uint16_t pot;
static uint8_t led_timer;

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

static void set_led(bool on)
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

static uint16_t rnd(void)
{
    rng ^= rng << 7;                            // xorshift16
    rng ^= rng >> 9;
    rng ^= rng << 8;
    return rng;
}

static uint8_t rnd_n(uint8_t n)                 // 0 .. n - 1
{
    return ((uint32_t)rnd() * n) >> 16;
}

/******************************************************************************/
/* Inputs                                                                     */
/******************************************************************************/

static void read_inputs(void)
{
    pot = read_pot();
    last_buttons = buttons;
    buttons = read_button_levels() ^ idle_buttons;
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

/******************************************************************************/
/* Drawing                                                                    */
/******************************************************************************/
static int16_t clip_x0, clip_y0, clip_x1 = PIX_W, clip_y1 = PIX_H;

static void set_clip(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    clip_x0 = x0;
    clip_y0 = y0;
    clip_x1 = x1;
    clip_y1 = y1;
}

static void clip_screen(void)
{
    set_clip(0, 0, PIX_W, PIX_H);
}

/**
 * Draws a w x h picture with its top left corner at (x, y), inside the
 * clipping rectangle.
 */
static void blit(const char *art, uint8_t w, uint8_t h, int16_t x, int16_t y,
                 uint8_t flags)
{
    int16_t i0, i1, j0, j1, i, j;
    int8_t step = (flags & F_MIRROR) ? -1 : 1;
    bool white = flags & F_WHITE;

    if (x >= clip_x1 || y >= clip_y1 || x + w <= clip_x0 || y + h <= clip_y0) {
        return;                                 // nothing to draw
    }
    i0 = clip_x0 - x;
    i1 = clip_x1 - x;
    j0 = clip_y0 - y;
    j1 = clip_y1 - y;
    if (i0 < 0) i0 = 0;
    if (j0 < 0) j0 = 0;
    if (i1 > w) i1 = w;
    if (j1 > h) j1 = h;
    for (j = j0; j < j1; j++) {
        const char *src = art + ((flags & F_FLIP) ? h - 1 - j : j) * w +
                          (step > 0 ? i0 : w - 1 - i0);
        unsigned char *dst = &pix[y + j][0] + x + i0;
        for (i = i0; i < i1; i++, src += step, dst++) {
            uint8_t c = ctab[(uint8_t)*src & 0x7F];
            if (c) {
                if (white) {
                    c = WHITE + 1;
                } else if (c > 8) {
                    c = theme[c - 9] + 1;
                }
                *dst = c - 1;
            }
        }
    }
}

static void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c)
{
    int16_t j;

    for (j = y; j < y + h; j++) {
        memset(&pix[j][x], c, w);
    }
}

static void draw_char(int16_t x, int16_t y, char ch, uint8_t c, uint8_t sx,
                      uint8_t sy)
{
    uint8_t k, row, bit;

    for (k = 0; font_chars[k] != '\0' && font_chars[k] != ch; k++);
    if (font_chars[k] == '\0') {
        return;                                 // space or unknown
    }
    for (row = 0; row < 5; row++) {
        for (bit = 0; bit < 3; bit++) {
            if (font[k][row] & (4 >> bit)) {
                fill_rect(x + bit * sx, y + row * sy, sx, sy, c);
            }
        }
    }
}

/**
 * Draws a string, 4 * sx pixels per character. x < 0 centres it.
 */
static void draw_text(int16_t x, int16_t y, const char *s, uint8_t c,
                      uint8_t sx, uint8_t sy)
{
    if (x < 0) {
        x = (PIX_W - ((int16_t)strlen(s) * 4 - 1) * sx) / 2;
    }
    while (*s) {
        draw_char(x, y, *s++, c, sx, sy);
        x += 4 * sx;
    }
}

static void draw_logo(int16_t x, int16_t y, const char *s, uint8_t c,
                      uint8_t sx, uint8_t sy)
{
    uint8_t k, row, bit;

    for (; *s; s++, x += 6 * sx) {
        for (k = 0; logo_chars[k] != *s; k++);
        for (row = 0; row < 5; row++) {
            for (bit = 0; bit < 5; bit++) {
                if (logo_font[k][row] & (0x10 >> bit)) {
                    fill_rect(x + bit * sx, y + row * sy, sx, sy, c);
                }
            }
        }
    }
}

static void number_text(char *text, uint16_t value, uint8_t digits)
{
    text[digits] = '\0';
    while (digits--) {
        text[digits] = '0' + value % 10;
        value /= 10;
    }
}

/******************************************************************************/
/* Tiles and sprites                                                          */
/******************************************************************************/

static const char *terrain_art(uint8_t t, uint8_t ty)
{
    bool ns = (ty == 0 || ty == RH - 1);    // door in the top or bottom wall

    switch (t) {
    case T_WALL:    return ART(wall_art);
    case T_BLOCK:   return ART(block_art);
    case T_WATER:   return ART(water_art);
    case T_DOOR:    return ns ? ART(door_ns_art) : ART(door_we_art);
    case T_LOCKED:  return ns ? ART(locked_ns_art) : ART(locked_we_art);
    case T_SHUT:    return ns ? ART(shut_ns_art) : ART(shut_we_art);
    case T_STAIRS:  return ART(stairs_art);
    case T_CHEST:   return ART(chest_art);
    case T_OPEN:    return ART(open_chest_art);
    default:        return 0;
    }
}

/**
 * Draws the background of a tile (terrain and item) at (px, py).
 */
static void draw_bg(uint8_t tx, uint8_t ty, int16_t px, int16_t py)
{
    uint8_t t = map[ty][tx];
    const char *art = terrain_art(TERRAIN(t), ty);

    if (TERRAIN(t) != T_WALL && TERRAIN(t) != T_WATER) {
        blit(ART(floor_art), TW, TH, px, py, 0);
    }
    if (art) {
        blit(art, TW, TH, px, py, 0);
    }
    if (ITEM(t)) {
        blit(item_art[ITEM(t)], TW, TH, px, py, 0);
    }
}

/**
 * Where and how an entity is drawn in this frame of the turn animation.
 */
static void ent_view(const struct ent *e, struct view *v)
{
    int16_t x, y, k;
    uint8_t f = (frame >> 4) & 1;               // idle animation

    v->art = 0;
    v->x = v->y = 0;
    v->w = TW;
    v->h = TH;
    v->flags = 0;
    if (e->type == E_NONE || (e->type == E_HERO && hero_hidden)) {
        return;
    }
    x = e->ox * TW + ((int16_t)e->x - e->ox) * TW * anim_t / ANIM_N;
    y = e->oy * TH + ((int16_t)e->y - e->oy) * TH * anim_t / ANIM_N;
    if (e->lunge) {
        k = anim_t <= ANIM_N / 2 ? anim_t : ANIM_N - anim_t;
        x += dxs[e->dir] * TW * k / ANIM_N;
        y += dys[e->dir] * TH * k / ANIM_N;
    }
    v->x = x;
    v->y = ROOM_Y + y;
    if (e->flash & 4) {
        v->flags = F_WHITE;
    }
    switch (e->type) {
    case E_HERO:
        if (state == ST_ITEM) {
            v->art = ART(hero_hold_art);
        } else if (e->dir == N) {
            v->art = ART(hero_up_art);
        } else if (e->dir == S) {
            v->art = ART(hero_down_art);
        } else {
            v->art = ART(hero_side_art);
            if (e->dir == E) v->flags |= F_MIRROR;
        }
        break;
    case E_SLIME:    v->art = ART(slime_art[f]); break;
    case E_BAT:      v->art = ART(bat_art[(frame >> 3) & 1]); break;
    case E_SKELETON:
        v->art = ART(skeleton_art);
        if (f) v->flags |= F_MIRROR;
        break;
    case E_WIZARD:
        v->art = ART(wizard_art);
        if (e->dir == E) v->flags |= F_MIRROR;
        break;
    case E_KNIGHT:
        v->art = ART(knight_art);
        if (e->dir == E) v->flags |= F_MIRROR;
        break;
    case E_FIRE:     v->art = ART(fire_art[(frame >> 2) & 1]); break;
    case E_DRAGON:
        v->art = ART(dragon_art[e->timer % 3 == 2]);    // fire is coming
        v->w = DW;
        v->h = DH;
        if (e->dir == E) v->flags |= F_MIRROR;
        break;
    }
}

static void mark_rect(const struct view *v)
{
    int16_t x0, y0, x1, y1, tx, ty;

    if (!v->art) {
        return;
    }
    x0 = v->x;
    y0 = v->y - ROOM_Y;
    x1 = x0 + v->w - 1;
    y1 = y0 + v->h - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > RW * TW - 1) x1 = RW * TW - 1;
    if (y1 > ROOM_PH - 1) y1 = ROOM_PH - 1;
    if (x0 > x1 || y0 > y1) {
        return;
    }
    for (ty = y0 / TH; ty <= y1 / TH; ty++) {
        for (tx = x0 / TW; tx <= x1 / TW; tx++) {
            map[ty][tx] |= DIRTY;
        }
    }
}

static void update_view(struct view *shown, const struct view *now)
{
    if (shown->art == now->art && shown->x == now->x &&
        shown->y == now->y && shown->flags == now->flags) {
        return;
    }
    mark_rect(shown);
    *shown = *now;
    mark_rect(shown);
}

static void blit_view(const struct view *v)
{
    if (v->art) {
        blit(v->art, v->w, v->h, v->x, v->y, v->flags);
    }
}

/**
 * Redraws one tile completely: background, then every sprite over it.
 */
static void composite(uint8_t tx, uint8_t ty)
{
    int16_t px = tx * TW, py = ROOM_Y + ty * TH;
    uint8_t i;

    set_clip(px, py, px + TW, py + TH);
    draw_bg(tx, ty, px, py);
    for (i = 1; i < MAX_ENTS; i++) {
        blit_view(&ents[i].v);
    }
    blit_view(&hero.v);
    for (i = 0; i < MAX_FX; i++) {
        blit_view(&fxs[i].v);
    }
}

/**
 * Brings the screen up to date: finds the sprites that changed and redraws
 * the tiles they leave and the tiles they cover.
 */
static void render(void)
{
    struct view now;
    uint8_t i, tx, ty;

    for (i = 0; i < MAX_ENTS; i++) {
        ent_view(&ents[i], &now);
        update_view(&ents[i].v, &now);
    }
    for (i = 0; i < MAX_FX; i++) {
        struct fx *f = &fxs[i];
        now.art = f->t ? f->art : 0;
        now.x = f->x;
        now.y = f->y;
        now.w = TW;
        now.h = TH;
        now.flags = f->flags;
        update_view(&f->v, &now);
    }
    for (ty = 0; ty < RH; ty++) {
        for (tx = 0; tx < RW; tx++) {
            if (map[ty][tx] & DIRTY) {
                map[ty][tx] &= ~DIRTY;
                composite(tx, ty);
            }
        }
    }
    clip_screen();
}

/**
 * Forgets what the sprites look like on the screen (it was redrawn).
 */
static void forget_views(void)
{
    uint8_t i;

    for (i = 0; i < MAX_ENTS; i++) {
        ents[i].v.art = 0;
    }
    for (i = 0; i < MAX_FX; i++) {
        fxs[i].v.art = 0;
    }
}

static void redraw_room(void)
{
    uint8_t x, y;

    for (y = 0; y < RH; y++) {
        for (x = 0; x < RW; x++) {
            map[y][x] |= DIRTY;
        }
    }
    forget_views();
}

static void spawn_fx(const char *art, int16_t x, int16_t y, uint8_t flags,
                     uint8_t t)
{
    uint8_t i;

    for (i = 0; i < MAX_FX; i++) {
        if (fxs[i].t == 0 && fxs[i].v.art == 0) {
            fxs[i].art = art;
            fxs[i].x = x;
            fxs[i].y = y;
            fxs[i].flags = flags;
            fxs[i].t = t;
            return;
        }
    }
}

static void fx_at_tile(const char *art, uint8_t tx, uint8_t ty, uint8_t t)
{
    spawn_fx(art, tx * TW, ROOM_Y + ty * TH, 0, t);
}

/******************************************************************************/
/* Status bar                                                                 */
/******************************************************************************/

static void draw_hud(void)
{
    char text[8];
    uint8_t r, i, c;

    clip_screen();
    fill_rect(0, 0, PIX_W, ROOM_Y, BLACK);
    // map of the floor: the rooms already seen
    for (r = 0; r < ROOMS; r++) {
        if (!(rooms[r].flags & R_SEEN)) {
            continue;
        }
        c = (r == cur) ? WHITE : (rooms[r].kind == K_BOSS) ? RED : BLUE;
        fill_rect(2 + (r % MAP_W) * 5, 1 + (r / MAP_W) * 3, 4, 2, c);
    }
    // floor number and hearts
    strcpy(text, "FLOOR ");
    text[6] = '0' + floor_no % 10;
    text[7] = '\0';
    if (floor_no >= 10) {
        text[5] = '0' + floor_no / 10;
        text[6] = '0' + floor_no % 10;
    }
    draw_text(32, 2, text, WHITE, 2, 1);
    for (i = 0; i < max_hp / 2; i++) {
        uint8_t k = hero.hp >= 2 * i + 2 ? 0 : hero.hp == 2 * i + 1 ? 1 : 2;
        blit(ART(heart_icon[k]), 7, 5, 32 + 8 * i, 10, 0);
    }
    // keys and gold
    blit(ART(key_icon), 6, 5, 104, 2, 0);
    text[0] = '*';
    text[1] = '0' + (keys > 9 ? 9 : keys);
    text[2] = '\0';
    draw_text(112, 2, text, WHITE, 2, 1);
    blit(ART(rupee_icon), 6, 5, 104, 10, 0);
    number_text(text, gold, 3);
    draw_text(112, 10, text, WHITE, 2, 1);
    if (fairy) {
        blit(ART(fairy_icon), 8, 5, 132, 2, 0);
    }
    if (sword > 1) {
        blit(ART(sword_icon), 8, 5, 140, 10, 0);
    }
    hud_dirty = false;
}

/******************************************************************************/
/* Floors and rooms                                                           */
/******************************************************************************/

static uint8_t neighbour(uint8_t r, uint8_t d)
{
    int8_t cx = r % MAP_W + dxs[d], cy = r / MAP_W + dys[d];

    if (cx < 0 || cy < 0 || cx >= MAP_W || cy >= MAP_H) {
        return 255;
    }
    return cy * MAP_W + cx;
}

static void connect(uint8_t a, uint8_t d)
{
    uint8_t b = neighbour(a, d);

    rooms[a].doors |= 1 << d;
    rooms[b].doors |= 1 << OPP(d);
}

static uint8_t degree(uint8_t r)
{
    uint8_t d, n = 0;

    for (d = 0; d < 4; d++) {
        if (rooms[r].doors & (1 << d)) n++;
    }
    return n;
}

static void lock_room(uint8_t r)
{
    uint8_t d;

    rooms[r].locks = rooms[r].doors;
    for (d = 0; d < 4; d++) {
        if (rooms[r].doors & (1 << d)) {
            rooms[neighbour(r, d)].locks |= 1 << OPP(d);
        }
    }
}

static bool centre_free(uint8_t layout)
{
    return layouts[layout][4][6] == '.' && layouts[layout][5][5] == '.' &&
           layouts[layout][5][6] == '.' && layouts[layout][5][7] == '.' &&
           layouts[layout][6][6] == '.';
}

/**
 * Picks a random room of the given kind, or 255.
 */
static uint8_t random_room(uint8_t kind)
{
    uint8_t r, n = 0, pick;

    for (r = 0; r < ROOMS; r++) {
        if ((rooms[r].flags & R_USED) && rooms[r].kind == kind) n++;
    }
    if (n == 0) {
        return 255;
    }
    pick = rnd_n(n);
    for (r = 0; r < ROOMS; r++) {
        if ((rooms[r].flags & R_USED) && rooms[r].kind == kind && pick-- == 0) {
            return r;
        }
    }
    return 255;
}

/**
 * Builds a new floor: rooms grown at random from the start room as a tree,
 * the dragon in the farthest dead end behind a locked door, maybe a locked
 * treasure room, a key for every lock and a chest.
 */
static void generate_floor(void)
{
    uint8_t want = 6 + 2 * floor_no, count = 1, r, d, nb, i, locks = 0;
    uint8_t dist[ROOMS], boss = 255, treasure = 255;
    uint16_t tries;
    bool changed;

    if (want > 15) {
        want = 15;
    }
    memset(rooms, 0, sizeof(rooms));
    rooms[START_ROOM].flags = R_USED;
    rooms[START_ROOM].kind = K_START;
    for (tries = 0; count < want && tries < 3000; tries++) {
        r = rnd_n(ROOMS);
        d = rnd() & 3;
        if (!(rooms[r].flags & R_USED) || degree(r) >= 3) continue;
        nb = neighbour(r, d);
        if (nb == 255 || (rooms[nb].flags & R_USED)) continue;
        rooms[nb].flags = R_USED;
        connect(r, d);
        count++;
    }

    // distance of every room from the start
    memset(dist, 255, sizeof(dist));
    dist[START_ROOM] = 0;
    do {
        changed = false;
        for (r = 0; r < ROOMS; r++) {
            if (dist[r] == 255) continue;
            for (d = 0; d < 4; d++) {
                if (!(rooms[r].doors & (1 << d))) continue;
                nb = neighbour(r, d);
                if (dist[nb] > dist[r] + 1) {
                    dist[nb] = dist[r] + 1;
                    changed = true;
                }
            }
        }
    } while (changed);

    // the dragon waits in the farthest dead end
    for (r = 0; r < ROOMS; r++) {
        if ((rooms[r].flags & R_USED) && r != START_ROOM && degree(r) == 1 &&
            (boss == 255 || dist[r] > dist[boss])) {
            boss = r;
        }
    }
    if (boss == 255) {                          // cannot happen with a tree
        boss = random_room(K_NORMAL);
    }
    rooms[boss].kind = K_BOSS;
    lock_room(boss);
    locks++;
    // and maybe a treasure in another one
    for (i = 0; i < 20 && treasure == 255; i++) {
        r = rnd_n(ROOMS);
        if ((rooms[r].flags & R_USED) && rooms[r].kind == K_NORMAL &&
            degree(r) == 1 && dist[r] >= 2) {
            treasure = r;
        }
    }
    if (treasure != 255 && rnd_n(3) != 0) {
        rooms[treasure].kind = K_TREASURE;
        rooms[treasure].item = (max_hp < 16) ? I_CONTAINER :
                               fairy ? I_RUPEE5 : I_FAIRY;
        lock_room(treasure);
        locks++;
    }
    // a few loops, away from the locked rooms
    for (i = 0; i < 4; i++) {
        r = rnd_n(ROOMS);
        d = rnd() & 3;
        nb = neighbour(r, d);
        if (nb == 255 || !(rooms[r].flags & R_USED) ||
            !(rooms[nb].flags & R_USED) || (rooms[r].doors & (1 << d)) ||
            rooms[r].kind == K_BOSS || rooms[r].kind == K_TREASURE ||
            rooms[nb].kind == K_BOSS || rooms[nb].kind == K_TREASURE) {
            continue;
        }
        connect(r, d);
    }
    // a key for every lock, dropped by the enemies or in a chest
    while (locks--) {
        r = random_room(K_NORMAL);
        if (r == 255) break;
        if (rnd() & 1) {
            rooms[r].kind = K_KEY;
        } else {
            rooms[r].kind = K_CHEST;
            rooms[r].item = I_KEY;
        }
    }
    // one more chest
    r = random_room(K_NORMAL);
    if (r != 255) {
        rooms[r].kind = K_CHEST;
        if (!sword_given && floor_no >= 2) {
            rooms[r].item = I_SWORD;
            sword_given = true;
        } else if (!fairy && rnd_n(3) == 0) {
            rooms[r].item = I_FAIRY;
        } else {
            rooms[r].item = I_RUPEE5;
        }
    }
    // layouts and traps
    for (r = 0; r < ROOMS; r++) {
        struct room *rm = &rooms[r];
        if (!(rm->flags & R_USED)) continue;
        do {
            rm->layout = rnd_n(LAYOUTS);
        } while (rm->kind != K_NORMAL && !centre_free(rm->layout));
        if (rm->kind == K_START) {
            rm->layout = 0;
        } else if (rm->kind == K_BOSS) {
            rm->layout = 1;
        }
        if ((rm->kind == K_NORMAL || rm->kind == K_KEY) && rnd_n(3) == 0) {
            rm->flags |= R_SHUTTER;
        }
    }
}

static void set_terrain(uint8_t x, uint8_t y, uint8_t t)
{
    map[y][x] = (map[y][x] & 0xF0) | t | DIRTY;
}

static void set_item(uint8_t x, uint8_t y, uint8_t it)
{
    map[y][x] = (map[y][x] & 0x8F) | (it << 4) | DIRTY;
}

/**
 * Fills map with the room r (walls, doors, layout, chest, stairs, items).
 */
static void build_room(uint8_t r)
{
    const struct room *rm = &rooms[r];
    uint8_t x, y, d;

    for (y = 0; y < RH; y++) {
        for (x = 0; x < RW; x++) {
            uint8_t t = T_WALL;
            if (x > 0 && y > 0 && x < RW - 1 && y < RH - 1) {
                char c = layouts[rm->layout][y - 1][x - 1];
                t = (c == '#') ? T_BLOCK : (c == '~') ? T_WATER : T_FLOOR;
            }
            map[y][x] = t | DIRTY;
        }
    }
    for (d = 0; d < 4; d++) {
        if (rm->doors & (1 << d)) {
            set_terrain(door_x[d], door_y[d],
                        (rm->locks & (1 << d)) ? T_LOCKED : T_DOOR);
        }
        set_terrain(door_x[d] - dxs[d], door_y[d] - dys[d], T_FLOOR);
    }
    switch (rm->kind) {
    case K_START:
        set_terrain(CX, CY + 3, T_FLOOR);
        break;
    case K_CHEST:
    case K_TREASURE:
        set_terrain(CX, CY, (rm->flags & R_LOOTED) ? T_OPEN : T_CHEST);
        break;
    case K_KEY:
        if ((rm->flags & (R_CLEAR | R_LOOTED)) == R_CLEAR) {
            set_item(CX, CY, I_KEY);
        }
        break;
    case K_BOSS:
        if (rm->flags & R_CLEAR) {
            set_terrain(CX, CY, T_STAIRS);
            if (!(rm->flags & R_LOOTED)) {
                set_item(CX, CY + 2, I_CONTAINER);
            }
        }
        break;
    }
}

static bool hero_floor(uint8_t x, uint8_t y)
{
    uint8_t t = TERRAIN(map[y][x]);

    return t == T_FLOOR || t == T_STAIRS;
}

/**
 * Finds the tiles the hero can reach from (ex, ey) and checks that every
 * door can be reached.
 */
static bool room_connected(uint8_t ex, uint8_t ey)
{
    uint8_t x, y, d;
    bool grew;

    memset(reach, 0, sizeof(reach));
    reach[ey] = 1u << ex;
    do {
        grew = false;
        for (y = 1; y < RH - 1; y++) {
            for (x = 1; x < RW - 1; x++) {
                if ((reach[y] & (1u << x)) || !hero_floor(x, y)) continue;
                if ((reach[y] & (1u << (x - 1))) || (reach[y] & (1u << (x + 1))) ||
                    (reach[y - 1] & (1u << x)) || (reach[y + 1] & (1u << x))) {
                    reach[y] |= 1u << x;
                    grew = true;
                }
            }
        }
    } while (grew);
    for (d = 0; d < 4; d++) {
        if ((rooms[cur].doors & (1 << d)) &&
            !(reach[door_y[d] - dys[d]] & (1u << (door_x[d] - dxs[d])))) {
            return false;
        }
    }
    return true;
}

/**
 * Builds the current room around the hero; a layout that cuts the room in
 * two is replaced for good by the empty one.
 */
static void prepare_room(void)
{
    build_room(cur);
    if (!room_connected(hero.x, hero.y)) {
        rooms[cur].layout = 0;
        build_room(cur);
        room_connected(hero.x, hero.y);
    }
}

/******************************************************************************/
/* Entities                                                                   */
/******************************************************************************/

static uint8_t ent_size(const struct ent *e)
{
    return e->type == E_DRAGON ? 2 : 1;
}

static bool covers(const struct ent *e, int8_t x, int8_t y)
{
    uint8_t s = ent_size(e);

    return x >= e->x && x < e->x + s && y >= e->y && y < e->y + s;
}

/**
 * Index of the living entity on tile (x, y), other than skip, or -1.
 */
static int8_t ent_at(int8_t x, int8_t y, uint8_t skip)
{
    uint8_t i;

    for (i = 0; i < MAX_ENTS; i++) {
        if (i != skip && ents[i].type != E_NONE && ents[i].hp > 0 &&
            covers(&ents[i], x, y)) {
            return i;
        }
    }
    return -1;
}

static struct ent *new_ent(uint8_t type, uint8_t x, uint8_t y)
{
    uint8_t i;

    for (i = 1; i < MAX_ENTS; i++) {
        struct ent *e = &ents[i];
        if (e->type == E_NONE && e->v.art == 0) {
            memset(e, 0, sizeof(*e));
            e->type = type;
            e->x = e->ox = x;
            e->y = e->oy = y;
            e->dir = S;
            return e;
        }
    }
    return 0;
}

static uint8_t enemies_left(void)
{
    uint8_t i, n = 0;

    for (i = 1; i < MAX_ENTS; i++) {
        if (ents[i].type != E_NONE && ents[i].type != E_FIRE &&
            ents[i].hp > 0) {
            n++;
        }
    }
    return n;
}

/**
 * Can enemy e (index idx) stand with its top left corner on (x, y)?
 */
static bool can_enter(const struct ent *e, uint8_t idx, int8_t x, int8_t y)
{
    uint8_t s = ent_size(e), i, j, t;

    for (j = 0; j < s; j++) {
        for (i = 0; i < s; i++) {
            int8_t tx = x + i, ty = y + j;
            if (tx < 1 || ty < 1 || tx > RW - 2 || ty > RH - 2) return false;
            t = TERRAIN(map[ty][tx]);
            // Only fire crosses the water: an enemy the hero cannot reach
            // would keep the doors of a trap room closed for ever.
            if (!(t == T_FLOOR || ((t == T_WATER || t == T_STAIRS) &&
                                   e->type == E_FIRE))) {
                return false;
            }
            if (ent_at(tx, ty, idx) >= 0) return false;
        }
    }
    return true;
}

static bool try_move(struct ent *e, uint8_t d)
{
    int8_t x = e->x + dxs[d], y = e->y + dys[d];

    if (!can_enter(e, e - ents, x, y)) {
        return false;
    }
    e->x = x;
    e->y = y;
    e->dir = d;
    return true;
}

static void move_random(struct ent *e)
{
    uint8_t d = rnd() & 3, k;

    for (k = 0; k < 4; k++) {
        if (try_move(e, (d + k) & 3)) return;
    }
}

static void move_toward_hero(struct ent *e)
{
    int8_t ddx = (int8_t)hero.x - e->x, ddy = (int8_t)hero.y - e->y;
    uint8_t dh = ddx > 0 ? E : W, dv = ddy > 0 ? S : N;
    int8_t adx = ddx < 0 ? -ddx : ddx, ady = ddy < 0 ? -ddy : ddy;

    if (adx > ady || (adx == ady && (rnd() & 1))) {
        if (adx && try_move(e, dh)) return;
        if (ady && try_move(e, dv)) return;
    } else {
        if (ady && try_move(e, dv)) return;
        if (adx && try_move(e, dh)) return;
    }
}

/**
 * Direction from e to the hero if they touch (side by side), or 255.
 */
static uint8_t hero_adjacent(const struct ent *e)
{
    uint8_t s = ent_size(e), d, k;

    for (d = 0; d < 4; d++) {
        for (k = 0; k < s; k++) {
            int8_t x = (d == E) ? e->x + s : (d == W) ? e->x - 1 : e->x + k;
            int8_t y = (d == S) ? e->y + s : (d == N) ? e->y - 1 : e->y + k;
            if (covers(&hero, x, y)) return d;
        }
    }
    return 255;
}

/******************************************************************************/
/* Combat                                                                     */
/******************************************************************************/

/**
 * The hero gets a hit from direction d (pointing from the attacker to the
 * hero). The raised shield stops what comes from the front.
 */
static void hurt_hero(uint8_t dmg, uint8_t d)
{
    if (shield && hero.dir == OPP(d)) {
        spawn_fx(ART(clang_art), hero.x * TW + dxs[hero.dir] * 5,
                 ROOM_Y + hero.y * TH + dys[hero.dir] * 3, 0, 10);
        return;
    }
    hero.hp -= dmg;
    if (hero.hp < 0) {
        hero.hp = 0;
    }
    hero.flash = 24;
    led_timer = 12;
    hud_dirty = true;
}

static void melee(struct ent *e, uint8_t d, uint8_t dmg)
{
    e->lunge = 1;
    e->dir = d;
    hurt_hero(dmg, d);
}

static bool fire_can_pass(int8_t x, int8_t y)
{
    uint8_t t;

    if (x < 1 || y < 1 || x > RW - 2 || y > RH - 2) return false;
    t = TERRAIN(map[y][x]);
    return t == T_FLOOR || t == T_WATER || t == T_STAIRS;
}

/**
 * Throws a fireball that starts on (x, y) and flies towards d.
 */
static void shoot(int8_t x, int8_t y, uint8_t d)
{
    struct ent *e;

    if (covers(&hero, x, y)) {
        hurt_hero(1, d);
        return;
    }
    if (!fire_can_pass(x, y) || ent_at(x, y, 255) >= 0) {
        return;
    }
    e = new_ent(E_FIRE, x, y);
    if (e) {
        e->hp = 1;
        e->dir = d;
        e->timer = 1;                           // it starts moving next turn
    }
}

/**
 * Same row or column as the hero, with nothing in between: direction to
 * the hero, or 255.
 */
static uint8_t clear_shot(const struct ent *e)
{
    uint8_t d;
    int8_t x, y;

    if (e->x == hero.x) {
        d = hero.y < e->y ? N : S;
    } else if (e->y == hero.y) {
        d = hero.x < e->x ? W : E;
    } else {
        return 255;
    }
    x = e->x + dxs[d];
    y = e->y + dys[d];
    while (!covers(&hero, x, y)) {
        if (!fire_can_pass(x, y) || ent_at(x, y, 255) >= 0) return 255;
        x += dxs[d];
        y += dys[d];
    }
    return d;
}

static void drop_loot(const struct ent *e)
{
    uint8_t r = rnd_n(10), it = I_NONE;

    if (ITEM(map[e->y][e->x]) || TERRAIN(map[e->y][e->x]) != T_FLOOR) {
        return;
    }
    if (r < 2) {
        it = (hero.hp < max_hp) ? I_HEART : I_RUPEE;
    } else if (r < 5) {
        it = (rnd_n(5) == 0) ? I_RUPEE5 : I_RUPEE;
    }
    if (it) {
        set_item(e->x, e->y, it);
    }
}

/**
 * The hero hits what is on the tile in front.
 */
static void hero_attack(struct ent *e)
{
    uint8_t d = hero.dir;
    const char *art = (d == N || d == S) ? ART(sword_v_art) : ART(sword_h_art);
    uint8_t flags = (d == E) ? F_MIRROR : (d == S) ? F_FLIP : 0;

    spawn_fx(art, hero.x * TW + dxs[d] * 8, ROOM_Y + hero.y * TH + dys[d] * 5,
             flags, ANIM_N + 2);
    if (e->type == E_FIRE) {
        e->hp = 0;
        return;
    }
    e->hp -= sword;
    e->flash = 16;
    if (e->hp > 0 && e->type != E_DRAGON && e->type != E_KNIGHT &&
        try_move(e, d)) {
        e->dir = OPP(d);
        e->stun = 1;                            // knocked back: loses its turn
    }
}

/**
 * One turn of every enemy, after the hero has acted.
 */
static void enemies_turn(void)
{
    uint8_t i, d;

    for (i = 1; i < MAX_ENTS; i++) {
        struct ent *e = &ents[i];
        if (e->type == E_NONE || e->hp <= 0 || e->stun) {
            continue;
        }
        d = hero_adjacent(e);
        switch (e->type) {
        case E_SLIME:                           // slow: every other turn
            e->timer ^= 1;
            if (!e->timer) break;
            if (d != 255) melee(e, d, 1);
            else if (rnd() & 1) move_toward_hero(e);
            else move_random(e);
            break;
        case E_BAT:                             // erratic
            if (d != 255 && (rnd() & 1)) melee(e, d, 1);
            else move_random(e);
            break;
        case E_SKELETON:
            if (d != 255) melee(e, d, 1);
            else if (rnd_n(10) < 7) move_toward_hero(e);
            else move_random(e);
            break;
        case E_KNIGHT:                          // strong and stubborn
            if (d != 255) melee(e, d, 2);
            else move_toward_hero(e);
            break;
        case E_WIZARD:                          // shoots when it sees you
            if (e->timer) e->timer--;
            d = clear_shot(e);
            if (d != 255 && e->timer == 0) {
                e->dir = d;
                shoot(e->x + dxs[d], e->y + dys[d], d);
                e->timer = 3;
            } else if (rnd() & 1) {
                move_random(e);
            }
            break;
        case E_FIRE:
            if (e->timer) {                     // just thrown
                e->timer = 0;
                break;
            }
            {
                int8_t x = e->x + dxs[e->dir], y = e->y + dys[e->dir];
                if (covers(&hero, x, y)) {
                    hurt_hero(1, e->dir);
                    e->x = x;
                    e->y = y;
                    e->hp = 0;
                } else if (!try_move(e, e->dir)) {
                    e->hp = 0;
                }
            }
            break;
        case E_DRAGON:                          // breathes fire every
            e->timer++;                         // third turn
            e->dir = (hero.x < e->x) ? W : E;
            if (e->timer % 3 == 0) {
                int8_t mx = (e->dir == W) ? e->x - 1 : e->x + 2;
                shoot(mx, e->y, e->dir);
                shoot(mx, e->y + 1, e->dir);
            } else if (e->timer % 3 == 1) {
                if (hero.y < e->y) {
                    try_move(e, N);
                } else if (hero.y > e->y + 1) {
                    try_move(e, S);
                } else {
                    try_move(e, (rnd() & 1) ? N : S);
                }
                e->dir = (hero.x < e->x) ? W : E;
            }
            break;
        }
    }
}

/**
 * Puts the enemies of the current room, far enough from the hero.
 */
static void place_enemy(uint8_t type, int8_t hp)
{
    uint8_t tries, x, y;
    int8_t dx, dy;
    struct ent *e;

    for (tries = 0; tries < 60; tries++) {
        x = 1 + rnd_n(RW - 2);
        y = 1 + rnd_n(RH - 2);
        dx = (int8_t)x - hero.x;
        dy = (int8_t)y - hero.y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (!(reach[y] & (1u << x)) || TERRAIN(map[y][x]) != T_FLOOR ||
            ITEM(map[y][x]) || dx + dy < 4 || ent_at(x, y, 255) >= 0) {
            continue;
        }
        e = new_ent(type, x, y);
        if (e) {
            e->hp = hp;
            e->timer = rnd() & 1;
        }
        return;
    }
}

static void spawn_enemies(void)
{
    static const uint8_t types[5] = {E_SLIME, E_BAT, E_SKELETON, E_WIZARD,
                                     E_KNIGHT};
    static const uint8_t costs[5] = {1, 1, 2, 3, 3};
    static const int8_t hps[5] = {1, 1, 2, 2, 4};
    uint8_t kind = rooms[cur].kind, budget, n = 0, tries, k, avail;
    int8_t bonus = (floor_no - 1) / 3;          // tougher deeper down

    if (kind == K_BOSS) {
        struct ent *e = new_ent(E_DRAGON, hero.x > CX ? 2 : RW - 4, 5);
        if (e) {
            e->hp = 4 + 2 * floor_no;
            e->dir = hero.x > CX ? E : W;
        }
        return;
    }
    if (kind == K_START || kind == K_TREASURE) {
        return;
    }
    budget = 1 + floor_no + rnd_n(3);
    if (budget > 10) budget = 10;
    avail = floor_no >= 3 ? 5 : floor_no >= 2 ? 4 : 3;
    for (tries = 0; budget && n < 6 && tries < 30; tries++) {
        k = rnd_n(avail);
        if (costs[k] > budget) continue;
        place_enemy(types[k], hps[k] + bonus);
        budget -= costs[k];
        n++;
    }
}

static void set_shutters(uint8_t from, uint8_t to)
{
    uint8_t d;

    for (d = 0; d < 4; d++) {
        if (TERRAIN(map[door_y[d]][door_x[d]]) == from) {
            set_terrain(door_x[d], door_y[d], to);
        }
    }
}

/**
 * The hero has just come into the current room.
 */
static void enter_room(void)
{
    uint8_t i;

    for (i = 1; i < MAX_ENTS; i++) {
        ents[i].type = E_NONE;
    }
    rooms[cur].flags |= R_SEEN;
    if (!(rooms[cur].flags & R_CLEAR)) {
        spawn_enemies();
        if (enemies_left() == 0) {
            rooms[cur].flags |= R_CLEAR;
        } else if (rooms[cur].flags & R_SHUTTER) {
            set_shutters(T_DOOR, T_SHUT);       // no way out
        }
    }
    hud_dirty = true;
}

static void room_cleared(void)
{
    struct room *rm = &rooms[cur];

    rm->flags |= R_CLEAR;
    set_shutters(T_SHUT, T_DOOR);
    if (rm->kind == K_KEY && !(rm->flags & R_LOOTED)) {
        set_item(CX, CY, I_KEY);
        fx_at_tile(ART(puff_art), CX, CY, 12);
    }
    led_timer = 30;
}

static void dragon_defeated(const struct ent *e)
{
    uint8_t i;

    for (i = 1; i < MAX_ENTS; i++) {            // its fire goes out
        if (ents[i].type == E_FIRE) ents[i].type = E_NONE;
    }
    set_terrain(CX, CY, T_STAIRS);
    set_item(CX, CY + 2, (max_hp < 16) ? I_CONTAINER : I_RUPEE5);
    fx_at_tile(ART(puff_art), e->x + 1, e->y, 20);
    fx_at_tile(ART(puff_art), e->x, e->y + 1, 20);
}

/******************************************************************************/
/* Game flow                                                                  */
/******************************************************************************/

static void set_theme(void)
{
    static const uint8_t themes[6][3] = {
        {BLUE, CYAN, BLUE}, {GREEN, YELLOW, GREEN}, {RED, MAGENTA, RED},
        {MAGENTA, BLUE, MAGENTA}, {YELLOW, RED, YELLOW}, {CYAN, BLUE, CYAN},
    };
    const uint8_t *t = themes[(floor_no - 1) % 6];

    theme[0] = t[0];
    theme[1] = t[1];
    theme[2] = t[2];
}

static void title_screen(void)
{
    clip_screen();
    memset(pix, BLACK, sizeof(pix));
    draw_logo(15, 6, "DUNGEON", RED, 3, 3);    // shadow
    draw_logo(13, 5, "DUNGEON", YELLOW, 3, 3);
    blit(ART(slime_art[0]), TW, TH, 30, 32, 0);
    blit(ART(skeleton_art), TW, TH, 48, 32, 0);
    blit(ART(hero_down_art), TW, TH, 70, 32, 0);
    blit(ART(sword_v_art), TW, TH, 79, 27, 0);
    blit(ART(bat_art[0]), TW, TH, 92, 32, 0);
    blit(ART(wizard_art), TW, TH, 110, 32, 0);
    draw_text(-1, 60, "KNOB: TURN", CYAN, 2, 1);
    draw_text(-1, 68, "S1: WALK / HIT", CYAN, 2, 1);
    draw_text(-1, 76, "S2: SHIELD", CYAN, 2, 1);
    state = ST_TITLE;
    timer = 0;
}

static void game_over_screen(void)
{
    char text[12];

    clip_screen();
    memset(pix, BLACK, sizeof(pix));
    if (floor_no > best_floor) {
        best_floor = floor_no;
    }
    draw_text(-1, 14, "GAME OVER", RED, 4, 2);
    strcpy(text, "FLOOR ");
    number_text(text + 6, floor_no, 2);
    draw_text(-1, 38, text, WHITE, 2, 1);
    strcpy(text, "GOLD ");
    number_text(text + 5, gold, 3);
    draw_text(-1, 48, text, YELLOW, 2, 1);
    strcpy(text, "BEST ");
    number_text(text + 5, best_floor, 2);
    draw_text(-1, 62, text, CYAN, 2, 1);
    state = ST_OVER;
    timer = 0;
}

/**
 * Builds a new floor and shows its number for a moment.
 */
static void start_floor(void)
{
    char text[9];
    int8_t hp = hero.hp;

    set_theme();
    generate_floor();
    cur = START_ROOM;
    memset(ents, 0, sizeof(ents));
    memset(fxs, 0, sizeof(fxs));
    hero.type = E_HERO;
    hero.x = hero.ox = CX;
    hero.y = hero.oy = CY + 3;
    hero.dir = N;
    hero.hp = hp > 0 ? hp : max_hp;
    hero_hidden = false;
    anim_t = ANIM_N;
    prepare_room();
    rooms[cur].flags |= R_SEEN;
    draw_hud();
    fill_rect(0, ROOM_Y, PIX_W, ROOM_PH, BLACK);
    strcpy(text, "FLOOR ");
    number_text(text + 6, floor_no, 2);
    if (text[6] == '0') {
        text[6] = text[7];
        text[7] = '\0';
    }
    draw_text(-1, ROOM_Y + 32, text, WHITE, 3, 2);
    state = ST_INTRO;
    timer = 100;
}

static void new_game(void)
{
    rng ^= frame ^ (pot << 5);
    if (rng == 0) rng = 0xACE1;
    max_hp = 6;
    hero.hp = 6;
    keys = 0;
    gold = 0;
    sword = 1;
    fairy = 0;
    sword_given = false;
    floor_no = 1;
    start_floor();
}

/**
 * Shows an item over the hero's head with a message, as in Zelda.
 */
static void item_banner(uint8_t it, const char *text)
{
    int16_t y = (hero.y > CY) ? ROOM_Y + 7 : PIX_H - 17;

    state = ST_ITEM;
    timer = 100;
    spawn_fx(item_art[it], hero.x * TW, ROOM_Y + (hero.y - 1) * TH, 0, 255);
    render();
    fill_rect(2, y, PIX_W - 4, 11, WHITE);
    fill_rect(3, y + 1, PIX_W - 6, 9, BLACK);
    draw_text(-1, y + 3, text, YELLOW, 2, 1);
    led_timer = 40;
}

static void give_item(uint8_t it, bool from_chest)
{
    switch (it) {
    case I_HEART:
        hero.hp += 2;
        if (hero.hp > max_hp) hero.hp = max_hp;
        break;
    case I_RUPEE:
        gold++;
        break;
    case I_RUPEE5:
        gold += from_chest ? 20 : 5;
        if (from_chest) item_banner(it, "20 GOLD!");
        break;
    case I_KEY:
        keys++;
        rooms[cur].flags |= R_LOOTED;
        if (from_chest) item_banner(it, "A KEY!");
        break;
    case I_CONTAINER:
        if (max_hp < 16) max_hp += 2;
        hero.hp = max_hp;
        rooms[cur].flags |= R_LOOTED;
        item_banner(it, "HEART CONTAINER!");
        break;
    case I_FAIRY:
        fairy = 1;
        item_banner(it, "FAIRY IN A BOTTLE");
        break;
    case I_SWORD:
        sword = 2;
        item_banner(it, "MAGIC SWORD!");
        break;
    }
    if (gold > 999) gold = 999;
    hud_dirty = true;
}

/**
 * The hero acts: S1 walks or hits in front, S2 raises the shield.
 * Returns true if this took a turn.
 */
static bool hero_act(bool walk)
{
    uint8_t d = hero.dir, t;
    int8_t x = hero.x + dxs[d], y = hero.y + dys[d], k;

    if (!walk) {
        shield = true;
        spawn_fx((d == N || d == S) ? ART(guard_v_art) : ART(guard_h_art),
                 hero.x * TW + dxs[d] * 6, ROOM_Y + hero.y * TH + dys[d] * 4,
                 (d == E) ? F_MIRROR : (d == S) ? F_FLIP : 0, ANIM_N + 4);
        return true;
    }
    if (x < 0 || y < 0 || x >= RW || y >= RH) {
        return false;
    }
    k = ent_at(x, y, 0);
    if (k > 0) {
        hero_attack(&ents[k]);
        return true;
    }
    t = TERRAIN(map[y][x]);
    if (t == T_LOCKED) {
        if (keys == 0) return false;
        keys--;
        rooms[cur].locks &= ~(1 << d);
        rooms[neighbour(cur, d)].locks &= ~(1 << OPP(d));
        set_terrain(x, y, T_DOOR);
        fx_at_tile(ART(puff_art), x, y, 12);
        hud_dirty = true;
        return true;
    }
    if (t == T_CHEST) {
        set_terrain(x, y, T_OPEN);
        rooms[cur].flags |= R_LOOTED;
        give_item(rooms[cur].item, true);
        return false;                           // the banner pauses the game
    }
    if (!(t == T_FLOOR || t == T_STAIRS || t == T_DOOR)) {
        return false;
    }
    hero.x = x;
    hero.y = y;
    return true;
}

static void start_scroll(uint8_t d)
{
    uint8_t i;

    for (i = 1; i < MAX_ENTS; i++) {
        ents[i].type = E_NONE;
    }
    for (i = 0; i < MAX_FX; i++) {
        fxs[i].t = 0;
    }
    hero_hidden = true;
    render();                                   // the room without sprites
    cur = neighbour(cur, d);
    hero.x = hero.ox = door_x[OPP(d)] + dxs[d];
    hero.y = hero.oy = door_y[OPP(d)] + dys[d];
    hero.dir = d;
    prepare_room();
    scroll_dir = d;
    scroll_k = 0;
    state = ST_SCROLL;
}

/**
 * Draws the part of the new room that comes into view: the room is drawn
 * shifted by (ox, oy) pixels and clipped to the strip that was uncovered.
 */
static void draw_room_part(int16_t ox, int16_t oy)
{
    uint8_t tx, ty;

    for (ty = 0; ty < RH; ty++) {
        int16_t py = ROOM_Y + oy + ty * TH;
        if (py + TH <= clip_y0 || py >= clip_y1) continue;
        for (tx = 0; tx < RW; tx++) {
            int16_t px = ox + tx * TW;
            if (px + TW <= clip_x0 || px >= clip_x1) continue;
            draw_bg(tx, ty, px, py);
        }
    }
}

/**
 * One step of the room transition: the old room slides out, the new one in.
 */
static void scroll_update(void)
{
    uint8_t vertical = (scroll_dir == N || scroll_dir == S);
    uint8_t step = vertical ? 3 : 5, steps = vertical ? ROOM_PH / 3 : PIX_W / 5;
    int16_t k = ++scroll_k * step, y;

    switch (scroll_dir) {
    case E:
        for (y = ROOM_Y; y < PIX_H; y++) {
            memmove(&pix[y][0], &pix[y][step], PIX_W - step);
        }
        set_clip(PIX_W - step, ROOM_Y, PIX_W, PIX_H);
        draw_room_part(PIX_W - k, 0);
        break;
    case W:
        for (y = ROOM_Y; y < PIX_H; y++) {
            memmove(&pix[y][step], &pix[y][0], PIX_W - step);
        }
        set_clip(0, ROOM_Y, step, PIX_H);
        draw_room_part(k - PIX_W, 0);
        break;
    case S:
        memmove(&pix[ROOM_Y][0], &pix[ROOM_Y + step][0],
                (ROOM_PH - step) * PIX_W);
        set_clip(0, PIX_H - step, PIX_W, PIX_H);
        draw_room_part(0, ROOM_PH - k);
        break;
    case N:
        memmove(&pix[ROOM_Y + step][0], &pix[ROOM_Y][0],
                (ROOM_PH - step) * PIX_W);
        set_clip(0, ROOM_Y, PIX_W, ROOM_Y + step);
        draw_room_part(0, k - ROOM_PH);
        break;
    }
    clip_screen();
    if (scroll_k == steps) {
        uint8_t x, yy;
        for (yy = 0; yy < RH; yy++) {
            for (x = 0; x < RW; x++) {
                map[yy][x] &= ~DIRTY;           // it is all on the screen
            }
        }
        forget_views();
        hero_hidden = false;
        enter_room();
        state = ST_PLAY;
    }
}

/**
 * The animation of the turn has finished: tidy up and see what happens.
 */
static void end_turn(void)
{
    uint8_t i, t, it;

    for (i = 0; i < MAX_ENTS; i++) {
        ents[i].ox = ents[i].x;
        ents[i].oy = ents[i].y;
        ents[i].lunge = 0;
        ents[i].stun = 0;
        if (ents[i].type == E_FIRE) {
            ents[i].timer = 0;                  // thrown fire moves next turn
        }
    }
    shield = false;
    for (i = 1; i < MAX_ENTS; i++) {
        struct ent *e = &ents[i];
        if (e->type == E_NONE || e->hp > 0) continue;
        if (e->type != E_FIRE) {
            fx_at_tile(ART(puff_art), e->x, e->y, 12);
            if (e->type == E_DRAGON) {
                dragon_defeated(e);
            } else {
                drop_loot(e);
            }
        }
        e->type = E_NONE;
    }
    state = ST_PLAY;
    if (!(rooms[cur].flags & R_CLEAR) && enemies_left() == 0) {
        room_cleared();
    }
    if (hero.hp <= 0) {
        if (fairy) {
            fairy = 0;
            hero.hp = max_hp;
            item_banner(I_FAIRY, "SAVED BY THE FAIRY");
        } else {
            state = ST_DYING;
            timer = 90;
        }
        hud_dirty = true;
        return;
    }
    t = map[hero.y][hero.x];
    it = ITEM(t);
    if (it) {
        set_item(hero.x, hero.y, I_NONE);
        give_item(it, false);
    }
    if (state != ST_PLAY) {
        return;
    }
    switch (TERRAIN(t)) {
    case T_DOOR:
        start_scroll(hero.y == 0 ? N : hero.y == RH - 1 ? S :
                     hero.x == 0 ? W : E);
        break;
    case T_STAIRS:
        state = ST_DESCEND;
        timer = 0;
        break;
    }
}

static void start_turn(bool walk)
{
    uint8_t i;

    for (i = 0; i < MAX_ENTS; i++) {
        ents[i].ox = ents[i].x;
        ents[i].oy = ents[i].y;
    }
    if (!hero_act(walk)) {
        return;
    }
    if (TERRAIN(map[hero.y][hero.x]) != T_DOOR) {
        enemies_turn();
    }
    anim_t = 0;
    state = ST_ANIM;
}

static void play_update(void)
{
    static const uint8_t knob_dirs[4] = {W, N, E, S};

    knob_dir = knob_select(4, knob_dir);
    hero.dir = knob_dirs[knob_dir];
    if (buttons & BTN_S1) {
        start_turn(true);
    } else if (pressed(BTN_S2)) {
        start_turn(false);
    }
}

/**
 * Frame counters of the sprites and effects.
 */
static void tick(void)
{
    uint8_t i;

    for (i = 0; i < MAX_ENTS; i++) {
        if (ents[i].flash) ents[i].flash--;
    }
    for (i = 0; i < MAX_FX; i++) {
        if (fxs[i].t && fxs[i].t != 255) fxs[i].t--;
    }
    if (led_timer) {
        led_timer--;
    }
    set_led(led_timer & 4);
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
    LATCbits.LATC7 = 0;                         // LED off
    for (d = 0; d < 1000; d++);                 // let the pull-ups settle
#endif
    // Whatever level the buttons have now is "not pressed": this works with
    // active low and active high buttons alike.
    idle_buttons = read_button_levels();
    pot = read_pot();
    knob_dir = knob_select(4, 255);
    title_screen();
}

/**
 * This function has all the user logic. It is called in an endless loop and
 * runs one 60 Hz frame per call.
 */
void MainApp(void)
{
    wait_frame();
    frame++;
    read_inputs();

    switch (state) {
    case ST_TITLE:
        timer++;
        fill_rect(0, 46, PIX_W, 5, BLACK);
        if (timer & 32) {
            draw_text(-1, 46, "PRESS S1", WHITE, 2, 1);
        }
        if (pressed(BTN_S1)) {
            new_game();
        }
        return;
    case ST_OVER:
        timer++;
        fill_rect(0, 78, PIX_W, 5, BLACK);
        if (timer > 60 && (timer & 32)) {
            draw_text(-1, 78, "PRESS S1", WHITE, 2, 1);
        }
        if (timer > 60 && pressed(BTN_S1)) {
            title_screen();
        }
        return;
    case ST_INTRO:
        if (--timer == 0) {
            redraw_room();
            enter_room();
            state = ST_PLAY;
        }
        break;
    case ST_SCROLL:
        scroll_update();
        if (state == ST_SCROLL) return;
        break;
    case ST_PLAY:
        play_update();
        break;
    case ST_ANIM:
        if (++anim_t >= ANIM_N) {
            anim_t = ANIM_N;
            end_turn();
        }
        break;
    case ST_ITEM:
        if (--timer == 0) {
            uint8_t i;
            for (i = 0; i < MAX_FX; i++) {
                if (fxs[i].t == 255) fxs[i].t = 0;
            }
            redraw_room();                      // removes the banner
            state = ST_PLAY;
        } else {
            tick();
            return;                             // the game waits
        }
        break;
    case ST_DESCEND:                            // the room fades to black
        fill_rect(0, ROOM_Y + timer * 3, PIX_W, 3, BLACK);
        if (++timer == ROOM_PH / 3) {
            floor_no++;
            start_floor();
        }
        return;
    case ST_DYING:                              // the hero spins and falls
        hero.dir = (timer >> 3) & 3;
        hero.flash = 0;
        if (--timer == 0) {
            set_led(false);
            game_over_screen();
            return;
        }
        break;
    }
    if (state == ST_INTRO) {
        return;
    }
    tick();
    render();
    if (hud_dirty) {
        draw_hud();
    }
}
