#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026, BC (https://github.com/bciuca).

# Generate assets/images/wiring_diagram_i2c.png — 128x64 1-bit wiring reference for the
# 24LC16B I2C EEPROM, styled to mirror the SPI wiring_diagram.png.
from PIL import Image, ImageDraw
import sys, os

W, H = 128, 64
im = Image.new("L", (W, H), 255)  # white
d = ImageDraw.Draw(im)

# ---- 3x5 pixel font ---------------------------------------------------------
F = {
 '0':["###","# #","# #","# #","###"],
 '1':[" # ","## "," # "," # ","###"],
 '2':["###","  #","###","#  ","###"],
 '3':["###","  #","###","  #","###"],
 '4':["# #","# #","###","  #","  #"],
 '5':["###","#  ","###","  #","###"],
 '6':["###","#  ","###","# #","###"],
 '7':["###","  #","  #","  #","  #"],
 '8':["###","# #","###","# #","###"],
 '9':["###","# #","###","  #","###"],
 'A':[" # ","# #","###","# #","# #"],
 'B':["## ","# #","## ","# #","## "],
 'C':["###","#  ","#  ","#  ","###"],
 'D':["## ","# #","# #","# #","## "],
 'E':["###","#  ","###","#  ","###"],
 'F':["###","#  ","###","#  ","#  "],
 'G':["###","#  ","# #","# #","###"],
 'H':["# #","# #","###","# #","# #"],
 'I':["###"," # "," # "," # ","###"],
 'K':["# #","# #","## ","# #","# #"],
 'L':["#  ","#  ","#  ","#  ","###"],
 'N':["# #","## #"[:3],"###"[:3],"# #","# #"],  # placeholder, fixed below
 'O':["###","# #","# #","# #","###"],
 'P':["## ","# #","## ","#  ","#  "],
 'R':["## ","# #","## ","# #","# #"],
 'S':["###","#  ","###","  #","###"],
 'T':["###"," # "," # "," # "," # "],
 'U':["# #","# #","# #","# #","###"],
 'V':["# #","# #","# #","# #"," # "],
 'W':["#   #","#   #","# # #","## ##","#   #"],  # 5px wide
 ':':["   "," # ","   "," # ","   "],
 '-':["   ","   ","###","   ","   "],
 '/':["  #","  #"," # ","#  ","#  "],
 '.':["   ","   ","   ","   "," # "],
 ',':["   ","   ","   "," # ","#  "],
 '>':["#  "," # ","  #"," # ","#  "],
 ' ':["   ","   ","   ","   ","   "],
}
# proper N
F['N']=["# #","###","###","# #","# #"]

def text(x, y, s, color=0):
    cx = x
    for ch in s:
        g = F.get(ch.upper(), F[' '])
        for ry, row in enumerate(g):
            for rx, c in enumerate(row):
                if c == '#':
                    im.putpixel((cx+rx, y+ry), color)
        cx += len(g[0]) + 1  # glyph width + 1px gap
    return cx

def textw(s):
    return sum(len(F.get(ch.upper(), F[' '])[0]) + 1 for ch in s) - 1

def box(x, y, s, pad=1):
    w = textw(s) + 2*pad + 1
    h = 5 + 2*pad
    d.rectangle([x, y, x+w-1, y+h-1], fill=0)
    text(x+pad+1, y+pad, s, color=255)
    return x+w-1, y+h-1  # right, bottom

def cbox(cx, cy, s):  # box centered on (cx,cy)
    w = textw(s) + 4
    h = 7
    x = cx - w//2; y = cy - h//2
    d.rectangle([x, y, x+w-1, y+h-1], fill=0)
    text(x+2, y+1, s, color=255)
    return x, y, x+w-1, y+h-1

# ---- badge top-left (overdrawn at runtime; baked as fallback) ----------------
d.rectangle([0, 0, 42, 10], fill=0)
text(2, 3, "WIRING:OK", color=255)

# ---- top-right chip id -------------------------------------------------------
text(W-1-textw("24LC16B"), 2, "24LC16B")

# ---- chip body ---------------------------------------------------------------
CX0, CY0, CX1, CY1 = 52, 18, 76, 52
d.rectangle([CX0, CY0, CX1, CY1], outline=0)
# pin-1 dot (top-left inside)
d.rectangle([CX0+3, CY0+3, CX0+4, CY0+4], fill=0)
pin_y = [24, 31, 38, 45]
# left pin numbers 1..4, right pin numbers 8,7,6,5
for i, y in enumerate(pin_y):
    text(CX0+3, y-2, str(i+1))            # 1..4
    text(CX1-5, y-2, str(8-i))            # 8,7,6,5
    d.line([CX0-6, y, CX0, y], fill=0)    # left stub
    d.line([CX1, y, CX1+6, y], fill=0)    # right stub

# ---- left side: pins 1-4 (A0,A1,A2,Vss) -> GND -------------------------------
busx = CX0-6
for y in pin_y:
    d.line([busx, y, busx, y], fill=0)
d.line([busx, pin_y[0], busx, pin_y[-1]], fill=0)   # vertical bus
midL = (pin_y[0]+pin_y[-1])//2
d.line([26, midL, busx, midL], fill=0)              # wire to GND box
box(2, midL-3, "GND 8")                             # GND box (Flipper pin 8)
text(2, pin_y[0]-9, "A0-2,VSS")                     # caption

# ---- right side --------------------------------------------------------------
# pins 8 (Vcc) + 7 (WP) -> 3V3
jx = CX1+6+6  # join x
d.line([CX1+6, pin_y[0], jx, pin_y[0]], fill=0)
d.line([CX1+6, pin_y[1], jx, pin_y[1]], fill=0)
d.line([jx, pin_y[0], jx, pin_y[1]], fill=0)
midR = (pin_y[0]+pin_y[1])//2
d.line([jx, midR, jx+3, midR], fill=0)
text(CX1+8, pin_y[0]-9, "VCC,WP")
box(jx+3, midR-3, "9 3V3")
# pin 6 SCL -> 16
d.line([CX1+6, pin_y[2], CX1+12, pin_y[2]], fill=0)
box(CX1+12, pin_y[2]-3, "16 SCL")
# pin 5 SDA -> 15
d.line([CX1+6, pin_y[3], CX1+12, pin_y[3]], fill=0)
box(CX1+12, pin_y[3]-3, "15 SDA")

# ---- pull-up note ------------------------------------------------------------
note = "SDA,SCL: 4K7 TO 3V3"
text((W-textw(note))//2, 57, note)

out = sys.argv[1] if len(sys.argv) > 1 else "wiring_diagram_i2c.png"
im.save(out)
# also an upscaled preview
im.resize((W*6, H*6), Image.NEAREST).save(os.path.splitext(out)[0] + "_big.png")
print("wrote", out)
