# SPI-I2C Chip Reader for the Flipper Zero

A Flipper Zero app that reads serial memory over the Flipper's external GPIO and saves the
whole chip to the SD card as a raw `.bin`. 

Tested with the following chips:
- Microchip EEPROM 25AA32A - 32 Kbit (4 KB) - SPI
- NOR flash SST25VF040B — 4000 Kbit (500 KB) - SPI
- Microchip 24LC16B — 16 Kbit (2 KB) - I2C

## Reading 25AA32A / SST25VF040B (SOIC-8)

Standard Microchip 25-series pinout.
**landscape, dimple at bottom-left**:

```
   top edge:   8 VCC    7 HOLD#   6 SCK    5 SI
               │        │         │        │
             ┌─┴────────┴─────────┴────────┴─┐
             │       25AA32A (4 KB SPI)      │
             │ •                             │
             └─┬────────┬─────────┬────────┬─┘
               │        │         │        │
   bot edge:   1 CS#    2 SO      3 WP#    4 VSS
```

| pin | name | role |
|----:|------|------|
| 1 | CS#  | chip select (active low) |
| 2 | SO   | data out (MISO) |
| 3 | WP#  | write-protect — (connect to VCC to read) |
| 4 | VSS  | ground |
| 5 | SI   | data in (MOSI) |
| 6 | SCK  | clock |
| 7 | HOLD#| (connect to VCC to read) |
| 8 | VCC  | 1.8–5.5 V |

## Wiring (Flipper external GPIO)

SPI mode 0. Pick the table that matches your setup, either powered by the Flipper Zero or self powered.

### A — Isolated chip (desoldered, Flipper powers chip)

| 25AA32A pin | name | connect to |
|------------:|------|------------|
| 1 | CS#  | Flipper **pin 4** (CS) |
| 2 | SO (MISO) | Flipper **pin 3** (MISO) |
| 3 | WP#  | Flipper **pin 9** (3V3) |
| 4 | VSS  | Flipper **pin 8** (GND) |
| 5 | SI (MOSI) | Flipper **pin 2** (MOSI) |
| 6 | SCK  | Flipper **pin 5** (SCK) |
| 7 | HOLD# | Flipper **pin 9** (3V3) |
| 8 | VCC  | Flipper **pin 9** (3V3) |

### B — In-circuit, board powers the chip

**Do not connect Flipper 3V3 to the chip** Share only ground and the 4 SPI lines.

Depending on setup, you may need to hold the board's host MCU in reset (e.g. tie its `MCLR`/`reset` pin to
GND so it stops driving the shared SPI bus. Then proceed to wire the 
chip the the Flipper Zero GPIOs.

| 25AA32A pin | name | connect to |
|------------:|------|------------|
| 1 | CS#  | Flipper **pin 4** (CS) |
| 2 | SO (MISO) | Flipper **pin 3** (MISO) |
| 3 | WP#  | tie to **VCC / pin 8**  |
| 4 | VSS  | Flipper **pin 8** (GND) |
| 5 | SI (MOSI) | Flipper **pin 2** (MOSI) |
| 6 | SCK  | Flipper **pin 5** (SCK) |
| 7 | HOLD# | tie to **VCC / pin 8**  |
| 8 | VCC  | DO NOT connect Flipper 3.3V |


## [UNTESTED] Reading 24LC16B (SOIC-8, I2C)

The I2C functionality has not been tested as didn't have any 4.7Ω resistors on hand.

The 24LC16B is a 16 Kbit / 2048-byte I2C EEPROM, organized as 8 blocks of 256 bytes.
It sits on a different bus from the SPI parts, the Flipper's external I2C (pins 15/16)
and uses the standard 24-series I2C SOIC-8 pinout.

```
   top edge:   8 VCC     7 WP      6 SCL     5 SDA
               │         │         │         │
             ┌─┴─────────┴─────────┴─────────┴─┐
             │       24LC16B (2 KB I2C)        │
             │ •                               │
             └─┬─────────┬─────────┬─────────┬─┘
               │         │         │         │
   bot edge:   1 A0      2 A1      3 A2      4 VSS
```

### Wiring

I2C, 100 kHz. The 24LC16B can be powered from the Flipper (isolated) or by its own board
in-circuit, same host-reset caveat as the SPI parts where you need to hold the host MCU in reset so it
releases the bus. DO NOT connect Flipper 3V3 into a board-powered chip.

| 24LC16B pin | name | connect to |
|------------:|------|------------|
| 1 | A0   | GND |
| 2 | A1   | GND |
| 3 | A2   | GND |
| 4 | VSS  | Flipper **pin 8** (GND) |
| 5 | SDA  | Flipper **pin 15** (SDA) AND pull up resistor 4.7 kΩ to **pin 9** |
| 6 | SCL  | Flipper **pin 16** (SCL) AND pull up resistor 4.7 kΩ to **pin 9** |
| 7 | WP   | VCC / 3V3 (write-protected) |
| 8 | VCC  | DO NOT connect to Flipper 3V3 if chip is powered by the board |


Check wiring option shows `WIRING: OK` once any of `0x50–0x57`
answers. 

## Build & install

Requires [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt)
(`pipx install ufbt`).

```sh
ufbt              # build -> dist/spi_i2c_chip_reader.fap
ufbt launch       # build, install to /ext/apps/GPIO/, and run (quit qFlipper first —
                  # it holds the serial port)
```

## Project commentary
I built this with Claude Opus 4.8, after the [picflipper](https://github.com/picflipper) project. This went much smoother now that I built one Flipper Zero app. Claude still try to call it quits when it got stuck, but I was able to keep it moving along much easier by unblocking and pointing it at the right tools. I also learned that less information is sometimes better, at least with Opus 4.8. Less to get distracted by, and fewer rabbit holes to go down. The tendency to rabbit hole was immediately evident after 2 or so attempts at a problem, where it insisted on needing the same information I already provided, but either in a different format or just "fresh" data.


## License

[MIT](LICENSE) Copyright (c) 2026, BC (https://github.com/bciuca).

Original work, built against the Flipper Zero SDK via its public API; no third-party
code bundled.
