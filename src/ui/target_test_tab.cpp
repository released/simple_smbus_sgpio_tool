#include "target_test_tab.h"

#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "../config/ini_manager.h"
#include "../core/text_utils.h"
#include "layout_utils.h"

namespace {

enum : UINT {
    IDC_TARGET_CONFIG_GROUP = 14000,
    IDC_TARGET_SEQ_PATH_LABEL,
    IDC_TARGET_SEQ_PATH_EDIT,
    IDC_TARGET_LOAD_SEQ_BTN,
    IDC_TARGET_SAVE_SEQ_BTN,
    IDC_TARGET_DEFAULT_SEQ_BTN,
    IDC_TARGET_PINS_LABEL,
    IDC_TARGET_PINS_COMBO,
    IDC_TARGET_BAUD_LABEL,
    IDC_TARGET_BAUD_EDIT,
    IDC_TARGET_ADDR_LABEL,
    IDC_TARGET_ADDR_EDIT,
    IDC_TARGET_SCAN_LABEL,
    IDC_TARGET_SCAN_START_EDIT,
    IDC_TARGET_SCAN_TO_LABEL,
    IDC_TARGET_SCAN_END_EDIT,
    IDC_TARGET_SGPIO_LABEL,
    IDC_TARGET_SLOT_COUNT_EDIT,
    IDC_TARGET_CLOCK_LABEL,
    IDC_TARGET_CLOCK_EDIT,
    IDC_TARGET_INTERVAL_LABEL,
    IDC_TARGET_INTERVAL_EDIT,
    IDC_TARGET_LOOP_CHECK,
    IDC_TARGET_LOOP_INTERVAL_LABEL,
    IDC_TARGET_LOOP_INTERVAL_EDIT,
    IDC_TARGET_COMMAND_GROUP,
    IDC_TARGET_COMMAND_LIST,
    IDC_TARGET_KIND_LABEL,
    IDC_TARGET_KIND_COMBO,
    IDC_TARGET_REG_LABEL,
    IDC_TARGET_REG_EDIT,
    IDC_TARGET_DATA_LABEL,
    IDC_TARGET_DATA_EDIT,
    IDC_TARGET_READ_LEN_LABEL,
    IDC_TARGET_READ_LEN_EDIT,
    IDC_TARGET_EXPECT_LABEL,
    IDC_TARGET_EXPECT_EDIT,
    IDC_TARGET_MASK_LABEL,
    IDC_TARGET_MASK_EDIT,
    IDC_TARGET_DELAY_LABEL,
    IDC_TARGET_DELAY_EDIT,
    IDC_TARGET_COMMENT_LABEL,
    IDC_TARGET_COMMENT_EDIT,
    IDC_TARGET_ADD_ROW_BTN,
    IDC_TARGET_REMOVE_ROW_BTN,
    IDC_TARGET_UPDATE_ROW_BTN,
    IDC_TARGET_MOVE_UP_BTN,
    IDC_TARGET_MOVE_DOWN_BTN,
    IDC_TARGET_ACTION_GROUP,
    IDC_TARGET_INIT_BTN,
    IDC_TARGET_SCAN_BTN,
    IDC_TARGET_RUN_ALL_BTN,
    IDC_TARGET_DEINIT_BTN,
    IDC_TARGET_CLEAR_BTN,
    IDC_TARGET_STATUS_LABEL,
    IDC_TARGET_STATUS_VALUE,
    IDC_TARGET_RESULT_GROUP,
    IDC_TARGET_RESULT_LIST,
    IDC_TARGET_SLOT_GROUP,
    IDC_TARGET_SLOT_LIST,
};

constexpr int kSgpioActMask = 0x0001;
constexpr int kSgpioLocateMask = 0x000A;
constexpr int kSgpioFailMask = 0x000C;
constexpr UINT_PTR kSequenceLoopTimerId = 0x5451u;

std::uint8_t SlotExpectedLowBits(int slot) {
    switch (slot) {
    case 0:
        return 0x01u;
    case 1:
        return 0x02u;
    case 2:
        return 0x04u;
    case 3:
        return 0x06u;
    default:
        return 0x00u;
    }
}

std::wstring Trim(std::wstring text) {
    while (!text.empty() && iswspace(text.front())) {
        text.erase(text.begin());
    }
    while (!text.empty() && iswspace(text.back())) {
        text.pop_back();
    }
    return text;
}

std::wstring Upper(std::wstring text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(towupper(ch));
    }
    return text;
}

std::wstring GetIniValue(const mfc_tool::config::IniData& data,
                         const std::wstring& section,
                         const std::wstring& key,
                         const std::wstring& fallback) {
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

BEGIN_MESSAGE_MAP(CTargetTestTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_TARGET_INIT_BTN, &CTargetTestTab::OnBtnInit)
    ON_BN_CLICKED(IDC_TARGET_SCAN_BTN, &CTargetTestTab::OnBtnScan)
    ON_BN_CLICKED(IDC_TARGET_RUN_ALL_BTN, &CTargetTestTab::OnBtnRunAll)
    ON_BN_CLICKED(IDC_TARGET_DEINIT_BTN, &CTargetTestTab::OnBtnDeinit)
    ON_BN_CLICKED(IDC_TARGET_CLEAR_BTN, &CTargetTestTab::OnBtnClear)
    ON_BN_CLICKED(IDC_TARGET_LOAD_SEQ_BTN, &CTargetTestTab::OnBtnLoadSequence)
    ON_BN_CLICKED(IDC_TARGET_SAVE_SEQ_BTN, &CTargetTestTab::OnBtnSaveSequence)
    ON_BN_CLICKED(IDC_TARGET_DEFAULT_SEQ_BTN, &CTargetTestTab::OnBtnDefaultSequence)
    ON_BN_CLICKED(IDC_TARGET_ADD_ROW_BTN, &CTargetTestTab::OnBtnAddRow)
    ON_BN_CLICKED(IDC_TARGET_REMOVE_ROW_BTN, &CTargetTestTab::OnBtnRemoveRow)
    ON_BN_CLICKED(IDC_TARGET_UPDATE_ROW_BTN, &CTargetTestTab::OnBtnUpdateRow)
    ON_BN_CLICKED(IDC_TARGET_MOVE_UP_BTN, &CTargetTestTab::OnBtnMoveUp)
    ON_BN_CLICKED(IDC_TARGET_MOVE_DOWN_BTN, &CTargetTestTab::OnBtnMoveDown)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TARGET_COMMAND_LIST, &CTargetTestTab::OnCommandRowChanged)
END_MESSAGE_MAP()

BOOL CTargetTestTab::Create(CWnd* parent, const RECT& rect, UINT id) {
    CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return CWnd::CreateEx(0, cls, L"", WS_CHILD | WS_VISIBLE, rect, parent, id);
}

void CTargetTestTab::Bind(mfc_tool::core::BridgeService* service,
                        std::function<void(const std::wstring&)> logger) {
    service_ = service;
    log_ = std::move(logger);
}

int CTargetTestTab::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    mfc_tool::ui::CreatePointFontForWindow(ui_font_, *this, 85);
    mfc_tool::ui::CreatePointFontForWindow(list_font_, *this, 80);

    auto mk_static = [this](CStatic& s, const wchar_t* text, UINT id) {
        s.Create(text, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, id);
        s.SetFont(&ui_font_);
    };
    auto mk_edit = [this](CEdit& e, const wchar_t* text, UINT id) {
        e.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, id);
        e.SetFont(&ui_font_);
        e.SetWindowTextW(text);
    };
    auto mk_btn = [this](CButton& b, const wchar_t* text, UINT id, DWORD style = BS_PUSHBUTTON) {
        b.Create(text, WS_CHILD | WS_VISIBLE | style, CRect(0, 0, 0, 0), this, id);
        b.SetFont(&ui_font_);
    };

    mk_btn(config_group_, L"Bus, Sequence INI, And SGPIO Setup", IDC_TARGET_CONFIG_GROUP, BS_GROUPBOX);
    mk_static(seq_path_label_, L"Sequence INI", IDC_TARGET_SEQ_PATH_LABEL);
    mk_edit(seq_path_edit_, L"", IDC_TARGET_SEQ_PATH_EDIT);
    mk_btn(load_seq_btn_, L"Load", IDC_TARGET_LOAD_SEQ_BTN);
    mk_btn(save_seq_btn_, L"Save", IDC_TARGET_SAVE_SEQ_BTN);
    mk_btn(default_seq_btn_, L"Default", IDC_TARGET_DEFAULT_SEQ_BTN);
    mk_static(pins_label_, L"I2C Pins", IDC_TARGET_PINS_LABEL);
    pins_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                       CRect(0, 0, 0, 0), this, IDC_TARGET_PINS_COMBO);
    pins_combo_.SetFont(&ui_font_);
    mk_static(baud_label_, L"Speed", IDC_TARGET_BAUD_LABEL);
    mk_edit(baud_edit_, L"100000", IDC_TARGET_BAUD_EDIT);
    mk_static(addr_label_, L"Addr7", IDC_TARGET_ADDR_LABEL);
    mk_edit(addr_edit_, L"0x64", IDC_TARGET_ADDR_EDIT);
    mk_static(scan_label_, L"Scan", IDC_TARGET_SCAN_LABEL);
    mk_edit(scan_start_edit_, L"0x60", IDC_TARGET_SCAN_START_EDIT);
    mk_static(scan_to_label_, L"to", IDC_TARGET_SCAN_TO_LABEL);
    mk_edit(scan_end_edit_, L"0x67", IDC_TARGET_SCAN_END_EDIT);
    mk_static(sgpio_label_, L"SGPIO Slots", IDC_TARGET_SGPIO_LABEL);
    mk_edit(slot_count_edit_, L"8", IDC_TARGET_SLOT_COUNT_EDIT);
    mk_static(clock_label_, L"Clock", IDC_TARGET_CLOCK_LABEL);
    mk_edit(clock_edit_, L"100000", IDC_TARGET_CLOCK_EDIT);
    mk_static(interval_label_, L"Interval", IDC_TARGET_INTERVAL_LABEL);
    mk_edit(interval_edit_, L"100", IDC_TARGET_INTERVAL_EDIT);
    mk_btn(loop_check_, L"Loop", IDC_TARGET_LOOP_CHECK, BS_AUTOCHECKBOX);
    mk_static(loop_interval_label_, L"Loop Interval(ms)", IDC_TARGET_LOOP_INTERVAL_LABEL);
    mk_edit(loop_interval_edit_, L"1000", IDC_TARGET_LOOP_INTERVAL_EDIT);

    mk_btn(command_group_, L"Command Sequence Rows", IDC_TARGET_COMMAND_GROUP, BS_GROUPBOX);
    command_list_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                         CRect(0, 0, 0, 0), this, IDC_TARGET_COMMAND_LIST);
    command_list_.SetFont(&list_font_);
    command_list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    command_list_.InsertColumn(0, L"#", LVCFMT_RIGHT, 42);
    command_list_.InsertColumn(1, L"Kind", LVCFMT_LEFT, 78);
    command_list_.InsertColumn(2, L"Reg", LVCFMT_LEFT, 58);
    command_list_.InsertColumn(3, L"Data / SGPIO", LVCFMT_LEFT, 190);
    command_list_.InsertColumn(4, L"Read", LVCFMT_LEFT, 52);
    command_list_.InsertColumn(5, L"Expect", LVCFMT_LEFT, 80);
    command_list_.InsertColumn(6, L"Mask", LVCFMT_LEFT, 70);
    command_list_.InsertColumn(7, L"Delay", LVCFMT_LEFT, 58);
    command_list_.InsertColumn(8, L"Comment", LVCFMT_LEFT, 310);

    mk_static(kind_label_, L"Kind", IDC_TARGET_KIND_LABEL);
    kind_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                       CRect(0, 0, 0, 0), this, IDC_TARGET_KIND_COMBO);
    kind_combo_.SetFont(&ui_font_);
    PopulateKindCombo();
    mk_static(reg_label_, L"Reg", IDC_TARGET_REG_LABEL);
    mk_edit(reg_edit_, L"0x30", IDC_TARGET_REG_EDIT);
    mk_static(data_label_, L"Write Data / SGPIO", IDC_TARGET_DATA_LABEL);
    mk_edit(data_edit_, L"", IDC_TARGET_DATA_EDIT);
    mk_static(read_len_label_, L"Read Len", IDC_TARGET_READ_LEN_LABEL);
    mk_edit(read_len_edit_, L"1", IDC_TARGET_READ_LEN_EDIT);
    mk_static(expect_label_, L"Expect", IDC_TARGET_EXPECT_LABEL);
    mk_edit(expect_edit_, L"", IDC_TARGET_EXPECT_EDIT);
    mk_static(mask_label_, L"Mask", IDC_TARGET_MASK_LABEL);
    mk_edit(mask_edit_, L"", IDC_TARGET_MASK_EDIT);
    mk_static(delay_label_, L"Delay", IDC_TARGET_DELAY_LABEL);
    mk_edit(delay_edit_, L"20", IDC_TARGET_DELAY_EDIT);
    mk_static(comment_label_, L"Comment", IDC_TARGET_COMMENT_LABEL);
    mk_edit(comment_edit_, L"", IDC_TARGET_COMMENT_EDIT);
    mk_btn(add_row_btn_, L"+", IDC_TARGET_ADD_ROW_BTN);
    mk_btn(remove_row_btn_, L"-", IDC_TARGET_REMOVE_ROW_BTN);
    mk_btn(update_row_btn_, L"Update", IDC_TARGET_UPDATE_ROW_BTN);
    mk_btn(move_up_btn_, L"Up", IDC_TARGET_MOVE_UP_BTN);
    mk_btn(move_down_btn_, L"Down", IDC_TARGET_MOVE_DOWN_BTN);

    mk_btn(action_group_, L"Validation", IDC_TARGET_ACTION_GROUP, BS_GROUPBOX);
    mk_btn(init_btn_, L"Init I2C", IDC_TARGET_INIT_BTN);
    mk_btn(scan_btn_, L"Scan Address", IDC_TARGET_SCAN_BTN);
    mk_btn(run_all_btn_, L"Run Sequence", IDC_TARGET_RUN_ALL_BTN);
    mk_btn(deinit_btn_, L"Deinit", IDC_TARGET_DEINIT_BTN);
    mk_btn(clear_btn_, L"Clear", IDC_TARGET_CLEAR_BTN);
    mk_static(status_label_, L"Status", IDC_TARGET_STATUS_LABEL);
    mk_static(status_value_, L"Idle", IDC_TARGET_STATUS_VALUE);

    mk_btn(result_group_, L"Test Results", IDC_TARGET_RESULT_GROUP, BS_GROUPBOX);
    result_list_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                        CRect(0, 0, 0, 0), this, IDC_TARGET_RESULT_LIST);
    result_list_.SetFont(&list_font_);
    result_list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    result_list_.InsertColumn(0, L"#", LVCFMT_RIGHT, 44);
    result_list_.InsertColumn(1, L"Result", LVCFMT_LEFT, 70);
    result_list_.InsertColumn(2, L"Step", LVCFMT_LEFT, 210);
    result_list_.InsertColumn(3, L"Details", LVCFMT_LEFT, 620);

    mk_btn(slot_group_, L"Slot 0..7 SMBus Readback", IDC_TARGET_SLOT_GROUP, BS_GROUPBOX);
    slot_list_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                      CRect(0, 0, 0, 0), this, IDC_TARGET_SLOT_LIST);
    slot_list_.SetFont(&list_font_);
    slot_list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    slot_list_.InsertColumn(0, L"Slot", LVCFMT_RIGHT, 46);
    slot_list_.InsertColumn(1, L"Value", LVCFMT_LEFT, 70);
    slot_list_.InsertColumn(2, L"Low3", LVCFMT_LEFT, 70);
    slot_list_.InsertColumn(3, L"Expected", LVCFMT_LEFT, 80);
    slot_list_.InsertColumn(4, L"Presence bit4", LVCFMT_LEFT, 105);
    slot_list_.InsertColumn(5, L"Result", LVCFMT_LEFT, 80);

    PopulatePinCombo(L"I2C0_PB4_PB5");
    seq_path_edit_.SetWindowTextW(DefaultSequencePath().c_str());
    AddDefaultSequenceRows();
    RefreshCommandList();
    ClearSlots();
    UpdateEnableState();
    return 0;
}

void CTargetTestTab::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutControls(CRect(0, 0, cx, cy));
}

void CTargetTestTab::RefreshDpiLayout() {
    if (!::IsWindow(GetSafeHwnd())) {
        return;
    }
    mfc_tool::ui::CreatePointFontForWindow(ui_font_, *this, 85);
    mfc_tool::ui::CreatePointFontForWindow(list_font_, *this, 80);
    mfc_tool::ui::ApplyFontToChildWindows(*this, ui_font_, FALSE);
    command_list_.SetFont(&list_font_, FALSE);
    result_list_.SetFont(&list_font_, FALSE);
    slot_list_.SetFont(&list_font_, FALSE);

    CRect rc;
    GetClientRect(&rc);
    LayoutControls(rc);
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void CTargetTestTab::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent != kSequenceLoopTimerId) {
        CWnd::OnTimer(nIDEvent);
        return;
    }

    KillTimer(kSequenceLoopTimerId);
    if (!loop_running_) {
        return;
    }
    if (!connected_ || service_ == nullptr) {
        StopSequenceLoop(L"Loop stopped: disconnected");
        return;
    }

    ++loop_iteration_;
    SetStatusText(L"Loop #" + std::to_wstring(loop_iteration_) + L" running");
    try {
        RunSequence();
    } catch (const std::exception& e) {
        StopSequenceLoop(L"Loop stopped: sequence failed");
        AddResult(L"Run sequence loop", false, AnsiToWide(e.what()));
        ::MessageBoxW(m_hWnd, AnsiToWide(e.what()).c_str(), L"Run Sequence Error", MB_ICONERROR | MB_OK);
        return;
    }

    ScheduleNextLoop();
}

void CTargetTestTab::PopulateKindCombo() {
    kind_combo_.ResetContent();
    kind_combo_.AddString(L"Scan");
    kind_combo_.AddString(L"Read");
    kind_combo_.AddString(L"Write");
    kind_combo_.AddString(L"WriteRead");
    kind_combo_.AddString(L"SGPIO");
    kind_combo_.AddString(L"Delay");
    kind_combo_.SetCurSel(1);
}

void CTargetTestTab::PopulatePinCombo(const std::wstring& preferred_ini_name) {
    const auto& pairs = mfc_tool::core::board_i2c::AllPinPairs();
    int preferred = 0;

    pins_combo_.ResetContent();
    for (int i = 0; i < static_cast<int>(pairs.size()); ++i) {
        pins_combo_.AddString(pairs[static_cast<size_t>(i)].label);
        if (preferred_ini_name == pairs[static_cast<size_t>(i)].ini_name) {
            preferred = i;
        }
    }
    pins_combo_.SetCurSel(preferred);
}

void CTargetTestTab::LayoutControls(const CRect& r) {
    if (!::IsWindow(config_group_.GetSafeHwnd()) || r.Width() <= 0 || r.Height() <= 0) {
        return;
    }

    const mfc_tool::ui::DpiScaler dpi = mfc_tool::ui::DpiScaler::FromWindow(*this);
    const mfc_tool::ui::LayoutMetrics metrics = mfc_tool::ui::MetricsForWindow(*this);
    const int margin = metrics.margin8;
    const int gap = metrics.gap;
    const int row = metrics.row26;
    const int label_h = metrics.label18;
    const int label_pad = dpi.Scale(8);
    const int y_pad = dpi.Scale(4);
    const int config_h = dpi.Scale(126);
    const int command_h = dpi.Scale(300);
    const int top_y = r.top + margin;
    const int list_bottom = r.bottom - margin;
    const int result_y = top_y + config_h + gap + command_h + gap;
    const int slot_w = (std::max)(dpi.Scale(430), r.Width() / 3);
    const int result_w = (std::max)(dpi.Scale(520), r.Width() - slot_w - margin * 3);
    int x = r.left + margin + dpi.Scale(12);
    int y = top_y + dpi.Scale(28);
    int btn_w = dpi.Scale(78);
    int action_x = x;

    config_group_.MoveWindow(r.left + margin, top_y, r.Width() - margin * 2, config_h);
    x = mfc_tool::ui::PlaceLabelAndControl(seq_path_label_, seq_path_edit_, x, y + y_pad, y,
                                           (std::max)(dpi.Scale(420), r.Width() - dpi.Scale(520)),
                                           row, gap, label_pad, label_h) + gap;
    load_seq_btn_.MoveWindow(x, y, btn_w, row);
    x += btn_w + gap;
    save_seq_btn_.MoveWindow(x, y, btn_w, row);
    x += btn_w + gap;
    default_seq_btn_.MoveWindow(x, y, btn_w, row);

    y += row + gap;
    x = r.left + margin + dpi.Scale(12);
    x = mfc_tool::ui::PlaceLabelAndControl(pins_label_, pins_combo_, x, y + y_pad, y,
                                           (std::max)(dpi.Scale(260), r.Width() / 4),
                                           row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(baud_label_, baud_edit_, x, y + y_pad, y, dpi.Scale(84), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(addr_label_, addr_edit_, x, y + y_pad, y, dpi.Scale(64), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(scan_label_, scan_start_edit_, x, y + y_pad, y, dpi.Scale(58), row, gap, label_pad, label_h) + gap;
    scan_to_label_.MoveWindow(x, y + y_pad, dpi.Scale(18), label_h);
    x += dpi.Scale(18) + gap;
    scan_end_edit_.MoveWindow(x, y, dpi.Scale(58), row);
    x += dpi.Scale(58) + dpi.Scale(16);
    x = mfc_tool::ui::PlaceLabelAndControl(sgpio_label_, slot_count_edit_, x, y + y_pad, y, dpi.Scale(48), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(clock_label_, clock_edit_, x, y + y_pad, y, dpi.Scale(76), row, gap, label_pad, label_h) + gap;
    mfc_tool::ui::PlaceLabelAndControl(interval_label_, interval_edit_, x, y + y_pad, y, dpi.Scale(58), row, gap, label_pad, label_h);

    y += row + gap;
    action_x = r.left + margin + dpi.Scale(12);
    btn_w = (std::max)(dpi.Scale(104), mfc_tool::ui::MeasureButtonMinWidth(run_all_btn_, dpi.Scale(20)));
    init_btn_.MoveWindow(action_x, y, btn_w, row);
    action_x += btn_w + gap;
    scan_btn_.MoveWindow(action_x, y, btn_w, row);
    action_x += btn_w + gap;
    run_all_btn_.MoveWindow(action_x, y, btn_w + dpi.Scale(20), row);
    action_x += btn_w + dpi.Scale(20) + gap;
    deinit_btn_.MoveWindow(action_x, y, btn_w, row);
    action_x += btn_w + gap;
    clear_btn_.MoveWindow(action_x, y, btn_w, row);
    action_x += btn_w + gap;
    loop_check_.MoveWindow(action_x, y + dpi.Scale(3), dpi.Scale(66), metrics.row24);
    action_x += dpi.Scale(66) + gap;
    action_x = mfc_tool::ui::PlaceLabelAndControl(loop_interval_label_, loop_interval_edit_,
                                                  action_x, y + y_pad, y,
                                                  dpi.Scale(64), row, gap, dpi.Scale(5), label_h) + dpi.Scale(18);
    status_label_.MoveWindow(action_x, y + y_pad, dpi.Scale(48), label_h);
    action_x += dpi.Scale(48) + gap;
    status_value_.MoveWindow(action_x, y + y_pad,
                             (std::max)(dpi.Scale(160), static_cast<int>(r.right) - margin - action_x),
                             label_h);
    action_group_.ShowWindow(SW_HIDE);

    y = top_y + config_h + gap;
    command_group_.MoveWindow(r.left + margin, y, r.Width() - margin * 2, command_h);
    const int command_list_h = command_h - dpi.Scale(134);
    command_list_.MoveWindow(r.left + margin + dpi.Scale(10), y + dpi.Scale(24),
                              r.Width() - margin * 2 - dpi.Scale(20), command_list_h);

    {
        const int edit_y = y + dpi.Scale(24) + command_list_h + dpi.Scale(8);
        const int button_y = edit_y + row + gap;
        int label_w = 0;
        int control_x = 0;
        x = r.left + margin + dpi.Scale(12);
        kind_label_.MoveWindow(x, edit_y + y_pad, dpi.Scale(36), label_h);
        x += dpi.Scale(36) + gap;
        kind_combo_.MoveWindow(x, edit_y, dpi.Scale(96), row + metrics.comboDrop);
        x += dpi.Scale(96) + gap;
        x = mfc_tool::ui::PlaceLabelAndControl(reg_label_, reg_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(58), row, gap, dpi.Scale(5), label_h) + gap;
        x = mfc_tool::ui::PlaceLabelAndControl(data_label_, data_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(205), row, gap, dpi.Scale(5), label_h) + gap;
        x = mfc_tool::ui::PlaceLabelAndControl(read_len_label_, read_len_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(44), row, gap, dpi.Scale(5), label_h) + gap;
        x = mfc_tool::ui::PlaceLabelAndControl(expect_label_, expect_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(72), row, gap, dpi.Scale(5), label_h) + gap;
        x = mfc_tool::ui::PlaceLabelAndControl(mask_label_, mask_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(58), row, gap, dpi.Scale(5), label_h) + gap;
        mfc_tool::ui::PlaceLabelAndControl(delay_label_, delay_edit_, x, edit_y + y_pad, edit_y, dpi.Scale(54), row, gap, dpi.Scale(5), label_h);

        x = r.left + margin + dpi.Scale(12);
        add_row_btn_.MoveWindow(x, button_y, dpi.Scale(34), row);
        x += dpi.Scale(34) + gap;
        remove_row_btn_.MoveWindow(x, button_y, dpi.Scale(34), row);
        x += dpi.Scale(34) + gap;
        update_row_btn_.MoveWindow(x, button_y, dpi.Scale(76), row);
        x += dpi.Scale(76) + gap;
        move_up_btn_.MoveWindow(x, button_y, dpi.Scale(54), row);
        x += dpi.Scale(54) + gap;
        move_down_btn_.MoveWindow(x, button_y, dpi.Scale(66), row);
        x += dpi.Scale(66) + dpi.Scale(18);
        label_w = mfc_tool::ui::PlaceLabel(comment_label_, x, button_y + y_pad, dpi.Scale(5), label_h);
        control_x = x + label_w + gap;
        comment_edit_.MoveWindow(control_x, button_y,
                                 (std::max)(0, static_cast<int>(r.right) - margin - control_x - dpi.Scale(12)),
                                 row);
    }

    result_group_.MoveWindow(r.left + margin, result_y, result_w, list_bottom - result_y);
    result_list_.MoveWindow(r.left + margin + dpi.Scale(10), result_y + dpi.Scale(24),
                            result_w - dpi.Scale(20), list_bottom - result_y - dpi.Scale(34));
    slot_group_.MoveWindow(r.left + margin + result_w + margin, result_y,
                           r.Width() - result_w - margin * 3, list_bottom - result_y);
    slot_list_.MoveWindow(r.left + margin + result_w + margin + dpi.Scale(10), result_y + dpi.Scale(24),
                          r.Width() - result_w - margin * 3 - dpi.Scale(20),
                          list_bottom - result_y - dpi.Scale(34));
}

void CTargetTestTab::SetConnected(bool connected) {
    connected_ = connected;
    if (!connected_) {
        StopSequenceLoop(L"Disconnected");
        i2c_enabled_ = false;
        SetStatusText(L"Disconnected");
    }
    UpdateEnableState();
}

void CTargetTestTab::OnDisconnected() {
    StopSequenceLoop(L"Disconnected");
    i2c_enabled_ = false;
    SetStatusText(L"Disconnected");
    UpdateEnableState();
}

void CTargetTestTab::LoadState(const mfc_tool::core::AppState& state) {
    PopulatePinCombo(state.target_test.pin_pair);
    baud_edit_.SetWindowTextW(state.target_test.baud.c_str());
    addr_edit_.SetWindowTextW(state.target_test.addr.c_str());
    scan_start_edit_.SetWindowTextW(state.target_test.scan_start.c_str());
    scan_end_edit_.SetWindowTextW(state.target_test.scan_end.c_str());
    slot_count_edit_.SetWindowTextW(state.target_test.slot_count.c_str());
    clock_edit_.SetWindowTextW(state.target_test.clock_hz.c_str());
    interval_edit_.SetWindowTextW(state.target_test.interval_ms.c_str());
    loop_check_.SetCheck(state.target_test.loop_enabled == L"1" ? BST_CHECKED : BST_UNCHECKED);
    loop_interval_edit_.SetWindowTextW(state.target_test.loop_interval_ms.c_str());
    seq_path_edit_.SetWindowTextW((state.target_test.sequence_path.empty() ? DefaultSequencePath() : state.target_test.sequence_path).c_str());
    if (!LoadSequenceFromFile(GetEditText(seq_path_edit_), false)) {
        AddDefaultSequenceRows();
        RefreshCommandList();
    }
    i2c_enabled_ = false;
    SetStatusText(L"Idle");
    UpdateEnableState();
}

void CTargetTestTab::SaveState(mfc_tool::core::AppState* state) const {
    if (state == nullptr) {
        return;
    }
    state->target_test.pin_pair = CurrentPinPairIniName();
    state->target_test.baud = GetEditText(baud_edit_);
    state->target_test.addr = GetEditText(addr_edit_);
    state->target_test.scan_start = GetEditText(scan_start_edit_);
    state->target_test.scan_end = GetEditText(scan_end_edit_);
    state->target_test.slot_count = GetEditText(slot_count_edit_);
    state->target_test.clock_hz = GetEditText(clock_edit_);
    state->target_test.interval_ms = GetEditText(interval_edit_);
    state->target_test.loop_enabled = loop_check_.GetCheck() == BST_CHECKED ? L"1" : L"0";
    state->target_test.loop_interval_ms = GetEditText(loop_interval_edit_);
    state->target_test.sequence_path = GetEditText(seq_path_edit_);
}

void CTargetTestTab::UpdateEnableState() {
    if (!::IsWindow(init_btn_.GetSafeHwnd())) {
        return;
    }
    const BOOL has_rows = commands_.empty() ? FALSE : TRUE;
    const BOOL has_selection = SelectedCommandIndex() >= 0 ? TRUE : FALSE;
    const BOOL can_edit = loop_running_ ? FALSE : TRUE;
    run_all_btn_.SetWindowTextW(loop_running_ ? L"Stop Loop" : L"Run Sequence");
    mfc_tool::ui::SafeEnableWindow(init_btn_, connected_ ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(scan_btn_, connected_ ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(run_all_btn_, (connected_ && has_rows) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(deinit_btn_, (connected_ && i2c_enabled_) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(clear_btn_, TRUE);
    mfc_tool::ui::SafeEnableWindow(load_seq_btn_, can_edit);
    mfc_tool::ui::SafeEnableWindow(save_seq_btn_, can_edit);
    mfc_tool::ui::SafeEnableWindow(default_seq_btn_, can_edit);
    mfc_tool::ui::SafeEnableWindow(loop_check_, loop_running_ ? FALSE : TRUE);
    mfc_tool::ui::SafeEnableWindow(loop_interval_edit_, loop_running_ ? FALSE : TRUE);
    mfc_tool::ui::SafeEnableWindow(add_row_btn_, can_edit);
    mfc_tool::ui::SafeEnableWindow(remove_row_btn_, (can_edit && has_selection) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(update_row_btn_, (can_edit && has_selection) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(move_up_btn_, (can_edit && has_selection) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(move_down_btn_, (can_edit && has_selection) ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(pins_combo_, connected_ && !i2c_enabled_ && can_edit ? TRUE : FALSE);
    mfc_tool::ui::SafeEnableWindow(baud_edit_, connected_ && !i2c_enabled_ && can_edit ? TRUE : FALSE);
}

void CTargetTestTab::SetStatusText(const std::wstring& text) {
    if (::IsWindow(status_value_.GetSafeHwnd())) {
        status_value_.SetWindowTextW(text.c_str());
    }
}

void CTargetTestTab::AddResult(const std::wstring& step, bool pass, const std::wstring& details) {
    const int row = result_list_.GetItemCount();
    CString index_text;
    index_text.Format(L"%d", row + 1);
    result_list_.InsertItem(row, index_text);
    result_list_.SetItemText(row, 1, pass ? L"PASS" : L"FAIL");
    result_list_.SetItemText(row, 2, step.c_str());
    result_list_.SetItemText(row, 3, details.c_str());
    result_list_.EnsureVisible(row, FALSE);
    if (log_) {
        log_(std::wstring(L"Validation sequence ") + (pass ? L"PASS " : L"FAIL ") + step + L": " + details);
    }
}

void CTargetTestTab::ClearResults() {
    result_list_.DeleteAllItems();
}

void CTargetTestTab::ClearSlots() {
    slot_list_.DeleteAllItems();
    for (int i = 0; i < 8; ++i) {
        CString slot_text;
        slot_text.Format(L"%d", i);
        slot_list_.InsertItem(i, slot_text);
        slot_list_.SetItemText(i, 1, L"-");
        slot_list_.SetItemText(i, 2, L"-");
        slot_list_.SetItemText(i, 3, HexByte(SlotExpectedLowBits(i)).c_str());
        slot_list_.SetItemText(i, 4, L"-");
        slot_list_.SetItemText(i, 5, L"-");
    }
}

void CTargetTestTab::SetSlotRow(int slot, std::uint8_t value, std::uint8_t expected_low, bool checked) {
    if (slot < 0 || slot >= slot_list_.GetItemCount()) {
        return;
    }
    const std::uint8_t low = static_cast<std::uint8_t>(value & 0x07u);
    slot_list_.SetItemText(slot, 1, HexByte(value).c_str());
    slot_list_.SetItemText(slot, 2, HexByte(low).c_str());
    slot_list_.SetItemText(slot, 3, HexByte(expected_low).c_str());
    slot_list_.SetItemText(slot, 4, (value & 0x10u) ? L"1" : L"0");
    slot_list_.SetItemText(slot, 5, checked ? (low == expected_low ? L"PASS" : L"FAIL") : L"INFO");
}

void CTargetTestTab::InitI2cFromUi() {
    const auto& pair = CurrentPinPair();
    const int baud = ParseEditInt(baud_edit_);
    service_->I2cMasterInit(pair.i2c_port, pair.sda_pin, pair.scl_pin, baud);
    service_->I2cMasterBusStatus(pair.i2c_port, true);
    i2c_enabled_ = true;
    SetStatusText(L"I2C master ready");
    AddResult(L"I2C init", true, std::wstring(pair.label) + L", speed=" + std::to_wstring(baud));
    UpdateEnableState();
}

std::vector<int> CTargetTestTab::ScanAddresses(bool update_ui) {
    const auto& pair = CurrentPinPair();
    int start = ParseEditInt(scan_start_edit_);
    int end = ParseEditInt(scan_end_edit_);
    std::vector<int> found;

    start = (std::max)(0x03, (std::min)(start, 0x77));
    end = (std::max)(0x03, (std::min)(end, 0x77));
    if (start > end) {
        std::swap(start, end);
    }

    for (int addr = start; addr <= end; ++addr) {
        try {
            const std::vector<std::uint8_t> rx = ReadReg(addr, 0x30, 1);
            if (!rx.empty()) {
                found.push_back(addr);
            }
        } catch (...) {
        }
    }

    if (!found.empty()) {
        last_scanned_addr_ = found.front();
        if (update_ui) {
            addr_edit_.SetWindowTextW(HexWord(found.front()).c_str());
        }
    }

    if (found.empty()) {
        AddResult(L"Address scan", false,
                  L"no target response in " + HexWord(start) + L".." + HexWord(end));
    } else {
        std::wstringstream ss;
        for (size_t i = 0; i < found.size(); ++i) {
            if (i > 0u) {
                ss << L", ";
            }
            ss << HexWord(found[i]);
        }
        AddResult(L"Address scan", true,
                  L"found " + std::to_wstring(found.size()) + L": " + ss.str() +
                  L" on " + mfc_tool::core::board_i2c::PortLabel(pair.i2c_port));
    }
    return found;
}

std::vector<std::uint8_t> CTargetTestTab::ReadReg(int addr7, int reg, int length) {
    const auto& pair = CurrentPinPair();
    std::vector<std::uint8_t> tx;
    tx.push_back(static_cast<std::uint8_t>(reg & 0xFF));
    return service_->I2cMasterWriteThenReadRaw(pair.i2c_port, addr7, true, tx, length);
}

void CTargetTestTab::WriteRegBytes(int addr7, int reg, const std::vector<std::uint8_t>& data) {
    const auto& pair = CurrentPinPair();
    std::vector<std::uint8_t> tx;
    tx.push_back(static_cast<std::uint8_t>(reg & 0xFF));
    tx.insert(tx.end(), data.begin(), data.end());
    service_->I2cMasterWrite(pair.i2c_port, addr7, tx);
}

void CTargetTestTab::RunSequence() {
    int addr7 = (last_scanned_addr_ >= 0) ? last_scanned_addr_ : CurrentAddress();
    bool all_pass = true;

    EnsureSequenceLoaded();
    ClearResults();
    ClearSlots();
    InitI2cFromUi();

    for (int i = 0; i < static_cast<int>(commands_.size()); ++i) {
        const bool pass = ExecuteRow(commands_[static_cast<size_t>(i)], i, &addr7);
        all_pass = all_pass && pass;
    }

    try {
        service_->SgpioOff();
        AddResult(L"SGPIO off", true, L"bridge pins returned to idle");
    } catch (...) {
        AddResult(L"SGPIO off", false, L"bridge returned an error while stopping SGPIO");
        all_pass = false;
    }

    SetStatusText(all_pass ? L"Sequence complete: PASS" : L"Sequence complete: check failures");
}

void CTargetTestTab::ScheduleNextLoop() {
    int interval_ms = 0;

    if (!loop_running_) {
        return;
    }

    interval_ms = LoopIntervalMs();
    SetTimer(kSequenceLoopTimerId, static_cast<UINT>((std::max)(1, interval_ms)), nullptr);
    SetStatusText(L"Loop #" + std::to_wstring(loop_iteration_) +
                  L" complete; next in " + std::to_wstring(interval_ms) + L" ms");
    UpdateEnableState();
}

void CTargetTestTab::StopSequenceLoop(const std::wstring& status_text) {
    if (loop_running_) {
        KillTimer(kSequenceLoopTimerId);
        loop_running_ = false;
        loop_iteration_ = 0;
    }
    if (::IsWindow(run_all_btn_.GetSafeHwnd())) {
        run_all_btn_.SetWindowTextW(L"Run Sequence");
    }
    if (!status_text.empty() && ::IsWindow(status_value_.GetSafeHwnd())) {
        SetStatusText(status_text);
    }
    UpdateEnableState();
}

int CTargetTestTab::LoopIntervalMs() const {
    return std::clamp(ParseIntOrDefault(GetEditText(loop_interval_edit_), 1000), 0, 3600000);
}

bool CTargetTestTab::ExecuteRow(const CommandRow& row, int index, int* addr7) {
    if (addr7 == nullptr) {
        return false;
    }
    switch (row.kind) {
    case CommandKind::Scan:
    {
        const std::vector<int> found = ScanAddresses(true);
        if (!found.empty()) {
            *addr7 = found.front();
            return true;
        }
        return false;
    }
    case CommandKind::Read:
        return ExecuteReadRow(row, index, *addr7);
    case CommandKind::Write:
        return ExecuteWriteRow(row, index, *addr7);
    case CommandKind::WriteRead:
        return ExecuteWriteReadRow(row, index, *addr7);
    case CommandKind::Sgpio:
        return ExecuteSgpioRow(row, index);
    case CommandKind::Delay:
        ::Sleep(static_cast<DWORD>((std::max)(0, ParseIntOrDefault(row.delay_ms, 20))));
        AddResult(L"Step " + std::to_wstring(index + 1) + L" Delay", true, row.delay_ms + L" ms");
        return true;
    default:
        break;
    }
    AddResult(L"Step " + std::to_wstring(index + 1), false, L"unsupported command kind");
    return false;
}

bool CTargetTestTab::ExecuteReadRow(const CommandRow& row, int index, int addr7) {
    const int reg = ParseIntOrDefault(row.reg, 0);
    const int read_len = (std::max)(1, ParseIntOrDefault(row.read_len, 1));
    const std::vector<std::uint8_t> expected = ParseByteList(row.expect);
    const std::vector<std::uint8_t> mask = ParseByteList(row.mask);
    const std::vector<std::uint8_t> rx = ReadReg(addr7, reg, read_len);
    const bool pass = expected.empty() ? (static_cast<int>(rx.size()) == read_len) : CompareBytes(rx, expected, mask);
    const std::wstring step = L"Step " + std::to_wstring(index + 1) + L" Read " + HexByte(static_cast<std::uint8_t>(reg));
    std::wstring details = L"addr=" + HexWord(addr7) + L" rx=" + HexBytes(rx);

    if (!expected.empty()) {
        details += L" expect=" + HexBytes(expected);
        if (!mask.empty()) {
            details += L" mask=" + HexBytes(mask);
        }
    }
    if (!row.comment.empty()) {
        details += L" (" + row.comment + L")";
    }

    if (reg >= 0 && reg <= 7 && rx.size() == 1u) {
        std::uint8_t expected_low = SlotExpectedLowBits(reg);
        bool check_slot = false;
        if (!expected.empty()) {
            expected_low = static_cast<std::uint8_t>(expected[0] & 0x07u);
            check_slot = true;
        }
        SetSlotRow(reg, rx[0], expected_low, check_slot);
    }

    AddResult(step, pass, details);
    return pass;
}

bool CTargetTestTab::ExecuteWriteRow(const CommandRow& row, int index, int addr7) {
    const int reg = ParseIntOrDefault(row.reg, 0);
    const std::vector<std::uint8_t> data = ParseByteList(row.data);
    const int delay_ms = (std::max)(0, ParseIntOrDefault(row.delay_ms, 20));

    WriteRegBytes(addr7, reg, data);
    if (delay_ms > 0) {
        ::Sleep(static_cast<DWORD>(delay_ms));
    }
    AddResult(L"Step " + std::to_wstring(index + 1) + L" Write " + HexByte(static_cast<std::uint8_t>(reg)),
              true,
              L"addr=" + HexWord(addr7) + L" data=" + HexBytes(data) +
              (row.comment.empty() ? L"" : L" (" + row.comment + L")"));
    return true;
}

bool CTargetTestTab::ExecuteWriteReadRow(const CommandRow& row, int index, int addr7) {
    const int reg = ParseIntOrDefault(row.reg, 0);
    const int read_len = (std::max)(1, ParseIntOrDefault(row.read_len, 1));
    const int delay_ms = (std::max)(0, ParseIntOrDefault(row.delay_ms, 20));
    const std::vector<std::uint8_t> data = ParseByteList(row.data);
    std::vector<std::uint8_t> expected = ParseByteList(row.expect);
    const std::vector<std::uint8_t> mask = ParseByteList(row.mask);
    std::vector<std::uint8_t> rx;
    bool pass = false;

    if (expected.empty()) {
        expected = data;
    }
    WriteRegBytes(addr7, reg, data);
    if (delay_ms > 0) {
        ::Sleep(static_cast<DWORD>(delay_ms));
    }
    rx = ReadReg(addr7, reg, read_len);
    pass = CompareBytes(rx, expected, mask);
    AddResult(L"Step " + std::to_wstring(index + 1) + L" WriteRead " + HexByte(static_cast<std::uint8_t>(reg)),
              pass,
              L"addr=" + HexWord(addr7) + L" data=" + HexBytes(data) +
              L" rx=" + HexBytes(rx) + L" expect=" + HexBytes(expected) +
              (mask.empty() ? L"" : L" mask=" + HexBytes(mask)) +
              (row.comment.empty() ? L"" : L" (" + row.comment + L")"));
    return pass;
}

bool CTargetTestTab::ExecuteSgpioRow(const CommandRow& row, int index) {
    const int slots = std::clamp(SgpioValueFromData(row, L"SLOTS", ParseEditInt(slot_count_edit_)), 1, 16);
    const int clock_hz = SgpioValueFromData(row, L"CLOCK", ParseEditInt(clock_edit_));
    const int interval_ms = std::clamp(SgpioValueFromData(row, L"INTERVAL", ParseEditInt(interval_edit_)), 20, 5000);
    const int sload_raw = SgpioValueFromData(row, L"SLOAD", 0);
    const int act_mask = SgpioValueFromData(row, L"ACT", kSgpioActMask);
    const int locate_mask = SgpioValueFromData(row, L"LOCATE", kSgpioLocateMask);
    const int fail_mask = SgpioValueFromData(row, L"FAIL", kSgpioFailMask);
    const int delay_ms = (std::max)(interval_ms * 3, ParseIntOrDefault(row.delay_ms, 300));

    service_->SgpioConfig(slots, clock_hz);
    service_->SgpioApply(true, true, interval_ms, sload_raw, act_mask, locate_mask, fail_mask);
    ::Sleep(static_cast<DWORD>((std::min)(2000, delay_ms)));
    AddResult(L"Step " + std::to_wstring(index + 1) + L" SGPIO",
              true,
              L"slots=" + std::to_wstring(slots) +
              L" clock=" + std::to_wstring(clock_hz) +
              L" ACT=" + HexWord(act_mask) +
              L" LOCATE=" + HexWord(locate_mask) +
              L" FAIL=" + HexWord(fail_mask) +
              (row.comment.empty() ? L"" : L" (" + row.comment + L")"));
    return true;
}

bool CTargetTestTab::CompareBytes(const std::vector<std::uint8_t>& rx,
                                const std::vector<std::uint8_t>& expected,
                                const std::vector<std::uint8_t>& mask) const {
    if (rx.size() < expected.size()) {
        return false;
    }
    for (size_t i = 0u; i < expected.size(); ++i) {
        const std::uint8_t m = mask.empty() ? 0xFFu : mask[(std::min)(i, mask.size() - 1u)];
        if ((rx[i] & m) != (expected[i] & m)) {
            return false;
        }
    }
    return true;
}

void CTargetTestTab::RefreshCommandList() {
    command_list_.DeleteAllItems();
    for (int i = 0; i < static_cast<int>(commands_.size()); ++i) {
        const CommandRow& row = commands_[static_cast<size_t>(i)];
        CString index_text;
        index_text.Format(L"%d", i + 1);
        command_list_.InsertItem(i, index_text);
        command_list_.SetItemText(i, 1, KindText(row.kind).c_str());
        command_list_.SetItemText(i, 2, row.reg.c_str());
        command_list_.SetItemText(i, 3, row.data.c_str());
        command_list_.SetItemText(i, 4, row.read_len.c_str());
        command_list_.SetItemText(i, 5, row.expect.c_str());
        command_list_.SetItemText(i, 6, row.mask.c_str());
        command_list_.SetItemText(i, 7, row.delay_ms.c_str());
        command_list_.SetItemText(i, 8, row.comment.c_str());
    }
    if (!commands_.empty()) {
        SelectCommandIndex((std::min)(SelectedCommandIndex(), static_cast<int>(commands_.size()) - 1));
    }
    UpdateEnableState();
}

void CTargetTestTab::LoadEditorFromSelectedRow() {
    const int index = SelectedCommandIndex();
    if (index < 0 || index >= static_cast<int>(commands_.size())) {
        return;
    }
    const CommandRow& row = commands_[static_cast<size_t>(index)];
    loading_editor_ = true;
    kind_combo_.SetCurSel(static_cast<int>(row.kind));
    reg_edit_.SetWindowTextW(row.reg.c_str());
    data_edit_.SetWindowTextW(row.data.c_str());
    read_len_edit_.SetWindowTextW(row.read_len.c_str());
    expect_edit_.SetWindowTextW(row.expect.c_str());
    mask_edit_.SetWindowTextW(row.mask.c_str());
    delay_edit_.SetWindowTextW(row.delay_ms.c_str());
    comment_edit_.SetWindowTextW(row.comment.c_str());
    loading_editor_ = false;
}

void CTargetTestTab::StoreEditorToSelectedRow() {
    const int index = SelectedCommandIndex();
    if (index < 0 || index >= static_cast<int>(commands_.size())) {
        return;
    }
    commands_[static_cast<size_t>(index)] = BuildRowFromEditor();
    RefreshCommandList();
    SelectCommandIndex(index);
}

CTargetTestTab::CommandRow CTargetTestTab::BuildRowFromEditor() const {
    CommandRow row;
    CString kind_text;
    kind_combo_.GetWindowTextW(kind_text);
    row.kind = KindFromText(kind_text.GetString());
    row.reg = GetEditText(reg_edit_);
    row.data = GetEditText(data_edit_);
    row.read_len = GetEditText(read_len_edit_);
    row.expect = GetEditText(expect_edit_);
    row.mask = GetEditText(mask_edit_);
    row.delay_ms = GetEditText(delay_edit_);
    row.comment = GetEditText(comment_edit_);
    return row;
}

int CTargetTestTab::SelectedCommandIndex() const {
    POSITION pos = command_list_.GetFirstSelectedItemPosition();
    if (pos == nullptr) {
        return -1;
    }
    return command_list_.GetNextSelectedItem(pos);
}

void CTargetTestTab::SelectCommandIndex(int index) {
    if (commands_.empty()) {
        return;
    }
    index = std::clamp(index, 0, static_cast<int>(commands_.size()) - 1);
    command_list_.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    command_list_.EnsureVisible(index, FALSE);
    LoadEditorFromSelectedRow();
}

void CTargetTestTab::AddDefaultSequenceRows() {
    auto add = [this](CommandKind kind, const wchar_t* reg, const wchar_t* data,
                      const wchar_t* read_len, const wchar_t* expect,
                      const wchar_t* mask, const wchar_t* delay_ms, const wchar_t* comment) {
        CommandRow row;
        row.kind = kind;
        row.reg = reg;
        row.data = data;
        row.read_len = read_len;
        row.expect = expect;
        row.mask = mask;
        row.delay_ms = delay_ms;
        row.comment = comment;
        commands_.push_back(row);
    };

    commands_.clear();
    add(CommandKind::Scan, L"", L"", L"0", L"", L"", L"20", L"scan 0x60..0x67 using read 0x30");
    add(CommandKind::Read, L"0x30", L"", L"1", L"20", L"F0", L"20", L"H/W config, Addr3/OEM/CONF");
    add(CommandKind::Read, L"0x60", L"", L"1", L"07", L"", L"20", L"FW version");
    add(CommandKind::Read, L"0x61", L"", L"1", L"0F", L"", L"20", L"FW major");
    add(CommandKind::Read, L"0x62", L"", L"1", L"0F", L"", L"20", L"FW minor");
    add(CommandKind::Read, L"0x63", L"", L"1", L"43", L"", L"20", L"Chip ID");
    add(CommandKind::Read, L"0x64", L"", L"1", L"01", L"", L"20", L"Chip revision");
    add(CommandKind::Read, L"0x3C", L"", L"1", L"00", L"", L"20", L"default SGPIO control");
    add(CommandKind::WriteRead, L"0x3C", L"FF", L"1", L"FF", L"", L"20", L"enable SMBus manual mode");
    add(CommandKind::WriteRead, L"0x42", L"01", L"1", L"01", L"", L"20", L"manual Locate slot 0");
    add(CommandKind::WriteRead, L"0x44", L"02", L"1", L"02", L"", L"20", L"manual Fail slot 1");
    add(CommandKind::WriteRead, L"0x46", L"04", L"1", L"04", L"", L"20", L"manual Rebuild slot 2");
    add(CommandKind::WriteRead, L"0x42", L"00", L"1", L"00", L"", L"20", L"clear Locate mask");
    add(CommandKind::WriteRead, L"0x44", L"00", L"1", L"00", L"", L"20", L"clear Fail mask");
    add(CommandKind::WriteRead, L"0x46", L"00", L"1", L"00", L"", L"20", L"clear Rebuild mask");
    add(CommandKind::Write, L"0x66", L"FF", L"0", L"", L"", L"120", L"lamp test L0..L7 on");
    add(CommandKind::Write, L"0x66", L"00", L"0", L"", L"", L"20", L"lamp test L0..L7 off");
    add(CommandKind::Write, L"0x67", L"FF", L"0", L"", L"", L"120", L"lamp test L8..L15 on");
    add(CommandKind::Write, L"0x67", L"00", L"0", L"", L"", L"20", L"lamp test L8..L15 off");
    add(CommandKind::WriteRead, L"0x3C", L"00", L"1", L"00", L"", L"20", L"return to SGPIO control");
    add(CommandKind::Sgpio, L"", L"ACT=0x0001 LOCATE=0x000A FAIL=0x000C SLOAD=0x0", L"0", L"", L"", L"300", L"slot0 ACT, slot1 LOC, slot2 FAIL, slot3 Rebuild");
    for (int slot = 0; slot < 8; ++slot) {
        wchar_t reg_buf[8] = {};
        wchar_t expect_buf[8] = {};
        swprintf_s(reg_buf, L"0x%02X", slot);
        swprintf_s(expect_buf, L"%02X", SlotExpectedLowBits(slot));
        add(CommandKind::Read, reg_buf, L"", L"1", expect_buf, L"07", L"20", L"slot readback low three bits");
    }
}

bool CTargetTestTab::LoadSequenceFromFile(const std::wstring& path, bool show_errors) {
    mfc_tool::config::IniManager ini(path);
    mfc_tool::config::IniData data;
    std::wstring error;
    int count = 0;
    std::vector<CommandRow> loaded;

    if (!ini.Exists() || !ini.Load(&data, &error)) {
        if (show_errors) {
            ::MessageBoxW(m_hWnd, (error.empty() ? L"Sequence INI not found or unreadable." : error).c_str(),
                          L"Load Sequence", MB_ICONERROR | MB_OK);
        }
        return false;
    }

    count = ParseIntOrDefault(GetIniValue(data, L"SEQUENCE", L"count", L"0"), 0);
    for (int i = 0; i < count; ++i) {
        wchar_t sec_buf[32] = {};
        CommandRow row;
        swprintf_s(sec_buf, L"STEP_%03d", i);
        row.kind = KindFromText(GetIniValue(data, sec_buf, L"kind", L"Read"));
        row.reg = GetIniValue(data, sec_buf, L"reg", L"");
        row.data = GetIniValue(data, sec_buf, L"data", L"");
        row.read_len = GetIniValue(data, sec_buf, L"read_len", L"1");
        row.expect = GetIniValue(data, sec_buf, L"expect", L"");
        row.mask = GetIniValue(data, sec_buf, L"mask", L"");
        row.delay_ms = GetIniValue(data, sec_buf, L"delay_ms", L"20");
        row.comment = GetIniValue(data, sec_buf, L"comment", L"");
        loaded.push_back(row);
    }

    if (loaded.empty()) {
        if (show_errors) {
            ::MessageBoxW(m_hWnd, L"Sequence INI contains no STEP rows.", L"Load Sequence", MB_ICONERROR | MB_OK);
        }
        return false;
    }

    commands_ = loaded;
    RefreshCommandList();
    seq_path_edit_.SetWindowTextW(path.c_str());
    if (log_) {
        log_(L"Loaded sequence INI: " + path);
    }
    return true;
}

bool CTargetTestTab::SaveSequenceToFile(const std::wstring& path, bool show_errors) const {
    mfc_tool::config::IniManager ini(path);
    mfc_tool::config::IniData data;
    data[L"SEQUENCE"] = {
        {L"name", L"SGPIO_SMBUS_Default"},
        {L"count", std::to_wstring(commands_.size())}
    };

    for (int i = 0; i < static_cast<int>(commands_.size()); ++i) {
        const CommandRow& row = commands_[static_cast<size_t>(i)];
        wchar_t sec_buf[32] = {};
        swprintf_s(sec_buf, L"STEP_%03d", i);
        data[sec_buf] = {
            {L"kind", KindText(row.kind)},
            {L"reg", row.reg},
            {L"data", row.data},
            {L"read_len", row.read_len},
            {L"expect", row.expect},
            {L"mask", row.mask},
            {L"delay_ms", row.delay_ms},
            {L"comment", row.comment}
        };
    }

    std::wstring error;
    if (!ini.Save(data, &error)) {
        if (show_errors) {
            ::MessageBoxW(m_hWnd, error.c_str(), L"Save Sequence", MB_ICONERROR | MB_OK);
        }
        return false;
    }
    if (log_) {
        log_(L"Saved sequence INI: " + path);
    }
    return true;
}

std::wstring CTargetTestTab::DefaultSequencePath() const {
    return mfc_tool::config::IniManager::DefaultIniPath(L"sequence_presets\\Target_Default.ini");
}

void CTargetTestTab::EnsureSequenceLoaded() {
    if (!commands_.empty()) {
        return;
    }
    if (!LoadSequenceFromFile(GetEditText(seq_path_edit_), false)) {
        AddDefaultSequenceRows();
        RefreshCommandList();
    }
}

int CTargetTestTab::CurrentAddress() const {
    return std::clamp(ParseEditInt(addr_edit_), 0x03, 0x77);
}

int CTargetTestTab::ParseEditInt(const CEdit& edit) const {
    CString s;
    const_cast<CEdit&>(edit).GetWindowTextW(s);
    return mfc_tool::core::ParseInt(s.GetString());
}

int CTargetTestTab::ParseIntOrDefault(const std::wstring& text, int fallback) const {
    try {
        return mfc_tool::core::ParseInt(text);
    } catch (...) {
        return fallback;
    }
}

std::wstring CTargetTestTab::GetEditText(const CEdit& edit) const {
    CString s;
    const_cast<CEdit&>(edit).GetWindowTextW(s);
    return s.GetString();
}

const mfc_tool::core::board_i2c::PinPair& CTargetTestTab::CurrentPinPair() const {
    const auto& pairs = mfc_tool::core::board_i2c::AllPinPairs();
    int sel = pins_combo_.GetCurSel();
    if (sel < 0 || sel >= static_cast<int>(pairs.size())) {
        sel = 0;
    }
    return pairs[static_cast<size_t>(sel)];
}

std::wstring CTargetTestTab::CurrentPinPairIniName() const {
    return CurrentPinPair().ini_name;
}

std::vector<std::uint8_t> CTargetTestTab::ParseByteList(const std::wstring& text) const {
    std::vector<std::uint8_t> out;
    std::wstring token;
    auto flush = [&]() {
        int value = 0;
        if (token.empty()) {
            return;
        }
        if (token.size() >= 2u && token[0] == L'0' && (token[1] == L'x' || token[1] == L'X')) {
            value = mfc_tool::core::ParseInt(token);
        } else {
            value = mfc_tool::core::ParseInt(L"0x" + token);
        }
        out.push_back(static_cast<std::uint8_t>(value & 0xFF));
        token.clear();
    };

    for (wchar_t ch : text) {
        if (iswspace(ch) || ch == L',' || ch == L';' || ch == L':' || ch == L'|' || ch == L'/') {
            flush();
        } else {
            token.push_back(ch);
        }
    }
    flush();

    if (!out.empty()) {
        return out;
    }
    return mfc_tool::core::ParseHexBytes(text);
}

int CTargetTestTab::SgpioValueFromData(const CommandRow& row, const std::wstring& key, int fallback) const {
    const std::wstring data = Upper(row.data);
    const std::wstring needle = Upper(key) + L"=";
    const size_t pos = data.find(needle);
    size_t end = std::wstring::npos;
    std::wstring value;
    if (pos == std::wstring::npos) {
        return fallback;
    }
    value = data.substr(pos + needle.size());
    end = value.find_first_of(L" ,;|");
    if (end != std::wstring::npos) {
        value = value.substr(0, end);
    }
    value = Trim(value);
    if (value.empty()) {
        return fallback;
    }
    return ParseIntOrDefault(value, fallback);
}

CTargetTestTab::CommandKind CTargetTestTab::KindFromText(const std::wstring& text) {
    const std::wstring t = Upper(Trim(text));
    if (t == L"SCAN") {
        return CommandKind::Scan;
    }
    if (t == L"WRITE") {
        return CommandKind::Write;
    }
    if (t == L"WRITEREAD" || t == L"WRITE_READ" || t == L"WRITE/READ") {
        return CommandKind::WriteRead;
    }
    if (t == L"SGPIO") {
        return CommandKind::Sgpio;
    }
    if (t == L"DELAY") {
        return CommandKind::Delay;
    }
    return CommandKind::Read;
}

std::wstring CTargetTestTab::KindText(CommandKind kind) {
    switch (kind) {
    case CommandKind::Scan:
        return L"Scan";
    case CommandKind::Read:
        return L"Read";
    case CommandKind::Write:
        return L"Write";
    case CommandKind::WriteRead:
        return L"WriteRead";
    case CommandKind::Sgpio:
        return L"SGPIO";
    case CommandKind::Delay:
        return L"Delay";
    default:
        break;
    }
    return L"Read";
}

std::wstring CTargetTestTab::HexByte(std::uint8_t v) {
    wchar_t buf[8] = {};
    swprintf_s(buf, L"0x%02X", static_cast<unsigned int>(v));
    return buf;
}

std::wstring CTargetTestTab::HexWord(int v) {
    wchar_t buf[12] = {};
    swprintf_s(buf, L"0x%02X", static_cast<unsigned int>(v & 0xFFFF));
    return buf;
}

std::wstring CTargetTestTab::HexBytes(const std::vector<std::uint8_t>& data) {
    if (data.empty()) {
        return L"<empty>";
    }
    std::wstringstream ss;
    ss << std::uppercase << std::hex << std::setfill(L'0');
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0u) {
            ss << L" ";
        }
        ss << L"0x" << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return ss.str();
}

std::wstring CTargetTestTab::AnsiToWide(const char* text) {
    if (text == nullptr) {
        return L"";
    }
    const int n = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    if (n <= 0) {
        return L"";
    }
    std::wstring out(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, out.data(), n);
    return out;
}

void CTargetTestTab::OnBtnInit() {
    if (!connected_ || service_ == nullptr) {
        return;
    }
    try {
        InitI2cFromUi();
    } catch (const std::exception& e) {
        SetStatusText(L"I2C init failed");
        AddResult(L"I2C init", false, AnsiToWide(e.what()));
        ::MessageBoxW(m_hWnd, AnsiToWide(e.what()).c_str(), L"I2C Init Error", MB_ICONERROR | MB_OK);
    }
}

void CTargetTestTab::OnBtnScan() {
    if (!connected_ || service_ == nullptr) {
        return;
    }
    try {
        if (!i2c_enabled_) {
            InitI2cFromUi();
        }
        ScanAddresses(true);
    } catch (const std::exception& e) {
        AddResult(L"Address scan", false, AnsiToWide(e.what()));
        ::MessageBoxW(m_hWnd, AnsiToWide(e.what()).c_str(), L"Scan Error", MB_ICONERROR | MB_OK);
    }
}

void CTargetTestTab::OnBtnRunAll() {
    if (!connected_ || service_ == nullptr) {
        return;
    }
    if (loop_running_) {
        StopSequenceLoop(L"Loop stopped");
        return;
    }
    try {
        loop_iteration_ = 1;
        RunSequence();
        if (loop_check_.GetCheck() == BST_CHECKED) {
            loop_running_ = true;
            ScheduleNextLoop();
            return;
        }
    } catch (const std::exception& e) {
        loop_running_ = false;
        SetStatusText(L"Sequence failed");
        AddResult(L"Run sequence", false, AnsiToWide(e.what()));
        ::MessageBoxW(m_hWnd, AnsiToWide(e.what()).c_str(), L"Run Sequence Error", MB_ICONERROR | MB_OK);
    }
    UpdateEnableState();
}

void CTargetTestTab::OnBtnDeinit() {
    if (!connected_ || service_ == nullptr) {
        return;
    }
    StopSequenceLoop(L"");
    try {
        service_->SgpioOff();
        service_->I2cDeinit(CurrentPinPair().i2c_port);
        i2c_enabled_ = false;
        SetStatusText(L"Deinitialized");
        AddResult(L"Deinit", true, L"I2C deinit and SGPIO off complete");
    } catch (const std::exception& e) {
        AddResult(L"Deinit", false, AnsiToWide(e.what()));
        ::MessageBoxW(m_hWnd, AnsiToWide(e.what()).c_str(), L"Deinit Error", MB_ICONERROR | MB_OK);
    }
    UpdateEnableState();
}

void CTargetTestTab::OnBtnClear() {
    ClearResults();
    ClearSlots();
    SetStatusText(L"Idle");
}

void CTargetTestTab::OnBtnLoadSequence() {
    const std::wstring path = GetEditText(seq_path_edit_);
    if (!LoadSequenceFromFile(path, true)) {
        return;
    }
    SetStatusText(L"Sequence loaded");
}

void CTargetTestTab::OnBtnSaveSequence() {
    const std::wstring path = GetEditText(seq_path_edit_);
    if (SaveSequenceToFile(path.empty() ? DefaultSequencePath() : path, true)) {
        SetStatusText(L"Sequence saved");
    }
}

void CTargetTestTab::OnBtnDefaultSequence() {
    AddDefaultSequenceRows();
    RefreshCommandList();
    seq_path_edit_.SetWindowTextW(DefaultSequencePath().c_str());
    SetStatusText(L"Default sequence restored");
}

void CTargetTestTab::OnBtnAddRow() {
    const int selected = SelectedCommandIndex();
    CommandRow row = BuildRowFromEditor();
    const int insert_at = (selected >= 0) ? selected + 1 : static_cast<int>(commands_.size());
    commands_.insert(commands_.begin() + insert_at, row);
    RefreshCommandList();
    SelectCommandIndex(insert_at);
}

void CTargetTestTab::OnBtnRemoveRow() {
    const int selected = SelectedCommandIndex();
    if (selected < 0 || selected >= static_cast<int>(commands_.size())) {
        return;
    }
    commands_.erase(commands_.begin() + selected);
    RefreshCommandList();
    if (!commands_.empty()) {
        SelectCommandIndex((std::min)(selected, static_cast<int>(commands_.size()) - 1));
    }
}

void CTargetTestTab::OnBtnUpdateRow() {
    StoreEditorToSelectedRow();
}

void CTargetTestTab::OnBtnMoveUp() {
    const int selected = SelectedCommandIndex();
    if (selected <= 0 || selected >= static_cast<int>(commands_.size())) {
        return;
    }
    std::swap(commands_[static_cast<size_t>(selected)], commands_[static_cast<size_t>(selected - 1)]);
    RefreshCommandList();
    SelectCommandIndex(selected - 1);
}

void CTargetTestTab::OnBtnMoveDown() {
    const int selected = SelectedCommandIndex();
    if (selected < 0 || selected + 1 >= static_cast<int>(commands_.size())) {
        return;
    }
    std::swap(commands_[static_cast<size_t>(selected)], commands_[static_cast<size_t>(selected + 1)]);
    RefreshCommandList();
    SelectCommandIndex(selected + 1);
}

void CTargetTestTab::OnCommandRowChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    const NM_LISTVIEW* nm = reinterpret_cast<const NM_LISTVIEW*>(pNMHDR);
    if (!loading_editor_ && nm != nullptr && (nm->uNewState & LVIS_SELECTED) != 0u) {
        LoadEditorFromSelectedRow();
    }
    UpdateEnableState();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}
