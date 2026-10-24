# SELF_CHECK.md

## Verified x64 EXE slimming 2026-10-25

- PASS fresh comparable Release baseline 2956800 bytes (original delivery
  2956288), formal 867328. Required delivery 2960073 -> 870601 bytes:
  GUI plus unchanged 3273-byte Target_Default.ini preset.
- Baseline SHA: 70304A688D65938E9BB4D23770D6F87D30ACC8275A5C92F9ED30477472433BB4.
- Formal SHA: FBD9E9359664B232F250463D2ED245CB5D88467B232C6AF91B6ADE3BF97B54D0.
- PASS canonical build_mfc.ps1 -Configuration Release -Platform x64
  -CloseRunningTool $false; isolated MSBuild Debug with version prebuild disabled.
  Zero reported compiler/linker warnings/errors; SDK informational defaults retained.
- PASS authorized isolated actual-EXE tests: hash-matched fresh fixtures, identical
  child-control class counts, owned common-dialog open/cancel, assertion-free Debug
  startup/close and formal exit 0. No hardware, flash or clipboard actions.
- Impact: unused x64 MFC registration only; /O2/static runtime/security PE 0x8160,
  Win32 definitions, protocol/algorithm/firmware sources retained. Measure startup/control/UI paths, not bus throughput.
- PASS paired gate: baseline warmups/calibration, five alternating AB/BA pairs,
  no concurrent compilation. Before-candidate delta max(floor, 10% baseline median,
  3*MAD); memory 5%; floors startup 25ms/UI 2ms/CPU 16ms/memory 1MiB.
  All medians remain within baseline-derived observed variability:
  - StartupMs: 603.1283 -> 547.1072, limit 658.1283.
  - PeakBytes: 28561408.0000 -> 28086272.0000, limit 29988250.0000.
  - CpuMs: 500.0000 -> 484.3750, limit 547.0000.
  - WorkflowMs: 12.6019 -> 13.3588, limit 21.6019.
- Raw samples/limits: docs/verification/exe-slimming-20261025.json.
- PASS actual clean_mfc.bat: 32 compiler files removed, 3 protected actual
  files SHA unchanged; 17 nested fixture files/source, help/idempotence PASS.
  Unchanged earlier junction-rejection evidence is reused, not rerun. Personal
  root INI hashes match the pre-build manifest without inspecting private values.
- NOT RUN: physical devices/protocol, unrelated firmware, visual/DPI, clean-PC,
  Win32 builds. No new driver/runtime, feature removal or global /O1 preset.

## Layout / Clean Audit 2026-10-20

- Existing source layout already conforms; no application/protocol/firmware restructuring or dependencies were introduced.
- Replaced broad directory deletion with standalone `scripts/clean_intermediates.ps1`; batch compatibility remains.
- Build: PASS: scripts/build_mfc.ps1 -Configuration Release -Platform x64 -CloseRunningTool $false; 0 warnings/errors; normal generated version 1.0.0.020.
- Runtime: PASS: isolated captioned main frame/startup/normal close, exit 0; no connection/write.
- PASS: shared verifier `-CleanBatch scripts/clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection`; 17 protected fixture build files plus source retained, object fixture removed, repeat-clean and pre-deletion junction rejection PASS. 3 actual protected build files unchanged (SHA-256).
- The initial expanded audit classified MSBuild `obj/*.recipe` compiler metadata as protected and reported FAIL; the classification was corrected against Microsoft.CppBuild.targets (DesktopDeployRecipeFile), then the audit passed. No delivery EXE was deleted.
- NOT RUN: physical hardware/firmware workflows, complete GUI/DPI, new Debug acceptance. Publication only updates already tracked deliverables; private INIs/data/logs and newly built test EXEs are excluded.
- Final commit/push state: workspace `#READ_ME_FIRST#/PROJECT_LAYOUT_MIGRATION.md`.

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
