#pragma once

#include <string>

#include "../config/ini_manager.h"
#include "fw_version.generated.h"

namespace mfc_tool::core {

struct UiState {
    std::wstring vid = L"0x0416";
    std::wstring pid = L"0x5020";
    std::wstring timeout_ms = L"2000";
    std::wstring device_label = L"Auto Select";
    bool save_log_checked = false;
    std::wstring expected_fw_version = M032_EXPECTED_FW_VERSION;
    std::wstring last_seen_fw_version = L"-";
};

struct SgpioState {
    std::wstring slot_count = L"8";
    std::wstring clock_hz = L"100000";
    std::wstring periodic = L"1";
    std::wstring interval_ms = L"100";
    std::wstring sload_raw = L"0x0";
    std::wstring act_mask = L"0x0001";
    std::wstring locate_mask = L"0x0000";
    std::wstring fail_mask = L"0x0000";
};

struct SgpioCalcState {
    std::wstring slot_count = L"8";
    std::wstring sload_raw = L"0x0";
    std::wstring act_mask = L"0x0001";
    std::wstring locate_mask = L"0x000A";
    std::wstring fail_mask = L"0x000C";
};

struct TargetTestState {
    std::wstring pin_pair = L"I2C0_PB4_PB5";
    std::wstring baud = L"100000";
    std::wstring addr = L"0x64";
    std::wstring scan_start = L"0x60";
    std::wstring scan_end = L"0x67";
    std::wstring slot_count = L"8";
    std::wstring clock_hz = L"100000";
    std::wstring interval_ms = L"100";
    std::wstring loop_enabled = L"0";
    std::wstring loop_interval_ms = L"1000";
    std::wstring sequence_path;
};

struct AppState {
    UiState ui;
    TargetTestState target_test;
    SgpioCalcState sgpio_calc;
    SgpioState sgpio;

    static AppState Default();

    config::IniData ToIniData(const std::wstring& ini_path) const;
    void ApplyIniData(const config::IniData& data);
};

} // namespace mfc_tool::core
