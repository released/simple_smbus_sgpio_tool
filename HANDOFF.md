# HANDOFF.md

## Current State

`simple_smbus_sgpio_tool` now contains a dedicated SMBus/SGPIO validation GUI and a combined M032 EVB HID bridge firmware.

PC tool:

- Solution: `SmbusSgpioTool.sln`
- Project: `SmbusSgpioTool.vcxproj`
- Output: `build\SmbusSgpioTool.exe`
- Main tabs:
  - `Target Test`
  - `SGPIO Calc`
  - `SGPIO Manual`
- Default sequence INI: `build\sequence_presets\Target_Default.ini`

Firmware:

- Keil project: `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\Template\Keil\Template.uvprojx`
- Bridge info string: `m032-smbus-sgpio-bridge/1.0.0`
- Supports I2C bridge commands from the PMBus/SMBus tool and SGPIO commands from the SGPIO tool.

## Target Test Tab

The Target test tab follows this document and default INI:

- `docs\SGPIO_SMBUS_TEST_SEQUENCE.md`
- `build\sequence_presets\Target_Default.ini`

The command sequence is row-based and editable from the GUI. Each row selects `Scan`, `Read`, `Write`, `WriteRead`, `SGPIO`, or `Delay`, then stores register, data/read length, expected data, mask, delay, and comment fields in INI format.

`Run Sequence` is single-shot unless `Loop` is checked. With loop enabled, the GUI waits `Loop Interval(ms)` between completed sequence rounds and the run button changes to `Stop Loop`.

`SGPIO Calc` is a pure UI calculator for building SGPIO row data. It has a 1..16 drive checkbox matrix and produces `ACT=... LOCATE=... FAIL=... SLOAD=...` text that can be copied directly into an `SGPIO` sequence row.

Default assumptions:

- Address scan range: `0x60` through `0x67`
- MCU1 default address: `0x64`
- MCU2 default address: `0x65`
- SGPIO frame pattern:
  - ACT mask `0x0001`
  - LOCATE mask `0x000A`
  - FAIL mask `0x000C`

Slot readback compares only low three bits against SGPIO decode expectation. Presence bit 4 is displayed as information because it depends on target ADC/HDD hardware.

## Verification Done

- PC Release x64 build: PASS, `0 warning(s), 0 error(s)`.
- M032 Keil firmware build: PASS, `0 Error(s), 0 Warning(s)`.

## Not Yet Done

- Hardware smoke test with M032 EVB.
- Target-board validation against actual M0564 APROM firmware.
- Screenshot capture of the new GUI.

## Suggested Next Hardware Test

1. Flash the M032 bridge firmware.
2. Launch `build\SmbusSgpioTool.exe`.
3. Connect VID `0x0416`, PID `0x5020`.
4. Confirm `Get Info` returns `m032-smbus-sgpio-bridge/1.0.0`.
5. Wire I2C and one SGPIO channel to the target.
6. Run `Target Test` -> `Run Sequence`.
7. Compare GUI results with logic analyzer decode and target LED behavior.
