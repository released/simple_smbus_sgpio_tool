#include "bridge_service.h"

#include <algorithm>

namespace mfc_tool::core {
namespace {

constexpr std::size_t kI2cShortWriteMax = 53u;
constexpr int kI2cShortReadMax = 57;

void AppendLe16(std::vector<std::uint8_t>* out, std::uint16_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void AppendLe32(std::vector<std::uint8_t>* out, std::uint32_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

} // namespace

std::vector<mfc_tool::hid::DeviceInfo> BridgeService::ScanDevices(std::uint16_t vid, std::uint16_t pid) const {
    return mfc_tool::hid::EnumerateHidDevices(vid, pid, true);
}

void BridgeService::Connect(std::uint16_t vid, std::uint16_t pid, const std::wstring& path, int timeout_ms) {
    client_.Connect(vid, pid, path, timeout_ms);
}

void BridgeService::Disconnect() {
    client_.Disconnect();
}

bool BridgeService::IsConnected() const noexcept {
    return client_.IsConnected();
}

std::vector<std::uint8_t> BridgeService::Tx(std::uint8_t cmd, const std::vector<std::uint8_t>& payload) {
    return client_.Transact(cmd, payload);
}

std::vector<std::uint8_t> BridgeService::Ping(const std::vector<std::uint8_t>& data) {
    return Tx(CMD_PING, data);
}

std::vector<std::uint8_t> BridgeService::GetInfo() {
    return Tx(CMD_GET_INFO, {});
}

std::vector<std::uint8_t> BridgeService::ResetMcu() {
    return Tx(CMD_RESET_MCU, {});
}

std::vector<std::uint8_t> BridgeService::EnterIap() {
    return Tx(CMD_ENTER_IAP, {});
}

std::vector<std::uint8_t> BridgeService::I2cMasterInit(int port, int sda_pin, int scl_pin, int baud) {
    std::vector<std::uint8_t> p;
    p.push_back(static_cast<std::uint8_t>(port));
    p.push_back(static_cast<std::uint8_t>(sda_pin));
    p.push_back(static_cast<std::uint8_t>(scl_pin));
    AppendLe32(&p, static_cast<std::uint32_t>(baud));
    return Tx(CMD_I2C_INIT_MASTER, p);
}

std::vector<std::uint8_t> BridgeService::I2cMasterBusStatus(int port, bool recover_if_needed) {
    return Tx(CMD_I2C_MASTER_BUS_STATUS, {
        static_cast<std::uint8_t>(port),
        recover_if_needed ? 1u : 0u
    });
}

std::vector<std::uint8_t> BridgeService::I2cMasterWrite(int port, int addr7, const std::vector<std::uint8_t>& data) {
    if (data.size() > kI2cShortWriteMax) {
        throw mfc_tool::hid::BridgeException("I2C write payload exceeds short bridge command capacity");
    }

    std::vector<std::uint8_t> p;
    p.push_back(static_cast<std::uint8_t>(port));
    p.push_back(static_cast<std::uint8_t>(addr7 & 0x7F));
    p.push_back(static_cast<std::uint8_t>(data.size()));
    p.insert(p.end(), data.begin(), data.end());
    return Tx(CMD_I2C_MASTER_WRITE, p);
}

std::vector<std::uint8_t> BridgeService::I2cMasterWriteThenReadRaw(int port, int addr7, bool repeated_start,
                                                                   const std::vector<std::uint8_t>& write_data,
                                                                   int read_length) {
    if (write_data.size() > kI2cShortWriteMax) {
        throw mfc_tool::hid::BridgeException("I2C write-read payload exceeds short bridge command capacity");
    }
    if (read_length > kI2cShortReadMax) {
        throw mfc_tool::hid::BridgeException("I2C write-read length exceeds short bridge command capacity");
    }

    std::vector<std::uint8_t> p;
    std::vector<std::uint8_t> rx;
    int count = 0;

    p.push_back(static_cast<std::uint8_t>(port));
    p.push_back(static_cast<std::uint8_t>(addr7 & 0x7F));
    p.push_back(repeated_start ? 1u : 0u);
    p.push_back(static_cast<std::uint8_t>(write_data.size()));
    p.push_back(static_cast<std::uint8_t>(std::clamp(read_length, 0, kI2cShortReadMax)));
    p.insert(p.end(), write_data.begin(), write_data.end());

    rx = Tx(CMD_I2C_MASTER_WRITE_READ, p);
    count = rx.empty() ? 0 : static_cast<int>(rx[0]);
    if (count <= 0) {
        return {};
    }
    return std::vector<std::uint8_t>(rx.begin() + 1,
                                     rx.begin() + 1 + (std::min)(static_cast<std::size_t>(count),
                                                                  rx.size() > 1 ? rx.size() - 1 : static_cast<std::size_t>(0)));
}

std::vector<std::uint8_t> BridgeService::I2cDeinit(int port) {
    return Tx(CMD_I2C_DEINIT, {static_cast<std::uint8_t>(port)});
}

std::vector<std::uint8_t> BridgeService::SgpioConfig(int slot_count, int clock_hz) {
    std::vector<std::uint8_t> p;
    p.push_back(static_cast<std::uint8_t>(std::clamp(slot_count, 1, 16)));
    AppendLe32(&p, static_cast<std::uint32_t>(std::clamp(clock_hz, 100000, 400000)));
    return Tx(CMD_SGPIO_CONFIG, p);
}

std::vector<std::uint8_t> BridgeService::SgpioApply(bool enable, bool periodic, int interval_ms, int sload_raw,
                                                    int act_mask, int locate_mask, int fail_mask) {
    std::vector<std::uint8_t> p;
    p.push_back(enable ? 1u : 0u);
    p.push_back(periodic ? 1u : 0u);
    AppendLe16(&p, static_cast<std::uint16_t>(std::clamp(interval_ms, 0, 0xFFFF)));
    p.push_back(static_cast<std::uint8_t>(sload_raw & 0x0F));
    AppendLe16(&p, static_cast<std::uint16_t>(act_mask & 0xFFFF));
    AppendLe16(&p, static_cast<std::uint16_t>(locate_mask & 0xFFFF));
    AppendLe16(&p, static_cast<std::uint16_t>(fail_mask & 0xFFFF));
    return Tx(CMD_SGPIO_APPLY, p);
}

std::vector<std::uint8_t> BridgeService::SgpioStatus() {
    return Tx(CMD_SGPIO_STATUS, {});
}

std::vector<std::uint8_t> BridgeService::SgpioOff() {
    return Tx(CMD_SGPIO_OFF, {});
}

} // namespace mfc_tool::core
