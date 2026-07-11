#pragma once

#include <afxwin.h>

#include <array>
#include <functional>
#include <string>

#include "../core/app_state.h"

class CSgpioCalcTab : public CWnd {
public:
    BOOL Create(CWnd* parent, const RECT& rect, UINT id);

    void Bind(std::function<void(const std::wstring&)> logger);
    void RefreshDpiLayout();

    void LoadState(const mfc_tool::core::AppState& state);
    void SaveState(mfc_tool::core::AppState* state) const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnInputChanged();
    afx_msg void OnMaskChanged(UINT id);
    afx_msg void OnBtnDefaultPattern();
    afx_msg void OnBtnClearAll();
    afx_msg void OnBtnCopyRowData();
    DECLARE_MESSAGE_MAP()

private:
    void LayoutControls(const CRect& r);
    void LayoutSlotGrid(int left, int top, int right, int bottom);
    void UpdateOutputs();
    void UpdateSlotEnableState();
    void ApplyMasksToChecks(int act_mask, int locate_mask, int fail_mask);
    void BuildMasksFromChecks(int* act_mask, int* locate_mask, int* fail_mask) const;
    int EffectiveSlotCount() const;
    int ParseEditInt(const CEdit& edit, int fallback) const;
    std::wstring GetEditText(const CEdit& edit) const;
    std::wstring BuildRowDataText() const;
    bool CopyTextToClipboard(const std::wstring& text) const;
    static std::wstring HexWord(int value);

private:
    std::function<void(const std::wstring&)> log_;
    bool updating_ = false;

    CFont ui_font_;
    CFont slot_font_;

    CButton setup_group_;
    CStatic slot_count_label_;
    CEdit slot_count_edit_;
    CStatic sload_label_;
    CEdit sload_edit_;
    CStatic act_label_;
    CEdit act_edit_;
    CStatic locate_label_;
    CEdit locate_edit_;
    CStatic fail_label_;
    CEdit fail_edit_;
    CStatic row_data_label_;
    CEdit row_data_edit_;
    CButton copy_row_btn_;
    CButton default_pattern_btn_;
    CButton clear_all_btn_;

    CButton grid_group_;
    CStatic slot_hdr_;
    CStatic act_hdr_;
    CStatic locate_hdr_;
    CStatic fail_hdr_;
    std::array<CStatic, 16> slot_labels_;
    std::array<CButton, 16> act_checks_;
    std::array<CButton, 16> locate_checks_;
    std::array<CButton, 16> fail_checks_;
};
