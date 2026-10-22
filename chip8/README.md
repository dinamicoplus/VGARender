# CHIP-8 for VGARender

`src/user.c` on this branch is a CHIP-8 interpreter. The ROMs it offers in
its menu are compiled into the firmware from `src/roms.h`, which is generated
from the files in `chip8/roms/`.

## Controls

| Screen   | Knob               | S1                 | S2         |
|----------|--------------------|--------------------|------------|
| Menu     | choose a ROM       |                    | select     |
| Controls | choose the key     | next input         | start      |
| Game     | left / right zones | mapped key         | mapped key |

Hold S1 and S2 together for 1.5 s to go back to the menu from anywhere.

In a game the knob works as two keys: turned to the left third it presses the
key mapped to *knob L*, to the right third the one mapped to *knob R*. The
blue LED of the RGB LED lights while the CHIP-8 sound timer is running.

## Adding ROMs

1. Put the file in `chip8/roms/`:
   - `name.ch8`: a ROM image, with an optional `name.txt` next to it;
   - `name.asm`: source for `asm.py` (see the included games).
2. Optionally describe it in `name.txt` (or in the comments of the `.asm`):

   ```
   name: MY GAME        menu name (A-Z, 0-9, - : . / and spaces)
   keys: 4 6 5 -        CHIP-8 key for knob L, knob R, S1, S2 (- for none)
   quirks: vip          vip (COSMAC VIP, default) or schip
   speed: 12            instructions per frame, 60 frames per second
   ```

3. Run `python3 chip8/make_roms.py` and build the project.

The RAM left for the interpreter allows the standard 4 KB CHIP-8 memory, so
any ROM up to 3584 bytes fits. The included ROMs (DODGE, PONG and KEYTEST) are
original programs written for this project.

## Display

The 64 x 32 CHIP-8 screen is drawn 2 pixels wide and, alternately, 1 or 2
rows tall, so its pixels look nearly square on the monitor. To make room for
the 4 KB of CHIP-8 memory, `pix` is 150 x 64 on this branch (`PIX_H` in
`system.h`), centred vertically by the interrupt (`PIX_Y0`).
