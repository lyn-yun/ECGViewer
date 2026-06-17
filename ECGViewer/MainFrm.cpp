// MainFrm.cpp — 主框架窗口实现
#include "stdafx.h"
#include "MainFrm.h"
#include "Resource.h"
#include "ECGViewerView.h"

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;

    cs.style &= ~FWS_ADDTOTITLE;  // 不自动追加文档名（我们自己控制标题）
    cs.style |= WS_CLIPCHILDREN;  // 减少闪烁

    return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // 创建工具栏
    if (!CreateToolBar())
    {
        TRACE0("Failed to create toolbar\n");
        return -1;
    }

    // 创建状态栏
    if (!CreateStatusBar())
    {
        TRACE0("Failed to create status bar\n");
        return -1;
    }

    SetWindowText(_T("ECG Viewer — 心电信号波形显示与交互分析系统"));

    return 0;
}

BOOL CMainFrame::CreateToolBar()
{
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
        WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC))
    {
        return FALSE;
    }

    // 使用 SetButtons 创建工具栏按钮（不使用位图，MFC 会自动生成文字按钮）
    // 每个按钮映射到菜单中已有的命令 ID
    static const UINT buttons[] = {
        ID_FILE_OPEN,
        IDM_VIEW_ZOOM_IN,
        IDM_VIEW_ZOOM_OUT,
        IDM_VIEW_ZOOM_FIT,
        IDM_TOOLS_FILTER,
        IDM_TOOLS_DETECT_R,
    };

    if (!m_wndToolBar.SetButtons(buttons, _countof(buttons)))
        return FALSE;

    // 设置每个按钮的文字标签
    struct { UINT id; LPCTSTR text; } labels[] = {
        { ID_FILE_OPEN,       _T("打开\0")      },
        { IDM_VIEW_ZOOM_IN,   _T("放大\0")      },
        { IDM_VIEW_ZOOM_OUT,  _T("缩小\0")      },
        { IDM_VIEW_ZOOM_FIT,  _T("适应\0")      },
        { IDM_TOOLS_FILTER,   _T("滤波\0")      },
        { IDM_TOOLS_DETECT_R, _T("R波\0")       },
    };
    for (int i = 0; i < _countof(labels); ++i)
    {
        int idx = m_wndToolBar.CommandToIndex(labels[i].id);
        if (idx >= 0)
            m_wndToolBar.SetButtonText(idx, labels[i].text);
    }

    // 设置按钮大小（纯文字模式）
    m_wndToolBar.SetSizes(CSize(50, 30), CSize(24, 24));

    // 美化工具栏标题
    m_wndToolBar.SetWindowText(_T("主工具栏"));

    // 启用停靠
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    return TRUE;
}

BOOL CMainFrame::CreateStatusBar()
{
    if (!m_wndStatusBar.Create(this))
        return FALSE;

    // 状态栏指示器：就绪 | 鼠标位置 | 心率 | Caps/Num/Scroll
    static UINT indicators[] = {
        ID_SEPARATOR,           // 0: 状态信息
        IDS_CURSOR_POS_FMT,     // 1: 光标位置 (时间, mV)
        IDS_HEART_RATE_FMT,     // 2: 心率 (BPM)
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_SCRL,
    };

    m_wndStatusBar.SetIndicators(indicators, _countof(indicators));

    // 设置各个窗格的默认宽度
    m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 300);
    m_wndStatusBar.SetPaneInfo(1, IDS_CURSOR_POS_FMT, SBPS_NORMAL, 220);
    m_wndStatusBar.SetPaneInfo(2, IDS_HEART_RATE_FMT, SBPS_NORMAL, 130);

    // 初始状态文本
    m_wndStatusBar.SetPaneText(0, _T("就绪 — 请打开心电数据文件"));
    m_wndStatusBar.SetPaneText(1, _T("位置: --"));
    m_wndStatusBar.SetPaneText(2, _T("HR: -- BPM"));

    return TRUE;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // 使用默认的 Doc/View 客户端创建
    return CFrameWnd::OnCreateClient(lpcs, pContext);
}

void CMainFrame::OnDestroy()
{
    CFrameWnd::OnDestroy();
}

void CMainFrame::UpdateStatusBar(const CString& text, int paneIndex)
{
    if (m_wndStatusBar.GetSafeHwnd())
        m_wndStatusBar.SetPaneText(paneIndex, text);
}

void CMainFrame::UpdateCursorPosition(double timeSec, double amplitude)
{
    CString str;
    str.Format(_T("位置: %.3fs, %.3f mV"), timeSec, amplitude);
    UpdateStatusBar(str, 1);
}

void CMainFrame::UpdateHeartRate(double bpm)
{
    CString str;
    if (bpm > 0)
        str.Format(_T("HR: %.0f BPM"), bpm);
    else
        str = _T("HR: -- BPM");
    UpdateStatusBar(str, 2);
}
