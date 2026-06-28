#!/bin/env python
import sys
opcodes = {
    "pad": b"\x99",
    "r1": b"\x01",
    "r2": b"\x02",
    "r3": b"\x03",
    "r4": b"\x04",
    "r5": b"\x05",
    "r6": b"\x06",
    "r7": b"\x07",
    "r8": b"\x08",
    "r9": b"\x09",
    "r10": b"\x0a",
    "r11": b"\x0b",
    "r12": b"\x0c",
    "r13": b"\x0d",
    "r14": b"\x0e",
    "r15": b"\x0f",
    "r16": b"\x10",
    "r17": b"\x11",
    "sp": b"\x13",
    "debug": b"\x67",
    "halt": b"\x00",
    "jmp": b"\x63",
    "movbr": b"\x41",
    "push": b"\xff",
    "pop": b"\xfe",
    "shr": b"\xaa",
    "shl": b"\xbb",
    "movrr": b"\x42",
    "load": b"\x59",
    "store": b"\x60",
    "cmpbr": b"\x05",
    "je": b"\x06",
    "cmprr": b"\x07",
    "and": b"\x01",
    "or": b"\x02",
    "xor": "\x03",
    "add": b"\xad",
    "sub": b"\xba",
    "mul": b"\xac",
    "div": b"\xdd",
    "inc": b"\x43",
    "dec": b"\x44",
    "not": b"\x45",
    "jl": b"\x09",
    "jg": b"\xde",
    "hostcall": b"\x69",
    "int": b"\x70"
}

def parse(file):
    byteCode = b""
    program = file.read()
    lines = program.split('\n')
    for line in lines:
        count = 0
        tokens = line.split('.')
        for token in tokens:
            opc = opcodes.get(token)
            if opc != None:
                byteCode += opc
                count += 1
            elif opc == None and token.isdigit():
                num = int(token)
                byteCode += num.to_bytes(8, byteorder='big')
                count += len(num.to_bytes(8, byteorder='big'))
            else:
                print("invalid token " + token)
                sys.exit(1)
        if count != 10:
            byteCode += b'\x99' * (10 - count)
    return byteCode

if len(sys.argv) < 3:
    print("Usage: asmtrvm.py <input-file> <output-file>")
    sys.exit(1)

with open(sys.argv[1], "r") as file:
    byteCode = parse(file)

with open(sys.argv[2], "wb") as file:
    file.write(byteCode)
