#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bridge_commands.h"
#include "../hid/hid_bridge_client.h"
#include "../hid/hid_scan.h"

namespace mfc_tool::core {

class BridgeService {
public:
    BridgeService() = default;

    std::vector<mfc_tool::hid::DeviceInfo> ScanDevices(std::uint16_t vid, std::uint16_t pid) const;

    void Connect(std::uint16_t vid, std::uint16_t pid, const std::wstring& path, int timeout_ms);
    void Disconnect();
    [[nodiscard]] bool IsConnected() const noexcept;

    std::vector<std::uint8_t> Ping(const std::vector<std::uint8_t>& data);
    std::vector<std::uint8_t> GetInfo();
    std::vector<std::uint8_t> ResetMcu();
    std::vector<std::uint8_t> EnterIap();

    std::vector<std::uint8_t> I2cMasterInit(int port, int sda_pin, int scl_pin, int baud);
    std::vector<std::uint8_t> I2cMasterBusStatus(int port, bool recover_if_needed);
    std::vector<std::uint8_t> I2cMasterWrite(int port, int addr7, const std::vector<std::uint8_t>& data);
    std::vector<std::uint8_t> I2cMasterWriteThenReadRaw(int port, int addr7, bool repeated_start,
                                                        const std::vector<std::uint8_t>& write_data, int read_length);
    std::vector<std::uint8_t> I2cDeinit(int port);

    std::vector<std::uint8_t> SgpioConfig(int slot_count, int clock_hz);
    std::vector<std::uint8_t> SgpioApply(bool enable, bool periodic, int interval_ms, int sload_raw,
                                         int act_mask, int locate_mask, int fail_mask);
    std::vector<std::uint8_t> SgpioStatus();
    std::vector<std::uint8_t> SgpioOff();

private:
    std::vector<std::uint8_t> Tx(std::uint8_t cmd, const std::vector<std::uint8_t>& payload);

private:
    mfc_tool::hid::HidBridgeClient client_;
};

} // namespace mfc_tool::core
