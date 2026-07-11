#pragma once

#include <afxcmn.h>
#include <afxwin.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "../core/app_state.h"
#include "../core/board_i2c_catalog.h"
#include "../core/bridge_service.h"

class CTargetTestTab : public CWnd {
public:
    BOOL Create(CWnd* parent, const RECT& rect, UINT id);

    void Bind(mfc_tool::core::BridgeService* service,
              std::function<void(const std::wstring&)> logger);
    void SetConnected(bool connected);
    void OnDisconnected();
    void RefreshDpiLayout();

    void LoadState(const mfc_tool::core::AppState& state);
    void SaveState(mfc_tool::core::AppState* state) const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBtnInit();
    afx_msg void OnBtnScan();
    afx_msg void OnBtnRunAll();
    afx_msg void OnBtnDeinit();
    afx_msg void OnBtnClear();
    afx_msg void OnBtnLoadSequence();
    afx_msg void OnBtnSaveSequence();
    afx_msg void OnBtnDefaultSequence();
    afx_msg void OnBtnAddRow();
    afx_msg void OnBtnRemoveRow();
    afx_msg void OnBtnUpdateRow();
    afx_msg void OnBtnMoveUp();
    afx_msg void OnBtnMoveDown();
    afx_msg void OnCommandRowChanged(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    enum class CommandKind {
        Scan,
        Read,
        Write,
        WriteRead,
        Sgpio,
        Delay,
    };

    struct CommandRow {
        CommandKind kind = CommandKind::Read;
        std::wstring reg;
        std::wstring data;
        std::wstring read_len = L"1";
        std::wstring expect;
        std::wstring mask;
        std::wstring delay_ms = L"20";
        std::wstring comment;
    };

    void PopulatePinCombo(const std::wstring& preferred_ini_name);
    void PopulateKindCombo();
    void LayoutControls(const CRect& r);
    void UpdateEnableState();
    void SetStatusText(const std::wstring& text);
    void AddResult(const std::wstring& step, bool pass, const std::wstring& details);
    void ClearResults();
    void ClearSlots();
    void SetSlotRow(int slot, std::uint8_t value, std::uint8_t expected_low, bool checked);
    void InitI2cFromUi();
    std::vector<int> ScanAddresses(bool update_ui);
    std::vector<std::uint8_t> ReadReg(int addr7, int reg, int length);
    void WriteRegBytes(int addr7, int reg, const std::vector<std::uint8_t>& data);
    void RunSequence();
    void ScheduleNextLoop();
    void StopSequenceLoop(const std::wstring& status_text);
    int LoopIntervalMs() const;
    bool ExecuteRow(const CommandRow& row, int index, int* addr7);
    bool ExecuteReadRow(const CommandRow& row, int index, int addr7);
    bool ExecuteWriteRow(const CommandRow& row, int index, int addr7);
    bool ExecuteWriteReadRow(const CommandRow& row, int index, int addr7);
    bool ExecuteSgpioRow(const CommandRow& row, int index);
    bool CompareBytes(const std::vector<std::uint8_t>& rx,
                      const std::vector<std::uint8_t>& expected,
                      const std::vector<std::uint8_t>& mask) const;
    void RefreshCommandList();
    void LoadEditorFromSelectedRow();
    void StoreEditorToSelectedRow();
    CommandRow BuildRowFromEditor() const;
    int SelectedCommandIndex() const;
    void SelectCommandIndex(int index);
    void AddDefaultSequenceRows();
    bool LoadSequenceFromFile(const std::wstring& path, bool show_errors);
    bool SaveSequenceToFile(const std::wstring& path, bool show_errors) const;
    std::wstring DefaultSequencePath() const;
    void EnsureSequenceLoaded();
    int CurrentAddress() const;
    int ParseEditInt(const CEdit& edit) const;
    int ParseIntOrDefault(const std::wstring& text, int fallback) const;
    std::wstring GetEditText(const CEdit& edit) const;
    const mfc_tool::core::board_i2c::PinPair& CurrentPinPair() const;
    std::wstring CurrentPinPairIniName() const;
    std::vector<std::uint8_t> ParseByteList(const std::wstring& text) const;
    int SgpioValueFromData(const CommandRow& row, const std::wstring& key, int fallback) const;
    static CommandKind KindFromText(const std::wstring& text);
    static std::wstring KindText(CommandKind kind);
    static std::wstring HexByte(std::uint8_t v);
    static std::wstring HexWord(int v);
    static std::wstring HexBytes(const std::vector<std::uint8_t>& data);
    static std::wstring AnsiToWide(const char* text);

private:
    mfc_tool::core::BridgeService* service_ = nullptr;
    std::function<void(const std::wstring&)> log_;
    bool connected_ = false;
    bool i2c_enabled_ = false;
    bool loop_running_ = false;
    bool loading_editor_ = false;
    int last_scanned_addr_ = -1;
    int loop_iteration_ = 0;
    std::vector<CommandRow> commands_;

    CFont ui_font_;
    CFont list_font_;
    CButton config_group_;
    CStatic seq_path_label_;
    CEdit seq_path_edit_;
    CButton load_seq_btn_;
    CButton save_seq_btn_;
    CButton default_seq_btn_;
    CStatic pins_label_;
    CComboBox pins_combo_;
    CStatic baud_label_;
    CEdit baud_edit_;
    CStatic addr_label_;
    CEdit addr_edit_;
    CStatic scan_label_;
    CEdit scan_start_edit_;
    CStatic scan_to_label_;
    CEdit scan_end_edit_;
    CStatic sgpio_label_;
    CEdit slot_count_edit_;
    CStatic clock_label_;
    CEdit clock_edit_;
    CStatic interval_label_;
    CEdit interval_edit_;
    CButton loop_check_;
    CStatic loop_interval_label_;
    CEdit loop_interval_edit_;

    CButton command_group_;
    CListCtrl command_list_;
    CStatic kind_label_;
    CComboBox kind_combo_;
    CStatic reg_label_;
    CEdit reg_edit_;
    CStatic data_label_;
    CEdit data_edit_;
    CStatic read_len_label_;
    CEdit read_len_edit_;
    CStatic expect_label_;
    CEdit expect_edit_;
    CStatic mask_label_;
    CEdit mask_edit_;
    CStatic delay_label_;
    CEdit delay_edit_;
    CStatic comment_label_;
    CEdit comment_edit_;
    CButton add_row_btn_;
    CButton remove_row_btn_;
    CButton update_row_btn_;
    CButton move_up_btn_;
    CButton move_down_btn_;

    CButton action_group_;
    CButton init_btn_;
    CButton scan_btn_;
    CButton run_all_btn_;
    CButton deinit_btn_;
    CButton clear_btn_;
    CStatic status_label_;
    CStatic status_value_;

    CButton result_group_;
    CListCtrl result_list_;
    CButton slot_group_;
    CListCtrl slot_list_;
};
