#include "app_state.h"

#include <cwctype>

namespace mfc_tool::core {
namespace {

bool IsAbsolutePath(const std::wstring& path) {
    if (path.size() >= 2u && path[1] == L':') {
        return true;
    }
    return path.size() >= 2u &&
           ((path[0] == L'\\' && path[1] == L'\\') ||
            (path[0] == L'/' && path[1] == L'/'));
}

std::wstring LowerPath(std::wstring path) {
    for (wchar_t& ch : path) {
        if (ch == L'/') {
            ch = L'\\';
        } else {
            ch = static_cast<wchar_t>(towlower(ch));
        }
    }
    return path;
}

std::wstring ToExeRelativePath(const std::wstring& path) {
    if (path.empty() || !IsAbsolutePath(path)) {
        return path;
    }

    const std::wstring root = config::IniManager::DefaultIniPath(L"");
    const std::wstring lower_path = LowerPath(path);
    const std::wstring lower_root = LowerPath(root);
    if (lower_root.empty() || lower_path.find(lower_root) != 0u) {
        return path;
    }
    return path.substr(root.size());
}

std::wstring GetValue(
    const config::IniData& data,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& fallback
) {
    auto sec_it = data.find(section);
    if (sec_it == data.end()) {
        return fallback;
    }
    auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
        return fallback;
    }
    return key_it->second;
}

} // namespace

AppState AppState::Default() {
    return AppState{};
}

config::IniData AppState::ToIniData(const std::wstring& ini_path) const {
    config::IniData out;
    out[L"APP"] = {
        {L"ini_path", ToExeRelativePath(ini_path)}
    };

    out[L"UI"] = {
        {L"vid", ui.vid},
        {L"pid", ui.pid},
        {L"timeout_ms", ui.timeout_ms},
        {L"device_label", ui.device_label},
        {L"save_log_checked", ui.save_log_checked ? L"1" : L"0"},
        {L"expected_fw_version", ui.expected_fw_version},
        {L"last_seen_fw_version", ui.last_seen_fw_version}
    };

    out[L"TARGET_TEST"] = {
        {L"pin_pair", target_test.pin_pair},
        {L"baud", target_test.baud},
        {L"addr", target_test.addr},
        {L"scan_start", target_test.scan_start},
        {L"scan_end", target_test.scan_end},
        {L"slot_count", target_test.slot_count},
        {L"clock_hz", target_test.clock_hz},
        {L"interval_ms", target_test.interval_ms},
        {L"loop_enabled", target_test.loop_enabled},
        {L"loop_interval_ms", target_test.loop_interval_ms},
        {L"sequence_path", target_test.sequence_path}
    };

    out[L"SGPIO_CALC"] = {
        {L"slot_count", sgpio_calc.slot_count},
        {L"sload_raw", sgpio_calc.sload_raw},
        {L"act_mask", sgpio_calc.act_mask},
        {L"locate_mask", sgpio_calc.locate_mask},
        {L"fail_mask", sgpio_calc.fail_mask}
    };

    out[L"SGPIO"] = {
        {L"slot_count", sgpio.slot_count},
        {L"clock_hz", sgpio.clock_hz},
        {L"periodic", sgpio.periodic},
        {L"interval_ms", sgpio.interval_ms},
        {L"sload_raw", sgpio.sload_raw},
        {L"act_mask", sgpio.act_mask},
        {L"locate_mask", sgpio.locate_mask},
        {L"fail_mask", sgpio.fail_mask}
    };

    return out;
}

void AppState::ApplyIniData(const config::IniData& data) {
    ui.vid = GetValue(data, L"UI", L"vid", ui.vid);
    ui.pid = GetValue(data, L"UI", L"pid", ui.pid);
    ui.timeout_ms = GetValue(data, L"UI", L"timeout_ms", ui.timeout_ms);
    ui.device_label = GetValue(data, L"UI", L"device_label", ui.device_label);
    ui.save_log_checked = GetValue(data, L"UI", L"save_log_checked", ui.save_log_checked ? L"1" : L"0") == L"1";
    ui.last_seen_fw_version = GetValue(data, L"UI", L"last_seen_fw_version", ui.last_seen_fw_version);

    target_test.pin_pair = GetValue(data, L"TARGET_TEST", L"pin_pair", target_test.pin_pair);
    target_test.baud = GetValue(data, L"TARGET_TEST", L"baud", target_test.baud);
    target_test.addr = GetValue(data, L"TARGET_TEST", L"addr", target_test.addr);
    target_test.scan_start = GetValue(data, L"TARGET_TEST", L"scan_start", target_test.scan_start);
    target_test.scan_end = GetValue(data, L"TARGET_TEST", L"scan_end", target_test.scan_end);
    target_test.slot_count = GetValue(data, L"TARGET_TEST", L"slot_count", target_test.slot_count);
    target_test.clock_hz = GetValue(data, L"TARGET_TEST", L"clock_hz", target_test.clock_hz);
    target_test.interval_ms = GetValue(data, L"TARGET_TEST", L"interval_ms", target_test.interval_ms);
    target_test.loop_enabled = GetValue(data, L"TARGET_TEST", L"loop_enabled", target_test.loop_enabled);
    target_test.loop_interval_ms = GetValue(data, L"TARGET_TEST", L"loop_interval_ms", target_test.loop_interval_ms);
    target_test.sequence_path = GetValue(data, L"TARGET_TEST", L"sequence_path", target_test.sequence_path);

    sgpio_calc.slot_count = GetValue(data, L"SGPIO_CALC", L"slot_count", sgpio_calc.slot_count);
    sgpio_calc.sload_raw = GetValue(data, L"SGPIO_CALC", L"sload_raw", sgpio_calc.sload_raw);
    sgpio_calc.act_mask = GetValue(data, L"SGPIO_CALC", L"act_mask", sgpio_calc.act_mask);
    sgpio_calc.locate_mask = GetValue(data, L"SGPIO_CALC", L"locate_mask", sgpio_calc.locate_mask);
    sgpio_calc.fail_mask = GetValue(data, L"SGPIO_CALC", L"fail_mask", sgpio_calc.fail_mask);

    sgpio.slot_count = GetValue(data, L"SGPIO", L"slot_count", sgpio.slot_count);
    sgpio.clock_hz = GetValue(data, L"SGPIO", L"clock_hz", sgpio.clock_hz);
    sgpio.periodic = GetValue(data, L"SGPIO", L"periodic", sgpio.periodic);
    sgpio.interval_ms = GetValue(data, L"SGPIO", L"interval_ms", sgpio.interval_ms);
    sgpio.sload_raw = GetValue(data, L"SGPIO", L"sload_raw", GetValue(data, L"SGPIO", L"vendor_bits", sgpio.sload_raw));
    sgpio.act_mask = GetValue(data, L"SGPIO", L"act_mask", sgpio.act_mask);
    sgpio.locate_mask = GetValue(data, L"SGPIO", L"locate_mask", sgpio.locate_mask);
    sgpio.fail_mask = GetValue(data, L"SGPIO", L"fail_mask", sgpio.fail_mask);
}

} // namespace mfc_tool::core
