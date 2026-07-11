# SGPIO SMBus Test Sequence

This document records the validation sequence for the M058SSAN to M0564SSAE SGPIO/SMBus target.

## Scope

- Bridge board: M032 EVB running the combined SMBus/SGPIO HID bridge firmware in `demo_code`.
- Target board: SGPIO MCU board with M0564SSAE APROM firmware.
- SMBus role: M032 EVB acts as SMBus/I2C host.
- SGPIO role: M032 EVB drives one SGPIO input pair at a time.
- SGPIO reference: SFF-8485 slot stream, with three bits per slot.
- LED behavior reference: SFF-8489 / IBPI style Activity, Locate, Fail, and Rebuild indications.

## Wiring

SMBus/I2C:

- M032 selected SDA to target SMBus SDA.
- M032 selected SCL to target SMBus SCL.
- M032 GND to target GND.
- Use compatible pull-up voltage for both boards.

SGPIO:

- M032 PA2 to target SCLK input.
- M032 PA0 to target SDOUT input.
- M032 PA3 to target SLOAD input.
- M032 GND to target GND.
- Wire to either SGPIO0 or SGPIO1 on the target. The current bridge drives one SGPIO channel at a time.

## Address Rules

The target firmware selects its SMBus address from hardware straps.

| Board position | 7-bit address | IPMI / 8-bit write address | Expected H/W config |
| --- | ---: | ---: | ---: |
| MCU1 | `0x64` | `0xC8` | `0x24` |
| MCU2 | `0x65` | `0xCA` | `0x25` |

The GUI scans `0x60` through `0x67` first, then runs the rest of the flow against the selected or discovered address.

## Sequence INI Format

The `Target Test` GUI executes a row-based sequence INI. The default preset is:

- `build\sequence_presets\Target_Default.ini`

Each row is stored as one `STEP_xxx` section:

```ini
[SEQUENCE]
name=SGPIO_SMBUS_Default
count=29

[STEP_000]
kind=Scan
reg=
data=
read_len=0
expect=
mask=
delay_ms=20
comment=scan 0x60..0x67 using read 0x30
```

Row fields:

| Field | Meaning |
| --- | --- |
| `kind` | `Scan`, `Read`, `Write`, `WriteRead`, `SGPIO`, or `Delay`. |
| `reg` | SMBus register/command byte for `Read`, `Write`, and `WriteRead`. |
| `data` | Write payload bytes for SMBus rows. For `SGPIO`, key/value data such as `ACT=0x0001 LOCATE=0x000A FAIL=0x000C SLOAD=0x0`. |
| `read_len` | Number of bytes to read for `Read` and `WriteRead`. |
| `expect` | Expected readback bytes. Empty means only transaction success is checked. |
| `mask` | Optional comparison mask. Example: `expect=01`, `mask=07` checks only low three bits. |
| `delay_ms` | Delay after write or SGPIO apply. |
| `comment` | User-facing note shown in the GUI result. |

SMBus byte fields use hex notation by default. For example, `10`, `0x10`, and `10h` all mean `0x10`.

The GUI row editor uses the same fields. Use `+` and `-` to add or remove sequence rows, and `Update` to write the editor values back to the selected row.

`Run Sequence` executes the full sequence once. When `Loop` is checked, the same button starts a repeated run and changes to `Stop Loop`; `Loop Interval(ms)` is the wait time between completed rounds.

## SGPIO Row Calculator

Use the `SGPIO Calc` tab when building or changing an `SGPIO` row:

1. Set `Drive Count` to the number of drives/slots to encode.
2. Check each drive's `ACT`, `LOCATE`, and `FAIL` state.
3. Set `SLOAD L0..L3` when the SGPIO frame needs non-zero SLOAD bits.
4. Copy the generated `SGPIO Row Data` into the sequence row's `data` field.

The generated data uses this format:

```ini
ACT=0x0001 LOCATE=0x000A FAIL=0x000C SLOAD=0x0
```

## Command Sequence

1. Initialize the M032 I2C master with selected port, pin pair, and speed.
2. Recover the bus if the bridge reports a non-idle condition.
3. Scan addresses `0x60` through `0x67` by reading register `0x30`.
4. Read register `0x30` and decode:
   - bit 5: `Addr3`
   - bit 4: `OEM_Mode`
   - bit 3..0: `CONF[3:0]`
5. Read identity registers:
   - `0x60`: FW version, expected `0x07`
   - `0x61`: FW major, expected `0x0F`
   - `0x62`: FW minor, expected `0x0F`
   - `0x63`: chip ID, expected `0x43`
   - `0x64`: chip revision, expected `0x01`
6. Read `0x3C`; default is expected to be `0x00`, meaning SGPIO controls the LEDs.
7. Write `0x3C = 0xFF`; read `0x3C` back and expect `0xFF`.
8. In manual mode, write and read back the override masks:
   - `0x42 = 0x01` for Locate slot 0
   - `0x44 = 0x02` for Fail slot 1
   - `0x46 = 0x04` for Rebuild slot 2
9. Clear manual masks:
   - `0x42 = 0x00`
   - `0x44 = 0x00`
   - `0x46 = 0x00`
10. Run lamp test:
   - `0x66 = 0xFF`, then `0x66 = 0x00`
   - `0x67 = 0xFF`, then `0x67 = 0x00`
11. Write `0x3C = 0x00` to return control to SGPIO.
12. Drive an SGPIO frame:
   - slot 0 Activity bit set
   - slot 1 Locate bit set
   - slot 2 Fail bit set
   - slot 3 Locate and Fail bits set, representing Rebuild in the target decode
13. Read `0x00` through `0x07`.
14. Compare the low three bits of each slot register against the SGPIO frame:
   - slot 0 expected low bits `0x01`
   - slot 1 expected low bits `0x02`
   - slot 2 expected low bits `0x04`
   - slot 3 expected low bits `0x06`

Presence bit 4 is hardware/ADC dependent. The GUI displays it but does not fail SGPIO decode when only bit 4 differs.

## Register Map Used By The Tool

This SMBus command table is project-specific. The SGPIO frame behavior is based on SFF-8485, and LED behavior follows the SFF-8489 / IBPI style, but the SMBus register map itself is not a generic PMBus profile.

| Register | Access | Firmware behavior | GUI/default test use |
| --- | --- | --- | --- |
| `0x00`..`0x07` | R | Individual slot status. bit0 Activity, bit1 Locate, bit2 Fail, bit4 Presence/Empty. | Read after SGPIO frame; compare low three bits with `mask=07`. |
| `0x30` | R | Hardware configuration straps. bit5 `Addr3`, bit4 `OEM_Mode`, bit3..0 `CONF[3:0]`. | Address scan and hardware-config sanity check. Default compares `(value & F0h) == 20h`. |
| `0x34` | R | Legacy/code-only presence-style bitmap. | Not used by default; `0x00`..`0x07` are preferred for validation. |
| `0x38` | R | Slot mated / presence bitmap in the spec. | Not used by default because ADC/presence hardware state can vary. |
| `0x3C` | R/W | Manual override enable mask. bit n = 0 means SGPIO controls slot n; bit n = 1 means SMBus override controls slot n. | Read default `00h`, write/read `FFh`, then return to `00h`. |
| `0x42` | R/W | Locate override mask. | Write/read in manual mode. |
| `0x44` | R/W | Fail override mask. | Write/read in manual mode. |
| `0x46` | R/W | Rebuild override mask. | Write/read in manual mode. |
| `0x60` | R | Firmware version. Current sample returns `07h`. | Identity check. |
| `0x61` | R | Firmware major. Current sample returns `0Fh`. | Identity check. |
| `0x62` | R | Firmware minor. Current sample returns `0Fh`. | Identity check. |
| `0x63` | R | Chip ID. Current sample returns `43h`. | Identity check. |
| `0x64` | R | Chip revision. Current sample returns `01h`. | Identity check. |
| `0x66` | W | Lamp test L0..L7. | Write `FFh`, delay, then write `00h`. |
| `0x67` | W | Lamp test L8..L15. | Write `FFh`, delay, then write `00h`. |
| `0xA0` | R | Code-only two-byte hardware version, current code returns `16h 58h`. | Not used by default. |
| `0xA1` | R | Code falls through to `0xA2`; inherited behavior returns `00h`. | Not used by default. |
| `0xA2` | R | Code-only zero return. | Not used by default. |
| `0xA6` | W | Write `01h` to reset into LDROM ISP. | Intentionally excluded from APROM validation sequence. |

`0xA6` enters LDROM ISP and is intentionally not part of the normal APROM validation sequence.
