#!/usr/bin/env python3
"""
Minimal CHIP-8 assembler (Cowgod's mnemonics).

    python3 asm.py program.asm program.ch8

Syntax, one instruction per line, ';' starts a comment:

    label:                      define a label (its address)
    name = 42                   define a constant
    CLS / RET
    JP addr     JP V0, addr     CALL addr
    SE Vx, byte|Vy      SNE Vx, byte|Vy
    LD Vx, byte|Vy|DT|K|[I]     LD I, addr      LD DT|ST|F|B|[I], Vx
    ADD Vx, byte|Vy     ADD I, Vx
    OR AND XOR SUB SUBN Vx, Vy      SHR SHL Vx
    RND Vx, byte        DRW Vx, Vy, n       SKP Vx      SKNP Vx
    DB b, b, ...        raw bytes (numbers, labels or constants)

Numbers: 12, 0x0C, #0C or 0b00001100. Negative bytes wrap (-1 = 255).
"""
import re
import sys

ORIGIN = 0x200


class AsmError(Exception):
    pass


def register(token):
    m = re.fullmatch(r'[vV]([0-9a-fA-F])', token.strip())
    if not m:
        raise AsmError(f'register expected, found "{token}"')
    return int(m.group(1), 16)


def is_register(token):
    return re.fullmatch(r'[vV][0-9a-fA-F]', token.strip()) is not None


def split_line(text):
    """Returns (labels, constant, mnemonic, operands) for one source line."""
    text = text.split(';', 1)[0].strip()
    labels = []
    while True:
        m = re.match(r'([A-Za-z_][\w.]*):\s*(.*)', text)
        if not m:
            break
        labels.append(m.group(1))
        text = m.group(2)
    m = re.fullmatch(r'([A-Za-z_][\w.]*)\s*=\s*(.+)', text)
    if m:
        return labels, (m.group(1), m.group(2)), None, []
    if not text:
        return labels, None, None, []
    parts = text.split(None, 1)
    operands = [o.strip() for o in parts[1].split(',')] if len(parts) > 1 else []
    return labels, None, parts[0].upper(), operands


class Assembler:
    def __init__(self, source):
        self.lines = source.splitlines()
        self.symbols = {}

    def value(self, token):
        t = token.strip()
        if t in self.symbols:
            return self.symbols[t]
        try:
            return int(t[1:], 16) if t.startswith('#') else int(t, 0)
        except ValueError:
            raise AsmError(f'unknown value "{t}"')

    def byte(self, token):
        b = self.value(token)
        if not -128 <= b <= 255:
            raise AsmError(f'byte out of range: {token}')
        return b & 0xFF

    def address(self, token):
        a = self.value(token)
        if not 0 <= a <= 0xFFF:
            raise AsmError(f'address out of range: {token}')
        return a

    def encode(self, op, ops):
        """Returns the bytes of one instruction."""
        def word(w):
            return bytes([(w >> 8) & 0xFF, w & 0xFF])

        def need(n):
            if len(ops) != n:
                raise AsmError(f'{op} needs {n} operands')

        u = [o.upper() for o in ops]

        if op == 'DB':
            return bytes(self.byte(o) for o in ops)
        if op == 'CLS':
            return word(0x00E0)
        if op == 'RET':
            return word(0x00EE)
        if op == 'JP':
            if len(ops) == 2:
                if register(ops[0]) != 0:
                    raise AsmError('only JP V0, addr exists')
                return word(0xB000 | self.address(ops[1]))
            need(1)
            return word(0x1000 | self.address(ops[0]))
        if op == 'CALL':
            need(1)
            return word(0x2000 | self.address(ops[0]))
        if op in ('SE', 'SNE'):
            need(2)
            x = register(ops[0])
            if is_register(ops[1]):
                return word((0x5000 if op == 'SE' else 0x9000) | x << 8 |
                            register(ops[1]) << 4)
            return word((0x3000 if op == 'SE' else 0x4000) | x << 8 |
                        self.byte(ops[1]))
        if op == 'LD':
            need(2)
            special = {'DT': 0xF015, 'ST': 0xF018, 'F': 0xF029, 'B': 0xF033,
                       '[I]': 0xF055}
            if u[0] == 'I':
                return word(0xA000 | self.address(ops[1]))
            if u[0] in special:
                return word(special[u[0]] | register(ops[1]) << 8)
            x = register(ops[0])
            if u[1] == 'DT':
                return word(0xF007 | x << 8)
            if u[1] == 'K':
                return word(0xF00A | x << 8)
            if u[1] == '[I]':
                return word(0xF065 | x << 8)
            if is_register(ops[1]):
                return word(0x8000 | x << 8 | register(ops[1]) << 4)
            return word(0x6000 | x << 8 | self.byte(ops[1]))
        if op == 'ADD':
            need(2)
            if u[0] == 'I':
                return word(0xF01E | register(ops[1]) << 8)
            x = register(ops[0])
            if is_register(ops[1]):
                return word(0x8004 | x << 8 | register(ops[1]) << 4)
            return word(0x7000 | x << 8 | self.byte(ops[1]))
        if op in ('OR', 'AND', 'XOR', 'SUB', 'SUBN'):
            need(2)
            code = {'OR': 1, 'AND': 2, 'XOR': 3, 'SUB': 5, 'SUBN': 7}[op]
            return word(0x8000 | register(ops[0]) << 8 |
                        register(ops[1]) << 4 | code)
        if op in ('SHR', 'SHL'):
            # vY = vX, so it shifts vX in place on every interpreter.
            need(1)
            x = register(ops[0])
            return word(0x8000 | x << 8 | x << 4 | (6 if op == 'SHR' else 0xE))
        if op == 'RND':
            need(2)
            return word(0xC000 | register(ops[0]) << 8 | self.byte(ops[1]))
        if op == 'DRW':
            need(3)
            n = self.value(ops[2])
            if not 0 <= n <= 15:
                raise AsmError('sprite height must be 0 .. 15')
            return word(0xD000 | register(ops[0]) << 8 |
                        register(ops[1]) << 4 | n)
        if op in ('SKP', 'SKNP'):
            need(1)
            return word((0xE09E if op == 'SKP' else 0xE0A1) |
                        register(ops[0]) << 8)
        raise AsmError(f'unknown instruction {op}')

    def size(self, op, ops):
        return len(ops) if op == 'DB' else 2

    def run(self):
        # Pass 1: addresses of labels.
        address = ORIGIN
        for number, line in enumerate(self.lines, 1):
            labels, constant, op, ops = split_line(line)
            for label in labels:
                if label in self.symbols:
                    raise AsmError(f'line {number}: label "{label}" defined twice')
                self.symbols[label] = address
            if op:
                address += self.size(op, ops)

        # Constants (they may use labels).
        for number, line in enumerate(self.lines, 1):
            labels, constant, op, ops = split_line(line)
            if constant:
                try:
                    self.symbols[constant[0]] = self.value(constant[1])
                except AsmError as e:
                    raise AsmError(f'line {number}: {e}')

        # Pass 2: code.
        out = bytearray()
        for number, line in enumerate(self.lines, 1):
            labels, constant, op, ops = split_line(line)
            if op:
                try:
                    out += self.encode(op, ops)
                except AsmError as e:
                    raise AsmError(f'line {number}: {e}: {line.strip()}')
        if len(out) > 0x1000 - ORIGIN:
            raise AsmError('program too large')
        return bytes(out)


def assemble(source):
    return Assembler(source).run()


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    with open(sys.argv[1]) as f:
        code = assemble(f.read())
    with open(sys.argv[2], 'wb') as f:
        f.write(code)
    print(f'{sys.argv[2]}: {len(code)} bytes')


if __name__ == '__main__':
    try:
        main()
    except AsmError as e:
        sys.exit(f'error: {e}')
