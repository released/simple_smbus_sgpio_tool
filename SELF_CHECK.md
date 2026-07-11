# SELF_CHECK.md

## 2026-07-09

Scope checked:

- Created SMBus/SGPIO GUI tool in `simple_smbus_sgpio_tool`.
- Added `docs\SGPIO_SMBUS_TEST_SEQUENCE.md`.
- Added validation tab with:
  - I2C init
  - address scan
  - editable INI command sequence rows
  - optional sequence loop with loop interval
  - `0x30` read/decode
  - `0x60` through `0x64` identity checks
  - `0x3C` manual override checks
  - `0x42`, `0x44`, `0x46` override mask checks
  - `0x66`, `0x67` lamp test writes
  - SGPIO frame drive
  - `0x00` through `0x07` slot readback table
- Added default sequence preset:
  - `build\sequence_presets\Target_Default.ini`
- Added `SGPIO Calc` tab for checkbox-based ACT/LOCATE/FAIL row-data calculation.
- Replaced application icon with uppercase `SS` artwork fitted inside the icon frame.
- Documented command table and INI sequence format in:
  - `docs\SGPIO_SMBUS_TEST_SEQUENCE.md`
- Preserved manual SGPIO tab.
- Combined M032 demo firmware I2C bridge and SGPIO bridge commands.

## Verification

PC GUI build:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_mfc.ps1 -Configuration Release -Platform x64
```

Result:

- PASS
- Output: `build\SmbusSgpioTool.exe`
- MSBuild: `0 warning(s), 0 error(s)`
- Latest checked build bumped the PC tool version to `1.0.0.018`.

M032 firmware build:

```bat
C:\Keil_v5\UV4\UV4.exe -b Template.uvprojx -j0 -o build_smbus_sgpio_bridge.log
```

Run directory:

- `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\Template\Keil`

Result:

- PASS
- Log: `build_smbus_sgpio_bridge.log`
- Keil: `0 Error(s), 0 Warning(s)`
- Program size: `Code=15780 RO-data=804 RW-data=180 ZI-data=4792`

Hardware validation:

- NOT RUN
- Requires flashed M032 EVB and M0564 target board wiring.
