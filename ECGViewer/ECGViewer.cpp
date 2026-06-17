// ECGViewer.cpp — MFC 应用程序类实现
//
// 对应课堂知识点：
//   MFC 图形基础 — CWinApp 派生、InitInstance 注册文档模板
//   框架窗口      — 创建 SDI 主框架，设置标题、菜单

#include "stdafx.h"
#include "ECGViewer.h"
#include "MainFrm.h"
#include "ECGViewerDoc.h"
#include "ECGViewerView.h"
#include <gdiplus.h>

// ---- 全局对象 ----
CECGViewerApp theApp;

// ---- 消息映射 ----
BEGIN_MESSAGE_MAP(CECGViewerApp, CWinApp)
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

// ---- 构造/析构 ----
CECGViewerApp::CECGViewerApp()
    : m_gdiplusToken(0)
{
    // 启用重启管理器支持（Vista+）
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
}

CECGViewerApp::~CECGViewerApp()
{
}

// ---- 初始化实例 ----
BOOL CECGViewerApp::InitInstance()
{
    // 初始化公共控件
    INITCOMMONCONTROLSEX initCtrls;
    initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initCtrls.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&initCtrls);

    // 初始化 MFC
    if (!CWinApp::InitInstance())
        return FALSE;

    // 初始化 GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartup;
    if (Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartup, nullptr) != Gdiplus::Ok)
    {
        m_gdiplusToken = 0;
    }

    // Enable visual styles (use default MFC visual manager)
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));

    // 设置应用程序标题
    SetRegistryKey(_T("ECGViewer"));

    // 注册文档模板（Doc/View 架构）
    RegisterDocTemplate();

    // Parse command line (supports drag-and-drop file open)
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Create main frame window
    CFrameWnd* pFrame = new CMainFrame;
    if (!pFrame)
        return FALSE;

    m_pMainWnd = pFrame;

    // Load frame (menu, toolbar, etc.)
    if (!pFrame->LoadFrame(IDR_MAINFRAME,
        WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
        nullptr, nullptr))
    {
        return FALSE;
    }

    // If a file is specified on command line, open it.
    // Otherwise skip ProcessShellCommand to avoid "create empty document" failure,
    // since our app starts with an empty view showing "please open a file".
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
    {
        // No file specified — just show the main window directly.
        // OnDraw will display the "please open a file" prompt.
    }
    else
    {
        // File specified (e.g. drag-and-drop) — open it
        if (!ProcessShellCommand(cmdInfo))
        {
            pFrame->DestroyWindow();
            return FALSE;
        }
    }

    // Show and update window
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();

    return TRUE;
}

// ---- 清理 ----
int CECGViewerApp::ExitInstance()
{
    // 关闭 GDI+
    if (m_gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    return CWinApp::ExitInstance();
}

// ---- 注册文档模板 ----
void CECGViewerApp::RegisterDocTemplate()
{
    // SDI 模板：文档 + 框架 + 视图
    CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CECGViewerDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CECGViewerView)
    );
    if (pDocTemplate)
        AddDocTemplate(pDocTemplate);
}
