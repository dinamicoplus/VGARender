; SOLO PONG - one player against the computer. The first to 9 points wins;
; then press any key to play again.
;
; name: SOLO PONG
; keys: 1 4 - -         (knob left = up, knob right = down)
; quirks: vip
; speed: 12
;
; Registers:
;   V1      player paddle y (x = 1)     V7      player score
;   V2      computer paddle y (x = 61)  V8      computer score
;   V3, V4  ball x, y                   V9      frames per step
;   V5, V6  ball dx, dy (1 or -1)       VA      computer move counter
;   V0, VB, VC, VD                      scratch

PADDLE_H  = 6
PADDLE_MAX = 26         ; 32 - PADDLE_H
PLAYER_X  = 1
CPU_X     = 61

start:
    LD V7, 0
    LD V8, 0
    LD V9, 3            ; 20 steps per second
new_game_screen:
    CLS
    LD V1, 13
    LD V2, 13
    LD I, paddle
    LD V0, PLAYER_X
    DRW V0, V1, PADDLE_H
    LD V0, CPU_X
    DRW V0, V2, PADDLE_H
    CALL serve

loop:
    LD V0, DT
    SE V0, 0
    JP loop
    LD DT, V9

    ; Player paddle: key 1 up, key 4 down.
    LD VB, V1
    LD V0, 1
    SKNP V0
    CALL up
    LD V0, 4
    SKNP V0
    CALL down
    SNE VB, V1
    JP computer
    LD I, paddle
    LD V0, PLAYER_X
    DRW V0, V1, PADDLE_H
    LD V1, VB
    DRW V0, V1, PADDLE_H

computer:
    ; Moves every other step towards the ball, so it can be beaten.
    ADD VA, 1
    LD VC, 1
    AND VA, VC
    SE VA, 0
    JP ball
    LD VB, V2
    LD VC, V2
    ADD VC, 3           ; paddle centre
    LD VD, VC
    SUB VD, V4          ; VF = 1 if centre >= ball y
    SE VF, 1
    CALL down
    LD VD, V4
    SUB VD, VC          ; VF = 1 if ball y >= centre
    SE VF, 1
    CALL up
    SNE VB, V2
    JP ball
    LD I, paddle
    LD V0, CPU_X
    DRW V0, V2, PADDLE_H
    LD V2, VB
    DRW V0, V2, PADDLE_H

ball:
    LD I, dot
    DRW V3, V4, 1       ; erase
    ADD V3, V5
    ADD V4, V6
    SNE V4, 0
    CALL flip_dy
    SNE V4, 31
    CALL flip_dy
    SNE V3, 0
    JP computer_scores
    SNE V3, 63
    JP player_scores
    DRW V3, V4, 1
    SE VF, 1
    JP loop
    ; Hit a paddle: undo the horizontal move and bounce.
    DRW V3, V4, 1
    LD VC, 0
    SUB VC, V5
    LD V5, VC
    ADD V3, V5
    LD V0, 1
    LD ST, V0           ; blip
    DRW V3, V4, 1
    JP loop

flip_dy:
    LD VC, 0
    SUB VC, V6
    LD V6, VC
    RET

; VB = VB - 1, not below 0
up:
    SE VB, 0
    ADD VB, -1
    RET

; VB = VB + 1, not below the bottom
down:
    SE VB, PADDLE_MAX
    ADD VB, 1
    RET

player_scores:
    ADD V7, 1
    JP point
computer_scores:
    ADD V8, 1
point:
    LD V0, 10
    LD ST, V0
    CALL show_score
    LD V0, 60           ; show it for 1 s
    LD DT, V0
point_wait:
    LD V0, DT
    SE V0, 0
    JP point_wait
    SNE V7, 9
    JP game_over
    SNE V8, 9
    JP game_over
    CALL show_score     ; erase it (XOR)
    CALL serve
    JP loop

game_over:
    LD V0, 90           ; ignore keys for 1.5 s
    LD DT, V0
over_wait:
    LD V0, DT
    SE V0, 0
    JP over_wait
    LD V0, K
    JP start

; Draws (or erases) both scores at the top.
show_score:
    LD VB, 24
    LD VC, 1
    LD F, V7
    DRW VB, VC, 5
    LD VB, 36
    LD F, V8
    DRW VB, VC, 5
    RET

; Ball to the centre, random height and vertical direction, towards the
; side that did not score last (V5 keeps the last direction).
serve:
    LD V3, 32
    RND V4, 15
    ADD V4, 8
    LD V6, 1
    RND V0, 1
    SE V0, 0
    LD V6, -1
    SNE V5, 0
    LD V5, 1
    LD I, dot
    DRW V3, V4, 1
    RET

paddle:
    DB 0b11000000, 0b11000000, 0b11000000
    DB 0b11000000, 0b11000000, 0b11000000
dot:
    DB 0b10000000
