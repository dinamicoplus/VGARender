; KEYTEST - shows the last CHIP-8 key pressed and beeps.
; Useful to check a key mapping: every input shows the key it sends.
;
; name: KEYTEST
; keys: 4 6 5 A         (knob left, knob right, S1, S2)
; quirks: vip
; speed: 12

start:
    CLS
    LD I, dash          ; nothing pressed yet
    LD V1, 28
    LD V2, 15
    DRW V1, V2, 1
loop:
    LD V0, K            ; wait for a key to be pressed and released
    CLS
    LD F, V0
    LD V1, 30
    LD V2, 13
    DRW V1, V2, 5
    LD V3, 6
    LD ST, V3           ; blip
    JP loop

dash:
    DB 0b11111111
