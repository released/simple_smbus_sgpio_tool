# AGENTS.md

## Project Scope

This workspace is the SMBus/SGPIO HID validation tool.

- PC GUI: MFC HID tool based on the sibling `simple_sgpio_test_tool` structure.
- SMBus/I2C bridge behavior: from `simple_pmbus_smbus_tool`.
- SGPIO bridge behavior: from `simple_sgpio_test_tool`.
- Bridge board: M032 EVB running the combined HID firmware in `demo_code`.

## Local Rules

- Keep the validation sequence aligned with `docs\SGPIO_SMBUS_TEST_SEQUENCE.md`.
- Keep the HID bridge report format stable: 64-byte report, `0xA5` magic, 6-byte header.
- Keep M032 SGPIO fixed pin mapping stable:
  - `PA2`: SCLK
  - `PA0`: SDOUT
  - `PA3`: SLOAD
  - `PA1`: SDIN optional
- Prefer adding focused validation behavior over recreating the full PMBus/SMBus generic tool.
- Firmware C code should remain C90-friendly:
  - declare variables at the beginning of blocks
  - use English comments
  - avoid unrelated BSP refactors

## Verification Expectations

- PC GUI changes: build `SmbusSgpioTool.sln` with `scripts\build_mfc.ps1`.
- Firmware changes: build the Keil project under `demo_code\M031BSP_USB_HID_SGPIO\SampleCode\Template\Keil`.
- Hardware validation requires an M032 EVB and the M0564 target board.
