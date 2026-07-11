# SMBus/SGPIO HID Tool

Windows MFC GUI tool and M032/M261 EVB USB HID firmware for validating the M058SSAN to M0564SSAE SGPIO/SMBus target.

The bridge EVB acts as:

- SMBus/I2C host bridge for target register validation.
- SGPIO host bridge for SFF-8485 slot-frame generation.

## Quick Start

1. Build and flash the M032 or M261 bridge firmware:
   - M031/M032: `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\Template\Keil\Template.uvprojx`
   - M261: `demo_code\M261BSP_USB_HID_SGPIO\SampleCode\Template\Keil\Template.uvprojx`
2. Connect bridge USB to the PC.
3. Wire selected bridge I2C SDA/SCL/GND to the target SMBus.
4. Wire bridge SGPIO pins to one target SGPIO channel:
   - `PA2` to `SCLK`
   - `PA0` to `SDOUT`
   - `PA3` to `SLOAD`
   - `GND` to target ground
5. Launch `build\SmbusSgpioTool.exe`.
6. Scan/connect VID `0x0416`, PID `0x5020`.
7. Click `Get Info`; expected bridge name is `m032-smbus-sgpio-bridge/1.0.0` or `m261-smbus-sgpio-bridge/1.0.0`.
8. Open `Target Test`, select I2C pins/speed/address, load or edit the sequence INI, then click `Run Sequence`.

## GUI Tabs

- `Target Test`: row-based SMBus/SGPIO validation sequence for the M0564 SGPIO firmware.
- `SGPIO Calc`: checkbox-based ACT/LOCATE/FAIL mask calculator for SGPIO sequence rows.
- `SGPIO Manual`: inherited SGPIO waveform control for manual slot mask testing.

The validation sequence is documented in:

- `docs\SGPIO_SMBUS_TEST_SEQUENCE.md`

Default sequence preset:

- `build\sequence_presets\Target_Default.ini`

## Target Test Flow

The default INI runs this order:

1. Initialize M032 I2C master.
2. Recover/check bus idle.
3. Scan SMBus addresses `0x60` through `0x67`.
4. Read `0x30` hardware config.
5. Read identity registers `0x60` through `0x64`.
6. Read/write `0x3C` manual override.
7. Read/write `0x42`, `0x44`, and `0x46` LED override masks.
8. Run lamp tests `0x66` and `0x67`.
9. Return `0x3C` to `0x00` SGPIO control.
10. Drive an SGPIO pattern.
11. Read `0x00` through `0x07` and compare low three SGPIO status bits.

Presence bit 4 is shown but not used as a fail condition because it depends on target ADC/HDD hardware state.

The sequence table can be edited in the GUI:

- `+` / `-` adds or removes command rows.
- `Kind` selects `Scan`, `Read`, `Write`, `WriteRead`, `SGPIO`, or `Delay`.
- `Reg`, `Data`, `Read Len`, `Expect`, `Mask`, and `Delay` map directly to the sequence INI fields.
- `Load`, `Save`, and `Default` manage the INI path shown at the top of the tab.
- `Loop` repeats the whole sequence until `Stop Loop` is pressed.
- `Loop Interval(ms)` is the delay after one full sequence finishes and before the next round starts.

## SGPIO Calculator

`SGPIO Calc` avoids manual hex calculation for SGPIO rows:

- `Drive Count` selects how many drive/slot bits are active, from 1 through 16.
- Check `ACT`, `LOCATE`, and `FAIL` for each drive.
- The tab updates `ACT`, `LOCATE`, `FAIL`, and complete `SGPIO Row Data` immediately.
- `Copy Row` copies text like `ACT=0x0001 LOCATE=0x000A FAIL=0x000C SLOAD=0x0` for pasting into a sequence row.
- `Default Pattern` restores the default validation frame: drive 0 ACT, drive 1 LOCATE, drive 2 FAIL, drive 3 LOCATE+FAIL.

## Address Defaults

| Position | 7-bit address | IPMI / 8-bit write address | Expected `0x30` |
| --- | ---: | ---: | ---: |
| MCU1 | `0x64` | `0xC8` | `0x24` |
| MCU2 | `0x65` | `0xCA` | `0x25` |

## Build PC Tool

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Output:

- `build\SmbusSgpioTool.exe`

## Build Firmware

Keil projects:

- M031/M032 HID IAP bootloader: `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\ISP_HID\Keil\ISP_HID.uvprojx`
- M031/M032 combined SMBus/SGPIO bridge application: `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\Template\Keil\Template.uvprojx`
- M261 HID IAP bootloader: `demo_code\M261BSP_USB_HID_SGPIO\SampleCode\ISP_HID\Keil\ISP_HID.uvprojx`
- M261 combined SMBus/SGPIO bridge application: `demo_code\M261BSP_USB_HID_SGPIO\SampleCode\Template\Keil\Template.uvprojx`

The firmware combines:

- I2C master bridge commands from `simple_pmbus_smbus_tool`.
- SGPIO waveform commands from `simple_sgpio_test_tool`.
- 64-byte HID reports with `0xA5` magic and 6-byte bridge header.

M031/M032 IAP layout:

- Bootloader: `0x00000000..0x00003FFF` (`16 KB`)
- Combined bridge application: `0x00004000..0x0001FFFF` (`112 KB`)
- App CRC word: `0x0001FFFC`
- Bootloader USB HID PID: `0x3F00`
- Application USB HID PID: `0x5020`

M261/M263KIAAE IAP layout:

- Bootloader: `0x00000000..0x0000FFFF` (`64 KB`)
- Combined bridge application: `0x00010000..0x0007FFFF` (`448 KB`)
- App CRC word: `0x0007FFFC`
- Bootloader USB HID PID: `0x3F00`
- Application USB HID PID: `0x5020`

Build `ISP_HID.uvprojx` first when preparing a board for HID app update. Build `Template.uvprojx` next; its post-build step generates the padded CRC app image at `SampleCode\Template\Keil\obj\template.bin`. Use the sibling `simple_programming_tool` to program that padded app image while the board is already in HID ISP bootloader mode.

The top-level GUI includes `Return Bootloader`. It sends `BRIDGE_CMD_ENTER_IAP` (`0x04`) to the running bridge application, erases only the application CRC word, disconnects from the app HID device, and resets through the bootloader path. After the bootloader enumerates as `VID 0x0416`, `PID 0x3F00`, use `simple_programming_tool` to program a valid padded bridge application image. This button is an app-side bridge self-update entry point; it is not a target-board programming flow.

M261 support is source-level aligned with the M032 combined bridge behavior. It uses the M261 I2C backend from the PMBus/SMBus reference plus the M261 SGPIO Timer0/PDMA backend. Hardware validation on an M261 EVB still needs to be run before treating it as production-verified.

## Project Structure

- `src\` - MFC GUI, HID transport, Target test tab, SGPIO calculator tab, SGPIO manual tab.
- `demo_code\` - M032 EVB combined HID bridge firmware.
- `docs\` - validation sequence and UI notes.
- `scripts\` - PC build/clean helpers.
- `build\` - generated PC output.

## Verification

Current local verification:

- PC build: PASS, `0 Error(s), 0 Warning(s)`.
- M032 Keil firmware build: PASS, `0 Error(s), 0 Warning(s)`.
- Hardware target validation: not run in this workspace session.
