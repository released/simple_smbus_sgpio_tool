#include "sgpio_calc_tab.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <utility>

#include "../core/text_utils.h"
#include "layout_utils.h"

namespace {

enum : UINT {
    IDC_CALC_SETUP_GROUP = 15000,
    IDC_CALC_SLOT_COUNT_LABEL,
    IDC_CALC_SLOT_COUNT_EDIT,
    IDC_CALC_SLOAD_LABEL,
    IDC_CALC_SLOAD_EDIT,
    IDC_CALC_ACT_LABEL,
    IDC_CALC_ACT_EDIT,
    IDC_CALC_LOCATE_LABEL,
    IDC_CALC_LOCATE_EDIT,
    IDC_CALC_FAIL_LABEL,
    IDC_CALC_FAIL_EDIT,
    IDC_CALC_ROW_DATA_LABEL,
    IDC_CALC_ROW_DATA_EDIT,
    IDC_CALC_COPY_ROW_BTN,
    IDC_CALC_DEFAULT_PATTERN_BTN,
    IDC_CALC_CLEAR_ALL_BTN,
    IDC_CALC_GRID_GROUP,
    IDC_CALC_SLOT_HDR,
    IDC_CALC_ACT_HDR,
    IDC_CALC_LOCATE_HDR,
    IDC_CALC_FAIL_HDR,
    IDC_CALC_SLOT_LABEL_BASE = 15100,
    IDC_CALC_ACT_CHECK_BASE = 15200,
    IDC_CALC_LOCATE_CHECK_BASE = 15216,
    IDC_CALC_FAIL_CHECK_BASE = 15232,
};

constexpr int kSgpioCalcSlotMax = 16;

} // namespace

BEGIN_MESSAGE_MAP(CSgpioCalcTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_EN_CHANGE(IDC_CALC_SLOT_COUNT_EDIT, &CSgpioCalcTab::OnInputChanged)
    ON_EN_CHANGE(IDC_CALC_SLOAD_EDIT, &CSgpioCalcTab::OnInputChanged)
    ON_BN_CLICKED(IDC_CALC_DEFAULT_PATTERN_BTN, &CSgpioCalcTab::OnBtnDefaultPattern)
    ON_BN_CLICKED(IDC_CALC_CLEAR_ALL_BTN, &CSgpioCalcTab::OnBtnClearAll)
    ON_BN_CLICKED(IDC_CALC_COPY_ROW_BTN, &CSgpioCalcTab::OnBtnCopyRowData)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_CALC_ACT_CHECK_BASE, IDC_CALC_FAIL_CHECK_BASE + kSgpioCalcSlotMax - 1, &CSgpioCalcTab::OnMaskChanged)
END_MESSAGE_MAP()

BOOL CSgpioCalcTab::Create(CWnd* parent, const RECT& rect, UINT id) {
    CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return CWnd::CreateEx(0, cls, L"", WS_CHILD | WS_VISIBLE, rect, parent, id);
}

void CSgpioCalcTab::Bind(std::function<void(const std::wstring&)> logger) {
    log_ = std::move(logger);
}

int CSgpioCalcTab::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    int i = 0;

    if (CWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    mfc_tool::ui::CreatePointFontForWindow(ui_font_, *this, 85);
    mfc_tool::ui::CreatePointFontForWindow(slot_font_, *this, 80);

    auto mk_static = [this](CStatic& s, const wchar_t* text, UINT id) {
        s.Create(text, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, id);
        s.SetFont(&ui_font_);
    };
    auto mk_edit = [this](CEdit& e, const wchar_t* text, UINT id, DWORD extra_style = 0u) {
        e.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | extra_style,
                 CRect(0, 0, 0, 0), this, id);
        e.SetFont(&ui_font_);
        e.SetWindowTextW(text);
    };
    auto mk_btn = [this](CButton& b, const wchar_t* text, UINT id, DWORD style = BS_PUSHBUTTON) {
        b.Create(text, WS_CHILD | WS_VISIBLE | style, CRect(0, 0, 0, 0), this, id);
        b.SetFont(&ui_font_);
    };

    mk_btn(setup_group_, L"SGPIO Frame Calculator", IDC_CALC_SETUP_GROUP, BS_GROUPBOX);
    mk_static(slot_count_label_, L"Drive Count", IDC_CALC_SLOT_COUNT_LABEL);
    mk_edit(slot_count_edit_, L"8", IDC_CALC_SLOT_COUNT_EDIT);
    mk_static(sload_label_, L"SLOAD L0..L3", IDC_CALC_SLOAD_LABEL);
    mk_edit(sload_edit_, L"0x0", IDC_CALC_SLOAD_EDIT);
    mk_static(act_label_, L"ACT", IDC_CALC_ACT_LABEL);
    mk_edit(act_edit_, L"0x0001", IDC_CALC_ACT_EDIT, ES_READONLY);
    mk_static(locate_label_, L"LOCATE", IDC_CALC_LOCATE_LABEL);
    mk_edit(locate_edit_, L"0x000A", IDC_CALC_LOCATE_EDIT, ES_READONLY);
    mk_static(fail_label_, L"FAIL", IDC_CALC_FAIL_LABEL);
    mk_edit(fail_edit_, L"0x000C", IDC_CALC_FAIL_EDIT, ES_READONLY);
    mk_static(row_data_label_, L"SGPIO Row Data", IDC_CALC_ROW_DATA_LABEL);
    mk_edit(row_data_edit_, L"", IDC_CALC_ROW_DATA_EDIT, ES_READONLY);
    mk_btn(copy_row_btn_, L"Copy Row", IDC_CALC_COPY_ROW_BTN);
    mk_btn(default_pattern_btn_, L"Default Pattern", IDC_CALC_DEFAULT_PATTERN_BTN);
    mk_btn(clear_all_btn_, L"Clear All", IDC_CALC_CLEAR_ALL_BTN);

    mk_btn(grid_group_, L"Drive Bit Selection", IDC_CALC_GRID_GROUP, BS_GROUPBOX);
    mk_static(slot_hdr_, L"Drive", IDC_CALC_SLOT_HDR);
    mk_static(act_hdr_, L"ACT", IDC_CALC_ACT_HDR);
    mk_static(locate_hdr_, L"LOCATE", IDC_CALC_LOCATE_HDR);
    mk_static(fail_hdr_, L"FAIL", IDC_CALC_FAIL_HDR);
    slot_hdr_.SetFont(&slot_font_);
    act_hdr_.SetFont(&slot_font_);
    locate_hdr_.SetFont(&slot_font_);
    fail_hdr_.SetFont(&slot_font_);
    act_hdr_.ModifyStyle(0, SS_CENTER);
    locate_hdr_.ModifyStyle(0, SS_CENTER);
    fail_hdr_.ModifyStyle(0, SS_CENTER);

    for (i = 0; i < kSgpioCalcSlotMax; ++i) {
        wchar_t slot_text[8] = {};
        swprintf_s(slot_text, L"%d", i);
        slot_labels_[i].Create(slot_text, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_CALC_SLOT_LABEL_BASE + i);
        slot_labels_[i].SetFont(&slot_font_);
        act_checks_[i].Create(L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CALC_ACT_CHECK_BASE + i);
        locate_checks_[i].Create(L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CALC_LOCATE_CHECK_BASE + i);
        fail_checks_[i].Create(L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CALC_FAIL_CHECK_BASE + i);
        act_checks_[i].SetFont(&ui_font_);
        locate_checks_[i].SetFont(&ui_font_);
        fail_checks_[i].SetFont(&ui_font_);
    }

    ApplyMasksToChecks(0x0001, 0x000A, 0x000C);
    UpdateOutputs();
    return 0;
}

void CSgpioCalcTab::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    CRect rc;
    GetClientRect(&rc);
    LayoutControls(rc);
}

void CSgpioCalcTab::RefreshDpiLayout() {
    if (!::IsWindow(GetSafeHwnd())) {
        return;
    }

    mfc_tool::ui::CreatePointFontForWindow(ui_font_, *this, 85);
    mfc_tool::ui::CreatePointFontForWindow(slot_font_, *this, 80);
    mfc_tool::ui::ApplyFontToChildWindows(*this, ui_font_, FALSE);
    slot_hdr_.SetFont(&slot_font_, FALSE);
    act_hdr_.SetFont(&slot_font_, FALSE);
    locate_hdr_.SetFont(&slot_font_, FALSE);
    fail_hdr_.SetFont(&slot_font_, FALSE);
    for (int i = 0; i < kSgpioCalcSlotMax; ++i) {
        slot_labels_[i].SetFont(&slot_font_, FALSE);
    }

    CRect rc;
    GetClientRect(&rc);
    LayoutControls(rc);
    Invalidate(FALSE);
}

void CSgpioCalcTab::LoadState(const mfc_tool::core::AppState& state) {
    slot_count_edit_.SetWindowTextW(state.sgpio_calc.slot_count.c_str());
    sload_edit_.SetWindowTextW(state.sgpio_calc.sload_raw.c_str());
    ApplyMasksToChecks(mfc_tool::core::ParseInt(state.sgpio_calc.act_mask),
                       mfc_tool::core::ParseInt(state.sgpio_calc.locate_mask),
                       mfc_tool::core::ParseInt(state.sgpio_calc.fail_mask));
    UpdateOutputs();
}

void CSgpioCalcTab::SaveState(mfc_tool::core::AppState* state) const {
    int act_mask = 0;
    int locate_mask = 0;
    int fail_mask = 0;
    if (state == nullptr) {
        return;
    }
    BuildMasksFromChecks(&act_mask, &locate_mask, &fail_mask);
    state->sgpio_calc.slot_count = GetEditText(slot_count_edit_);
    state->sgpio_calc.sload_raw = GetEditText(sload_edit_);
    state->sgpio_calc.act_mask = HexWord(act_mask);
    state->sgpio_calc.locate_mask = HexWord(locate_mask);
    state->sgpio_calc.fail_mask = HexWord(fail_mask);
}

void CSgpioCalcTab::LayoutControls(const CRect& r) {
    if (!::IsWindow(setup_group_.GetSafeHwnd()) || r.Width() <= 0 || r.Height() <= 0) {
        return;
    }

    const mfc_tool::ui::DpiScaler dpi = mfc_tool::ui::DpiScaler::FromWindow(*this);
    const mfc_tool::ui::LayoutMetrics metrics = mfc_tool::ui::MetricsForWindow(*this);
    const int margin = metrics.margin8;
    const int gap = metrics.gap;
    const int row = metrics.row26;
    const int label_h = metrics.label18;
    const int y_pad = dpi.Scale(4);
    const int label_pad = dpi.Scale(8);
    const int setup_h = dpi.Scale(118);
    int x = r.left + margin + dpi.Scale(12);
    int y = r.top + margin + dpi.Scale(28);

    setup_group_.MoveWindow(r.left + margin, r.top + margin, r.Width() - margin * 2, setup_h);

    x = mfc_tool::ui::PlaceLabelAndControl(slot_count_label_, slot_count_edit_, x, y + y_pad, y,
                                           dpi.Scale(52), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(sload_label_, sload_edit_, x, y + y_pad, y,
                                           dpi.Scale(70), row, gap, label_pad, label_h) + dpi.Scale(16);
    x = mfc_tool::ui::PlaceLabelAndControl(act_label_, act_edit_, x, y + y_pad, y,
                                           dpi.Scale(84), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(locate_label_, locate_edit_, x, y + y_pad, y,
                                           dpi.Scale(84), row, gap, label_pad, label_h) + gap;
    x = mfc_tool::ui::PlaceLabelAndControl(fail_label_, fail_edit_, x, y + y_pad, y,
                                           dpi.Scale(84), row, gap, label_pad, label_h);

    y += row + gap;
    x = r.left + margin + dpi.Scale(12);
    x = mfc_tool::ui::PlaceLabelAndControl(row_data_label_, row_data_edit_, x, y + y_pad, y,
                                           (std::max)(dpi.Scale(420), r.Width() - dpi.Scale(550)),
                                           row, gap, label_pad, label_h) + gap;
    copy_row_btn_.MoveWindow(x, y, dpi.Scale(86), row);
    x += dpi.Scale(86) + gap;
    default_pattern_btn_.MoveWindow(x, y, dpi.Scale(122), row);
    x += dpi.Scale(122) + gap;
    clear_all_btn_.MoveWindow(x, y, dpi.Scale(82), row);

    y = r.top + margin + setup_h + gap;
    grid_group_.MoveWindow(r.left + margin, y, r.Width() - margin * 2, r.bottom - y - margin);
    LayoutSlotGrid(r.left + margin + dpi.Scale(14), y + dpi.Scale(28),
                   r.right - margin - dpi.Scale(14), r.bottom - margin - dpi.Scale(12));
}

void CSgpioCalcTab::LayoutSlotGrid(int left, int top, int right, int bottom) {
    const mfc_tool::ui::DpiScaler dpi = mfc_tool::ui::DpiScaler::FromWindow(*this);
    const mfc_tool::ui::LayoutMetrics metrics = mfc_tool::ui::MetricsForWindow(*this);
    const int gap = metrics.gap;
    const int min_row = dpi.Scale(18);
    const int rows_per_col = kSgpioCalcSlotMax / 2;
    const int header_h = dpi.Scale(16);
    const int check_size = dpi.Scale(16);
    const int col1_w = dpi.Scale(44);
    const int col_w = dpi.Scale(64);
    const int block_w = col1_w + gap + col_w + gap + col_w + gap + col_w + dpi.Scale(12);
    const int avail_w = (std::max)(0, right - left);
    const int remain_w = (std::max)(0, avail_w - block_w * 2);
    int gap_each = dpi.Scale(18);
    int row_step = 0;
    int row_base_y = 0;
    int first_col_x = 0;
    int second_col_x = 0;
    int i = 0;

    if (block_w * 2 + gap_each > avail_w) {
        gap_each = (std::max)(gap, remain_w);
    }
    first_col_x = left;
    second_col_x = first_col_x + block_w + gap_each;
    if (second_col_x + block_w > right) {
        second_col_x = right - block_w;
        first_col_x = (std::max)(left, second_col_x - gap_each - block_w);
    }

    row_base_y = top + header_h + dpi.Scale(3);
    row_step = (bottom - row_base_y - dpi.Scale(4)) / rows_per_col;
    row_step = (std::max)(min_row, row_step);
    row_step = (std::min)(dpi.Scale(24), row_step);

    mfc_tool::ui::SafeMoveWindow(slot_hdr_, first_col_x, top, col1_w, header_h);
    mfc_tool::ui::SafeMoveWindow(act_hdr_, first_col_x + col1_w + gap, top, col_w, header_h);
    mfc_tool::ui::SafeMoveWindow(locate_hdr_, first_col_x + col1_w + gap + col_w + gap, top, col_w, header_h);
    mfc_tool::ui::SafeMoveWindow(fail_hdr_, first_col_x + col1_w + gap + col_w + gap + col_w + gap, top, col_w, header_h);

    for (i = 0; i < kSgpioCalcSlotMax; ++i) {
        const bool left_block = i < rows_per_col;
        const int local_index = left_block ? i : (i - rows_per_col);
        const int base_x = left_block ? first_col_x : second_col_x;
        const int row_y = row_base_y + local_index * row_step;
        const int act_x = base_x + col1_w + gap;
        const int locate_x = act_x + col_w + gap;
        const int fail_x = locate_x + col_w + gap;

        slot_labels_[i].ShowWindow(SW_SHOW);
        act_checks_[i].ShowWindow(SW_SHOW);
        locate_checks_[i].ShowWindow(SW_SHOW);
        fail_checks_[i].ShowWindow(SW_SHOW);
        slot_labels_[i].MoveWindow(base_x, row_y + dpi.Scale(1), col1_w, header_h);
        act_checks_[i].MoveWindow(act_x + (col_w - check_size) / 2, row_y, check_size, check_size);
        locate_checks_[i].MoveWindow(locate_x + (col_w - check_size) / 2, row_y, check_size, check_size);
        fail_checks_[i].MoveWindow(fail_x + (col_w - check_size) / 2, row_y, check_size, check_size);
    }

    UpdateSlotEnableState();
}

void CSgpioCalcTab::UpdateOutputs() {
    int act_mask = 0;
    int locate_mask = 0;
    int fail_mask = 0;

    if (updating_ || !::IsWindow(row_data_edit_.GetSafeHwnd())) {
        return;
    }

    updating_ = true;
    UpdateSlotEnableState();
    BuildMasksFromChecks(&act_mask, &locate_mask, &fail_mask);
    act_edit_.SetWindowTextW(HexWord(act_mask).c_str());
    locate_edit_.SetWindowTextW(HexWord(locate_mask).c_str());
    fail_edit_.SetWindowTextW(HexWord(fail_mask).c_str());
    row_data_edit_.SetWindowTextW(BuildRowDataText().c_str());
    updating_ = false;
}

void CSgpioCalcTab::UpdateSlotEnableState() {
    const int slots = EffectiveSlotCount();
    int i = 0;

    for (i = 0; i < kSgpioCalcSlotMax; ++i) {
        const BOOL enabled = i < slots ? TRUE : FALSE;
        mfc_tool::ui::SafeEnableWindow(slot_labels_[i], enabled);
        mfc_tool::ui::SafeEnableWindow(act_checks_[i], enabled);
        mfc_tool::ui::SafeEnableWindow(locate_checks_[i], enabled);
        mfc_tool::ui::SafeEnableWindow(fail_checks_[i], enabled);
    }
}

void CSgpioCalcTab::ApplyMasksToChecks(int act_mask, int locate_mask, int fail_mask) {
    int i = 0;
    for (i = 0; i < kSgpioCalcSlotMax; ++i) {
        act_checks_[i].SetCheck((act_mask & (1 << i)) ? BST_CHECKED : BST_UNCHECKED);
        locate_checks_[i].SetCheck((locate_mask & (1 << i)) ? BST_CHECKED : BST_UNCHECKED);
        fail_checks_[i].SetCheck((fail_mask & (1 << i)) ? BST_CHECKED : BST_UNCHECKED);
    }
}

void CSgpioCalcTab::BuildMasksFromChecks(int* act_mask, int* locate_mask, int* fail_mask) const {
    const int slots = EffectiveSlotCount();
    int act = 0;
    int locate = 0;
    int fail = 0;
    int i = 0;

    for (i = 0; i < slots; ++i) {
        if (act_checks_[i].GetCheck() == BST_CHECKED) {
            act |= (1 << i);
        }
        if (locate_checks_[i].GetCheck() == BST_CHECKED) {
            locate |= (1 << i);
        }
        if (fail_checks_[i].GetCheck() == BST_CHECKED) {
            fail |= (1 << i);
        }
    }

    if (act_mask != nullptr) {
        *act_mask = act;
    }
    if (locate_mask != nullptr) {
        *locate_mask = locate;
    }
    if (fail_mask != nullptr) {
        *fail_mask = fail;
    }
}

int CSgpioCalcTab::EffectiveSlotCount() const {
    int slots = ParseEditInt(slot_count_edit_, 8);
    slots = (std::max)(1, slots);
    slots = (std::min)(kSgpioCalcSlotMax, slots);
    return slots;
}

int CSgpioCalcTab::ParseEditInt(const CEdit& edit, int fallback) const {
    try {
        return mfc_tool::core::ParseInt(GetEditText(edit));
    } catch (...) {
        return fallback;
    }
}

std::wstring CSgpioCalcTab::GetEditText(const CEdit& edit) const {
    CString s;
    const_cast<CEdit&>(edit).GetWindowTextW(s);
    return s.GetString();
}

std::wstring CSgpioCalcTab::BuildRowDataText() const {
    int act_mask = 0;
    int locate_mask = 0;
    int fail_mask = 0;
    const int sload_raw = ParseEditInt(sload_edit_, 0);
    wchar_t buf[128] = {};

    BuildMasksFromChecks(&act_mask, &locate_mask, &fail_mask);
    swprintf_s(buf, L"ACT=%ls LOCATE=%ls FAIL=%ls SLOAD=0x%X",
               HexWord(act_mask).c_str(), HexWord(locate_mask).c_str(),
               HexWord(fail_mask).c_str(), sload_raw & 0x0F);
    return buf;
}

bool CSgpioCalcTab::CopyTextToClipboard(const std::wstring& text) const {
    const SIZE_T bytes = (text.size() + 1u) * sizeof(wchar_t);
    HGLOBAL handle = nullptr;
    void* ptr = nullptr;

    if (!::OpenClipboard(m_hWnd)) {
        return false;
    }
    if (!::EmptyClipboard()) {
        ::CloseClipboard();
        return false;
    }
    handle = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (handle == nullptr) {
        ::CloseClipboard();
        return false;
    }
    ptr = ::GlobalLock(handle);
    if (ptr == nullptr) {
        ::GlobalFree(handle);
        ::CloseClipboard();
        return false;
    }
    memcpy(ptr, text.c_str(), bytes);
    ::GlobalUnlock(handle);
    if (::SetClipboardData(CF_UNICODETEXT, handle) == nullptr) {
        ::GlobalFree(handle);
        ::CloseClipboard();
        return false;
    }
    ::CloseClipboard();
    return true;
}

std::wstring CSgpioCalcTab::HexWord(int value) {
    wchar_t buf[16] = {};
    swprintf_s(buf, L"0x%04X", value & 0xFFFF);
    return buf;
}

void CSgpioCalcTab::OnInputChanged() {
    UpdateOutputs();
}

void CSgpioCalcTab::OnMaskChanged(UINT id) {
    (void)id;
    UpdateOutputs();
}

void CSgpioCalcTab::OnBtnDefaultPattern() {
    ApplyMasksToChecks(0x0001, 0x000A, 0x000C);
    UpdateOutputs();
}

void CSgpioCalcTab::OnBtnClearAll() {
    ApplyMasksToChecks(0, 0, 0);
    UpdateOutputs();
}

void CSgpioCalcTab::OnBtnCopyRowData() {
    const std::wstring text = BuildRowDataText();
    if (CopyTextToClipboard(text)) {
        row_data_edit_.SetWindowTextW(text.c_str());
        if (log_) {
            log_(L"SGPIO calculator copied row data: " + text);
        }
    } else {
        ::MessageBoxW(m_hWnd, L"Clipboard is not available.", L"Copy Row", MB_ICONERROR | MB_OK);
    }
}
