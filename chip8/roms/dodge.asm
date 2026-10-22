; DODGE - move left and right to avoid the falling rocks.
; Every rock that reaches the ground scores a point, and every 10 points
; the rocks fall faster. When a rock hits you, your score is shown; press
; any key to play again.
;
; name: DODGE
; keys: 4 6 - 5         (knob left, knob right, S1, S2)
; quirks: vip
; speed: 12
;
; Registers:
;   V1      player x            V8      score
;   V2, V3  rock A x, y         V9      frames per step
;   V4, V5  rock B x, y         VA      points until speed up
;   V6, V7  rock C x, y         VB      new player x / new rock x
;   V0, VC, VD                  scratch

PLAYER_Y = 28
GROUND   = 30           ; rocks are 3 rows tall: respawn before clipping
DANGER   = 24           ; rocks above this row can't touch the player

start:
    CLS
    LD V1, 29
    LD V8, 0
    LD V9, 6            ; 10 steps per second at first
    LD VA, 10
    CALL new_x
    LD V2, VB
    LD V3, 0
    CALL new_x
    LD V4, VB
    LD V5, 10
    CALL new_x
    LD V6, VB
    LD V7, 20

    LD I, player
    LD V0, PLAYER_Y
    DRW V1, V0, 3
    LD I, rock
    DRW V2, V3, 3
    DRW V4, V5, 3
    DRW V6, V7, 3

    LD V0, 60           ; one second to get ready
    LD DT, V0

loop:
    LD V0, DT           ; wait for the next step
    SE V0, 0
    JP loop
    LD DT, V9

    ; Player: 2 pixels per step.
    LD VB, V1
    LD V0, 4
    SKNP V0
    CALL left
    LD V0, 6
    SKNP V0
    CALL right
    SNE VB, V1
    JP rocks
    LD I, player
    LD V0, PLAYER_Y
    DRW V1, V0, 3       ; erase
    LD V1, VB
    DRW V1, V0, 3
    SE VF, 0
    JP game_over

rocks:
    LD I, rock

    DRW V2, V3, 3       ; rock A
    ADD V3, 1
    SE V3, GROUND
    JP draw_a
    LD V3, 0
    CALL new_x
    LD V2, VB
    CALL scored
    LD I, rock
draw_a:
    DRW V2, V3, 3
    SE VF, 1
    JP rock_b
    LD VC, DANGER
    SUB VC, V3          ; VF = 1 if the rock is still high up
    SE VF, 1
    JP game_over

rock_b:
    DRW V4, V5, 3
    ADD V5, 1
    SE V5, GROUND
    JP draw_b
    LD V5, 0
    CALL new_x
    LD V4, VB
    CALL scored
    LD I, rock
draw_b:
    DRW V4, V5, 3
    SE VF, 1
    JP rock_c
    LD VC, DANGER
    SUB VC, V5
    SE VF, 1
    JP game_over

rock_c:
    DRW V6, V7, 3
    ADD V7, 1
    SE V7, GROUND
    JP draw_c
    LD V7, 0
    CALL new_x
    LD V6, VB
    CALL scored
    LD I, rock
draw_c:
    DRW V6, V7, 3
    SE VF, 1
    JP loop
    LD VC, DANGER
    SUB VC, V7
    SE VF, 1
    JP game_over
    JP loop

; VB = VB - 2, not below 0
left:
    SE VB, 0
    ADD VB, -1
    SE VB, 0
    ADD VB, -1
    RET

; VB = VB + 2, not beyond 59 (the player is 5 pixels wide)
right:
    SE VB, 59
    ADD VB, 1
    SE VB, 59
    ADD VB, 1
    RET

; VB = random column 0 .. 59 for a rock (4 pixels wide)
new_x:
    RND VB, 63
    LD VC, 59
    SUB VC, VB          ; VF = 1 if VB <= 59
    SE VF, 0
    RET
    ADD VB, -8
    RET

; One more point; every 10 points one frame less per step (fastest: 1).
scored:
    ADD V8, 1
    LD V0, 2
    LD ST, V0           ; short blip
    ADD VA, -1
    SE VA, 0
    RET
    LD VA, 10
    SE V9, 1
    ADD V9, -1
    RET

game_over:
    LD V0, 30
    LD ST, V0           ; long beep
    CLS
    LD I, digits
    LD B, V8            ; hundreds, tens, ones
    LD V2, [I]          ; into V0, V1, V2
    LD VB, 23
    LD VC, 13
    LD F, V0
    DRW VB, VC, 5
    ADD VB, 7
    LD F, V1
    DRW VB, VC, 5
    ADD VB, 7
    LD F, V2
    DRW VB, VC, 5
    LD V0, 90           ; ignore keys for 1.5 s
    LD DT, V0
wait:
    LD V0, DT
    SE V0, 0
    JP wait
    LD V0, K
    JP start

player:
    DB 0b00100000
    DB 0b01110000
    DB 0b11111000
rock:
    DB 0b01100000
    DB 0b11110000
    DB 0b01100000
digits:
    DB 0, 0, 0
