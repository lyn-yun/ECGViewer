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

    // 标准 MFC SDI 模式：由 ProcessShellCommand 统一创建文档和框架
    //   FileNew  → OnFileNew()  → 创建空文档 + 框架（OnDraw 显示"请打开文件"）
    //   FileOpen → OnFileOpen() → 打开指定文件 + 框架
    // 不要手动 new CMainFrame + LoadFrame，否则 CCreateContext 断开，
    // View 的 m_pDocument 为空，导致"创建空文档失败"。
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // m_pMainWnd 已由 ProcessShellCommand 设置
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

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
