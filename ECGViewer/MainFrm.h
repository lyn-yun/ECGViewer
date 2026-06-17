// MainFrm.h — 主框架窗口类
//
// 对应课堂知识：
//   MFC 图形基础 — CFrameWnd 派生，管理菜单/工具栏/状态栏
//   控件           — CToolBar 工具栏按钮、CStatusBar 状态栏显示信息

#pragma once

class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();
    virtual ~CMainFrame();

    // ---- 覆盖 ----
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

    // ---- 状态栏更新 ----
    void UpdateStatusBar(const CString& text, int paneIndex = 0);
    void UpdateCursorPosition(double timeSec, double amplitude);
    void UpdateHeartRate(double bpm);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()
    DECLARE_DYNAMIC(CMainFrame)

private:
    CToolBar    m_wndToolBar;
    CStatusBar  m_wndStatusBar;

    BOOL CreateToolBar();
    BOOL CreateStatusBar();
};
