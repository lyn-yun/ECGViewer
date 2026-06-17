// ECGViewerView.cpp — 视图类实现
//
// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  课堂知识全覆盖：                                                        ║
// ║    • GDI 图形    — CDC 绘制波形曲线(LineTo/Polyline)、网格、坐标轴      ║
// ║                    CPen 不同颜色/线宽区分导联和信号                      ║
// ║                    CBrush 填充背景                                       ║
// ║                    CFont/TextOut 绘制坐标标签                            ║
// ║    • 鼠标        — OnMouseMove 实时跟踪光标位置                          ║
// ║                    OnLButtonDown/Up 左键拖拽平移波形                     ║
// ║                    OnRButtonDown/Up 右键框选测量 RR 间期                 ║
// ║                    OnMouseWheel 滚轮缩放                                 ║
// ║    • 键盘        — OnKeyDown: 方向键平移, +/- 缩放, Space 播放动画      ║
// ║    • 动态链接库  — LoadLibrary 加载 ECGFilterDLL.dll                     ║
// ║                    GetProcAddress 获取滤波/R波检测函数                    ║
// ║    • 控件/状态栏 — 实时更新状态栏中的光标位置和心率信息                  ║
// ╚══════════════════════════════════════════════════════════════════════════╝

#include "stdafx.h"
#include "ECGViewer.h"
#include "ECGViewerView.h"
#include "ECGViewerDoc.h"
#include "MainFrm.h"
#include "Resource.h"

IMPLEMENT_DYNCREATE(CECGViewerView, CScrollView)

// ---- 消息映射 ----
BEGIN_MESSAGE_MAP(CECGViewerView, CScrollView)
    // 鼠标
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEWHEEL()
    // 键盘
    ON_WM_KEYDOWN()
    // 定时器
    ON_WM_TIMER()
    // 菜单命令
    ON_COMMAND(ID_FILE_OPEN,         OnFileOpen)
    ON_COMMAND(IDM_VIEW_ZOOM_IN,     OnViewZoomIn)
    ON_COMMAND(IDM_VIEW_ZOOM_OUT,    OnViewZoomOut)
    ON_COMMAND(IDM_VIEW_ZOOM_FIT,    OnViewZoomFit)
    ON_COMMAND(IDM_VIEW_ZOOM_RESET,  OnViewZoomReset)
    ON_COMMAND(IDM_VIEW_TOGGLE_GRID,  OnViewToggleGrid)
    ON_COMMAND(IDM_TOOLS_FILTER,     OnToolsFilter)
    ON_COMMAND(IDM_TOOLS_DETECT_R,   OnToolsDetectR)
END_MESSAGE_MAP()

// ============================================================================
// 构造 / 析构
// ============================================================================

CECGViewerView::CECGViewerView()
    : m_pixelsPerSecond(DEFAULT_PPS)
    , m_pixelsPerMillivolt(DEFAULT_PPMV)
    , m_zoomFactor(1.0)
    , m_marginLeft(60)
    , m_marginTop(15)
    , m_marginRight(30)
    , m_marginBottom(40)
    , m_bMouseLDown(false)
    , m_bMouseRDown(false)
    , m_bHasSelection(false)
    , m_ptLastMouse(0, 0)
    , m_ptRStart(0, 0)
    , m_ptREnd(0, 0)
    , m_bPlaying(false)
    , m_playStartTime(0.0)
    , m_bShowGrid(true)
    , m_bShowFiltered(false)
    , m_bShowRPeaks(false)
    , m_colorRaw(RGB(30, 120, 210))         // 蓝色 — 原始信号
    , m_colorFiltered(RGB(210, 40, 40))     // 红色 — 滤波后
    , m_colorRPeak(RGB(220, 80, 0))         // 橙色 — R波标记
    , m_colorGrid(RGB(220, 220, 225))       // 浅灰 — 网格
    , m_colorBg(RGB(248, 248, 252))         // 近白 — 背景
{
}

CECGViewerView::~CECGViewerView()
{
    if (m_bPlaying)
        KillTimer(1);
}

// ============================================================================
// 初始化更新
// ============================================================================

void CECGViewerView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();
    UpdateScrollSizes();
}

void CECGViewerView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pHint);

    CECGViewerDoc* pDoc = GetDocument();
    if (pDoc && pDoc->HasData())
    {
        UpdateScrollSizes();
    }

    if (lHint == 1)  // 滤波完成
    {
        m_bShowFiltered = true;
    }
    else if (lHint == 2)  // R波检测完成
    {
        m_bShowRPeaks = true;
    }

    Invalidate();
}

// ============================================================================
// GDI 绘图 — 核心绘制函数
// ============================================================================

void CECGViewerView::OnDraw(CDC* pDC)
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData())
    {
        // 无数据时显示提示文本
        CRect rc;
        GetClientRect(&rc);
        pDC->FillSolidRect(&rc, m_colorBg);

        CFont font;
        font.CreatePointFont(140, _T("微软雅黑"));
        CFont* pOldFont = pDC->SelectObject(&font);
        pDC->SetTextColor(RGB(120, 120, 140));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("请打开心电数据文件\n\n"
                          "文件 → 打开 或 按 Ctrl+O\n"
                          "支持 CSV 格式（时间,幅值）"),
                      &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        pDC->SelectObject(pOldFont);
        return;
    }

    // ---- 准备绘图 ----
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcPlot = GetPlotRect(rcClient);

    // 设置绘图模式（减少闪烁）
    pDC->SetMapMode(MM_TEXT);
    pDC->SetBkMode(TRANSPARENT);

    // 双缓冲：在内存DC中绘制，再一次性输出到屏幕
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(pDC, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);

    // ---- 1. 填充背景 ----
    memDC.FillSolidRect(&rcClient, m_colorBg);

    // ---- 2. 绘制网格 ----
    if (m_bShowGrid)
        DrawGrid(&memDC, rcClient);

    // ---- 3. 绘制坐标轴 ----
    DrawAxes(&memDC, rcClient);

    // ---- 4. 获取数据并绘制波形 ----
    const auto& data = pDoc->GetData();
    const auto& time = data.GetLead(0).time;
    const auto& signal = data.GetLead(0).signal;

    // 原始信号（灰色，半透明效果用浅色替代）
    if (!signal.empty())
    {
        DrawSignal(&memDC, time, signal, m_colorRaw, 1);
    }

    // 滤波后信号（红色）
    if (m_bShowFiltered && pDoc->HasFilteredData())
    {
        const auto& filtered = pDoc->GetFilteredSignal();
        if (!filtered.empty())
            DrawSignal(&memDC, time, filtered, m_colorFiltered, 2);
    }

    // ---- 5. 绘制 R 波标记 ----
    if (m_bShowRPeaks)
    {
        const auto& peaks = pDoc->GetRPeaks();
        if (!peaks.empty())
            DrawRPeaks(&memDC, time, peaks, 0);
    }

    // ---- 6. 绘制右键选区矩形 ----
    if (m_bHasSelection)
        DrawSelectionRect(&memDC);

    // ---- 7. 绘制播放头 ----
    if (m_bPlaying)
        DrawPlayhead(&memDC);

    // ---- 9. 裁剪并输出到屏幕 ----
    pDC->BitBlt(0, 0, rcClient.Width(), rcClient.Height(),
                &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBitmap);
}

// ---- 绘制网格 ----
void CECGViewerView::DrawGrid(CDC* pDC, const CRect& rcClient)
{
    CRect rcPlot = GetPlotRect(rcClient);
    CPen penGrid(PS_DOT, 1, m_colorGrid);
    CPen* pOldPen = pDC->SelectObject(&penGrid);

    // 获取可见范围
    CPoint ptTL = rcPlot.TopLeft();
    CPoint ptBR = rcPlot.BottomRight();

    double tMin, tMax, aMin, aMax;
    ScreenToData(ptTL, tMin, aMax);
    ScreenToData(ptBR, tMax, aMin);

    // 计算网格间距（自适应）
    double dt = 0.2;  // 每 200ms 一条竖线
    // 根据缩放级别调整
    if (m_pixelsPerSecond < 20)      dt = 1.0;
    else if (m_pixelsPerSecond < 50) dt = 0.5;
    else if (m_pixelsPerSecond < 200) dt = 0.2;
    else if (m_pixelsPerSecond < 500) dt = 0.1;
    else                             dt = 0.04;

    double dv = 0.5;  // 每 0.5 mV 一条横线
    if (m_pixelsPerMillivolt < 100)  dv = 1.0;
    else if (m_pixelsPerMillivolt > 500) dv = 0.2;

    // 竖线（时间网格）
    double tStart = floor(tMin / dt) * dt;
    for (double t = tStart; t <= tMax; t += dt)
    {
        CPoint pt = DataToScreen(t, 0);
        if (pt.x >= rcPlot.left && pt.x <= rcPlot.right)
        {
            pDC->MoveTo(pt.x, rcPlot.top);
            pDC->LineTo(pt.x, rcPlot.bottom);
        }
    }

    // 横线（幅值网格）
    double vStart = floor(aMin / dv) * dv;
    for (double v = vStart; v <= aMax; v += dv)
    {
        CPoint pt = DataToScreen(0, v);
        if (pt.y >= rcPlot.top && pt.y <= rcPlot.bottom)
        {
            pDC->MoveTo(rcPlot.left, pt.y);
            pDC->LineTo(rcPlot.right, pt.y);
        }
    }

    pDC->SelectObject(pOldPen);
}

// ---- 绘制波形曲线 ----
void CECGViewerView::DrawSignal(CDC* pDC, const std::vector<double>& time,
                                 const std::vector<double>& signal,
                                 COLORREF color, int lineWidth)
{
    if (time.empty() || signal.empty()) return;

    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcPlot = GetPlotRect(rcClient);

    // 创建画笔
    CPen pen(PS_SOLID, lineWidth, color);
    CPen* pOldPen = pDC->SelectObject(&pen);

    // 计算可见范围
    CPoint ptTL = rcPlot.TopLeft();
    CPoint ptBR = rcPlot.BottomRight();
    double tMin, tMax, aMin, aMax;
    ScreenToData(ptTL, tMin, aMax);
    ScreenToData(ptBR, tMax, aMin);

    // 确定可见样本范围
    int n = (int)signal.size();
    int idxStart = 0, idxEnd = n - 1;

    // 二分查找起始位置
    {
        int lo = 0, hi = n - 1;
        while (lo < hi)
        {
            int mid = (lo + hi) / 2;
            if (time[mid] < tMin) lo = mid + 1;
            else hi = mid;
        }
        idxStart = (std::max)(lo - 1, 0);
    }
    {
        int lo = 0, hi = n - 1;
        while (lo < hi)
        {
            int mid = (lo + hi + 1) / 2;
            if (time[mid] > tMax) hi = mid - 1;
            else lo = mid;
        }
        idxEnd = (std::min)(lo + 1, n - 1);
    }

    // 连接线段绘制（比 Polyline 更灵活，可以裁剪到可见区域）
    pDC->MoveTo(DataToScreen(time[idxStart], signal[idxStart]));

    for (int i = idxStart + 1; i <= idxEnd; ++i)
    {
        CPoint pt = DataToScreen(time[i], signal[i]);

        // 只绘制在可视区域内的线段
        if (pt.x >= rcPlot.left - 1 && pt.x <= rcPlot.right + 1)
        {
            pDC->LineTo(pt);
        }
        else
        {
            pDC->MoveTo(pt);
        }
    }

    pDC->SelectObject(pOldPen);
}

// ---- 绘制 R 波峰值标记 ----
void CECGViewerView::DrawRPeaks(CDC* pDC, const std::vector<double>& time,
                                 const std::vector<int>& peaks, int leadIdx)
{
    if (time.empty() || peaks.empty()) return;

    const auto& signal = GetDocument()->GetData().GetLead(leadIdx).signal;

    CPen pen(PS_SOLID, 2, m_colorRPeak);
    CPen* pOldPen = pDC->SelectObject(&pen);

    CBrush brush(m_colorRPeak);
    CBrush* pOldBrush = pDC->SelectObject(&brush);

    CRect rcPlot = GetPlotRect(GetClientRectDPI());

    for (int idx : peaks)
    {
        if (idx < 0 || idx >= (int)signal.size()) continue;

        CPoint pt = DataToScreen(time[idx], signal[idx]);
        if (pt.x < rcPlot.left - 10 || pt.x > rcPlot.right + 10) continue;

        // 绘制小圆点标记
        pDC->Ellipse(pt.x - 4, pt.y - 4, pt.x + 4, pt.y + 4);

        // 绘制竖线标记
        pDC->MoveTo(pt.x, pt.y - 15);
        pDC->LineTo(pt.x, pt.y + 15);
    }

    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);
}

// ---- 绘制坐标轴 ----
void CECGViewerView::DrawAxes(CDC* pDC, const CRect& rcClient)
{
    CRect rcPlot = GetPlotRect(rcClient);

    // 坐标轴颜色
    CPen penAxis(PS_SOLID, 1, RGB(150, 150, 160));
    CPen* pOldPen = pDC->SelectObject(&penAxis);

    // 绘制边框
    pDC->MoveTo(rcPlot.left,  rcPlot.top);
    pDC->LineTo(rcPlot.right, rcPlot.top);
    pDC->LineTo(rcPlot.right, rcPlot.bottom);
    pDC->LineTo(rcPlot.left,  rcPlot.bottom);
    pDC->LineTo(rcPlot.left,  rcPlot.top);

    // 零线加粗
    CPen penZero(PS_SOLID, 1, RGB(180, 180, 190));
    pDC->SelectObject(&penZero);
    CPoint ptZero = DataToScreen(0, 0);
    if (ptZero.y >= rcPlot.top && ptZero.y <= rcPlot.bottom)
    {
        pDC->MoveTo(rcPlot.left,  ptZero.y);
        pDC->LineTo(rcPlot.right, ptZero.y);
    }

    // 标签
    CFont fontLabel;
    fontLabel.CreatePointFont(75, _T("Consolas"));
    CFont* pOldFont = pDC->SelectObject(&fontLabel);
    pDC->SetTextColor(RGB(100, 100, 110));

    CPoint ptTL = rcPlot.TopLeft();
    CPoint ptBR = rcPlot.BottomRight();
    double tMin, tMax, aMin, aMax;
    ScreenToData(ptTL, tMin, aMax);
    ScreenToData(ptBR, tMax, aMin);

    // 时间标签（底部）
    double dt = (tMax - tMin) / 5.0;
    if (dt < 0.01) dt = 0.1;
    double tStart = floor(tMin / dt) * dt;
    for (double t = tStart; t <= tMax; t += dt)
    {
        CPoint pt = DataToScreen(t, 0);
        CString str;
        str.Format(_T("%.2fs"), t);
        pDC->TextOutW(pt.x - 15, rcPlot.bottom + 3, str);
    }

    // 幅值标签（左侧）
    double dv = (aMax - aMin) / 5.0;
    if (dv < 0.01) dv = 0.5;
    double vStart = floor(aMin / dv) * dv;
    for (double v = vStart; v <= aMax; v += dv)
    {
        CPoint pt = DataToScreen(0, v);
        CString str;
        str.Format(_T("%.1f"), v);
        pDC->TextOutW(rcPlot.left - 35, pt.y - 8, str);
    }

    // 图例
    CFont fontLegend;
    fontLegend.CreatePointFont(80, _T("微软雅黑"));
    pDC->SelectObject(&fontLegend);

    int legendX = rcPlot.right - 180;
    int legendY = rcPlot.top + 8;

    // 原始信号图例
    CPen penRaw(PS_SOLID, 2, m_colorRaw);
    pDC->SelectObject(&penRaw);
    pDC->MoveTo(legendX, legendY + 5);
    pDC->LineTo(legendX + 25, legendY + 5);
    pDC->SetTextColor(m_colorRaw);
    pDC->TextOutW(legendX + 30, legendY, _T("原始信号"));

    // 滤波信号图例
    if (m_bShowFiltered)
    {
        legendY += 18;
        CPen penFilt(PS_SOLID, 2, m_colorFiltered);
        pDC->SelectObject(&penFilt);
        pDC->MoveTo(legendX, legendY + 5);
        pDC->LineTo(legendX + 25, legendY + 5);
        pDC->SetTextColor(m_colorFiltered);
        pDC->TextOutW(legendX + 30, legendY, _T("滤波信号"));
    }

    // R波图例
    if (m_bShowRPeaks)
    {
        legendY += 18;
        CPen penR(PS_SOLID, 2, m_colorRPeak);
        pDC->SelectObject(&penR);
        pDC->MoveTo(legendX, legendY + 5);
        pDC->LineTo(legendX + 25, legendY + 5);
        pDC->Ellipse(legendX + 9, legendY + 1, legendX + 16, legendY + 9);
        pDC->SetTextColor(m_colorRPeak);
        pDC->TextOutW(legendX + 30, legendY, _T("R波检测"));
    }

    pDC->SelectObject(pOldFont);
    pDC->SelectObject(pOldPen);
}

// ---- 绘制右键选区矩形 ----
void CECGViewerView::DrawSelectionRect(CDC* pDC)
{
    CPen pen(PS_DASH, 1, RGB(200, 80, 0));
    CPen* pOldPen = pDC->SelectObject(&pen);
    CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);

    pDC->Rectangle(m_ptRStart.x, m_ptRStart.y, m_ptREnd.x, m_ptREnd.y);

    // 显示选区信息
    double t1, a1, t2, a2;
    ScreenToData(m_ptRStart, t1, a1);
    ScreenToData(m_ptREnd, t2, a2);
    double dt = fabs(t2 - t1);
    double rrBPM = (dt > 0.001) ? (60.0 / dt) : 0.0;

    CString info;
    info.Format(_T("ΔT=%.3fs  HR≈%.0f BPM"), dt, rrBPM);
    pDC->SetTextColor(RGB(200, 80, 0));
    pDC->TextOutW(m_ptREnd.x + 8, m_ptREnd.y - 8, info);

    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);
}

// ---- 绘制播放头 ----
void CECGViewerView::DrawPlayhead(CDC* pDC)
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData()) return;

    CRect rcPlot = GetPlotRect(GetClientRectDPI());
    CPoint pt = DataToScreen(m_playStartTime, 0);

    if (pt.x >= rcPlot.left && pt.x <= rcPlot.right)
    {
        CPen pen(PS_SOLID, 1, RGB(0, 160, 0));
        CPen* pOldPen = pDC->SelectObject(&pen);
        pDC->MoveTo(pt.x, rcPlot.top);
        pDC->LineTo(pt.x, rcPlot.bottom);
        pDC->SelectObject(pOldPen);
    }
}

// ============================================================================
// 坐标变换
// ============================================================================

CPoint CECGViewerView::DataToScreen(double timeSec, double amplitude) const
{
    CRect rcPlot = GetPlotRect(GetClientRectDPI());

    int x = rcPlot.left + (int)(timeSec * m_pixelsPerSecond);
    // Center 0mV vertically in the plot area; positive amplitude goes UP (screen Y decreases)
    int y = rcPlot.CenterPoint().y - (int)(amplitude * m_pixelsPerMillivolt);

    return CPoint(x, y);
}

void CECGViewerView::ScreenToData(CPoint ptScreen, double& timeSec, double& amplitude) const
{
    CRect rcPlot = GetPlotRect(GetClientRectDPI());

    timeSec = (ptScreen.x - rcPlot.left) / m_pixelsPerSecond;
    amplitude = -(ptScreen.y - rcPlot.CenterPoint().y) / m_pixelsPerMillivolt;
}

CRect CECGViewerView::GetPlotRect(const CRect& rcClient) const
{
    return CRect(
        rcClient.left   + m_marginLeft,
        rcClient.top    + m_marginTop,
        rcClient.right  - m_marginRight,
        rcClient.bottom - m_marginBottom
    );
}

CRect CECGViewerView::GetClientRectDPI() const
{
    CRect rc;
    GetClientRect(&rc);
    return rc;
}

// ============================================================================
// 滚动视图
// ============================================================================

void CECGViewerView::UpdateScrollSizes()
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData())
    {
        SetScrollSizes(MM_TEXT, CSize(0, 0));
        return;
    }

    double duration = pDoc->GetData().GetDuration();
    int totalWidth = (int)(duration * m_pixelsPerSecond) + m_marginLeft + m_marginRight;
    int totalHeight = 2000;  // 固定的垂直滚动范围

    // 确保最小尺寸
    CRect rc;
    GetClientRect(&rc);
    if (totalWidth < rc.Width()) totalWidth = rc.Width();
    if (totalHeight < rc.Height()) totalHeight = rc.Height();

    SetScrollSizes(MM_TEXT, CSize(totalWidth, totalHeight),
                   CSize(rc.Width() / 4, rc.Height() / 4),
                   CSize(rc.Width() / 10, rc.Height() / 10));
}

// ============================================================================
// 缩放控制（体现了鼠标滚轮 & 键盘 +/- 缩放功能）
// ============================================================================

void CECGViewerView::ZoomIn()
{
    m_zoomFactor *= 1.3;
    m_pixelsPerSecond = DEFAULT_PPS * m_zoomFactor;
    if (m_pixelsPerSecond > MAX_PPS)
        m_pixelsPerSecond = MAX_PPS;
    m_pixelsPerMillivolt = DEFAULT_PPMV * m_zoomFactor;
    UpdateScrollSizes();
    Invalidate();
}

void CECGViewerView::ZoomOut()
{
    m_zoomFactor /= 1.3;
    m_pixelsPerSecond = DEFAULT_PPS * m_zoomFactor;
    if (m_pixelsPerSecond < MIN_PPS)
        m_pixelsPerSecond = MIN_PPS;
    m_pixelsPerMillivolt = DEFAULT_PPMV * m_zoomFactor;
    UpdateScrollSizes();
    Invalidate();
}

void CECGViewerView::ZoomFit()
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData()) return;

    CRect rcPlot = GetPlotRect(GetClientRectDPI());
    double duration = pDoc->GetData().GetDuration();
    if (duration <= 0) return;

    m_pixelsPerSecond = (rcPlot.Width() - 20) / duration;
    if (m_pixelsPerSecond < MIN_PPS) m_pixelsPerSecond = MIN_PPS;
    if (m_pixelsPerSecond > MAX_PPS) m_pixelsPerSecond = MAX_PPS;

    // 垂直适配
    double yRange = pDoc->GetData().GetMaxValue() - pDoc->GetData().GetMinValue();
    if (yRange < 0.5) yRange = 2.0;
    m_pixelsPerMillivolt = (rcPlot.Height() - 20) / (yRange * 1.2);

    m_zoomFactor = m_pixelsPerSecond / DEFAULT_PPS;

    UpdateScrollSizes();
    Invalidate();
}

void CECGViewerView::ZoomReset()
{
    m_zoomFactor = 1.0;
    m_pixelsPerSecond = DEFAULT_PPS;
    m_pixelsPerMillivolt = DEFAULT_PPMV;
    UpdateScrollSizes();
    Invalidate();
}

// ============================================================================
// 鼠标交互（体现了鼠标课程知识）
// ============================================================================

void CECGViewerView::OnMouseMove(UINT nFlags, CPoint point)
{
    CECGViewerDoc* pDoc = GetDocument();
    CMainFrame* pFrame = dynamic_cast<CMainFrame*>(GetParentFrame());

    // 左键拖拽平移（Pan）
    if (m_bMouseLDown)
    {
        int dx = point.x - m_ptLastMouse.x;
        int dy = point.y - m_ptLastMouse.y;

        CPoint ptScroll = GetScrollPosition();
        ptScroll.x -= dx;
        ptScroll.y -= dy;
        if (ptScroll.x < 0) ptScroll.x = 0;
        if (ptScroll.y < 0) ptScroll.y = 0;
        ScrollToPosition(ptScroll);

        Invalidate();
    }

    // 右键拖拽框选
    if (m_bMouseRDown)
    {
        m_ptREnd = point;
        Invalidate();
    }

    // 更新状态栏光标位置
    if (pDoc && pDoc->HasData() && pFrame)
    {
        double t, a;
        ScreenToData(point, t, a);
        pFrame->UpdateCursorPosition(t, a);
    }

    m_ptLastMouse = point;
    CScrollView::OnMouseMove(nFlags, point);
}

void CECGViewerView::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bMouseLDown = true;
    m_ptLastMouse = point;
    SetCapture();  // 捕获鼠标（即使移到窗口外也能接收消息）

    CScrollView::OnLButtonDown(nFlags, point);
}

void CECGViewerView::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_bMouseLDown = false;
    ReleaseCapture();

    CScrollView::OnLButtonUp(nFlags, point);
}

void CECGViewerView::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_bMouseRDown = true;
    m_ptRStart = point;
    m_ptREnd = point;
    m_bHasSelection = false;

    CScrollView::OnRButtonDown(nFlags, point);
}

void CECGViewerView::OnRButtonUp(UINT nFlags, CPoint point)
{
    m_bMouseRDown = false;
    m_ptREnd = point;

    // 只有拖拽超过一定像素才算有效选区
    int dx = abs(m_ptREnd.x - m_ptRStart.x);
    int dy = abs(m_ptREnd.y - m_ptRStart.y);

    if (dx > 5 || dy > 5)
    {
        m_bHasSelection = true;

        // 计算选区时间差和对应的心率
        double t1, a1, t2, a2;
        ScreenToData(m_ptRStart, t1, a1);
        ScreenToData(m_ptREnd, t2, a2);
        double dt = fabs(t2 - t1);
        double bpm = (dt > 0.001) ? (60.0 / dt) : 0.0;

        // 更新状态栏心率
        CMainFrame* pFrame = dynamic_cast<CMainFrame*>(GetParentFrame());
        if (pFrame)
            pFrame->UpdateHeartRate(bpm);
    }
    else
    {
        m_bHasSelection = false;
    }

    Invalidate();
    CScrollView::OnRButtonUp(nFlags, point);
}

BOOL CECGViewerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // Ctrl + 滚轮 = 缩放（体现了鼠标滚轮缩放功能）
    if (nFlags & MK_CONTROL)
    {
        if (zDelta > 0)
            ZoomIn();
        else
            ZoomOut();
        return TRUE;
    }
    else
    {
        // 普通滚轮 = 垂直滚动
        // 不做特殊处理，让基类处理
        return CScrollView::OnMouseWheel(nFlags, zDelta, pt);
    }
}

// ============================================================================
// 键盘交互（体现了键盘课程知识）
// ============================================================================

void CECGViewerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CECGViewerDoc* pDoc = GetDocument();
    CMainFrame* pFrame = dynamic_cast<CMainFrame*>(GetParentFrame());

    switch (nChar)
    {
    case VK_LEFT:       // ← 左移
        {
            CPoint pt = GetScrollPosition();
            pt.x -= 50;
            if (pt.x < 0) pt.x = 0;
            ScrollToPosition(pt);
            Invalidate();
        }
        break;

    case VK_RIGHT:      // → 右移
        {
            CPoint pt = GetScrollPosition();
            pt.x += 50;
            ScrollToPosition(pt);
            Invalidate();
        }
        break;

    case VK_UP:         // ↑ 上移
        {
            CPoint pt = GetScrollPosition();
            pt.y -= 30;
            if (pt.y < 0) pt.y = 0;
            ScrollToPosition(pt);
            Invalidate();
        }
        break;

    case VK_DOWN:       // ↓ 下移
        {
            CPoint pt = GetScrollPosition();
            pt.y += 30;
            ScrollToPosition(pt);
            Invalidate();
        }
        break;

    case VK_OEM_PLUS:   // + 放大
    case VK_ADD:
        ZoomIn();
        break;

    case VK_OEM_MINUS:  // - 缩小
    case VK_SUBTRACT:
        ZoomOut();
        break;

    case VK_SPACE:      // 空格 — 播放/暂停动画
        if (pDoc && pDoc->HasData())
        {
            if (m_bPlaying)
            {
                KillTimer(1);
                m_bPlaying = false;
                if (pFrame)
                    pFrame->UpdateStatusBar(_T("播放已暂停"), 0);
            }
            else
            {
                m_playStartTime = 0.0;
                m_bPlaying = true;
                SetTimer(1, 50, nullptr);  // 50ms 定时器 → ~20 fps
                if (pFrame)
                    pFrame->UpdateStatusBar(_T("正在播放心电波形..."), 0);
            }
            Invalidate();
        }
        break;

    case 'F':           // F — 适应窗口
        ZoomFit();
        break;

    case 'R':           // R — 重置缩放
        ZoomReset();
        break;

    case 'G':           // G — 切换网格
        m_bShowGrid = !m_bShowGrid;
        Invalidate();
        break;

    default:
        break;
    }

    CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

// ============================================================================
// 定时器 — 动画播放
// ============================================================================

void CECGViewerView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_bPlaying)
    {
        CECGViewerDoc* pDoc = GetDocument();
        if (!pDoc || !pDoc->HasData())
        {
            KillTimer(1);
            m_bPlaying = false;
            return;
        }

        m_playStartTime += 0.05;  // 每 50ms 前进 50ms（实时播放）

        double duration = pDoc->GetData().GetDuration();
        if (m_playStartTime >= duration)
        {
            m_playStartTime = 0.0;  // 循环播放
        }

        // 自动滚动使播放头始终在可见区域中央
        CRect rcPlot = GetPlotRect(GetClientRectDPI());
        double visibleWidth = rcPlot.Width() / m_pixelsPerSecond;
        double halfVisible = visibleWidth / 2.0;

        CPoint ptScroll = GetScrollPosition();
        int targetScrollX = (int)((m_playStartTime - halfVisible) * m_pixelsPerSecond);
        if (targetScrollX < 0) targetScrollX = 0;
        ptScroll.x = targetScrollX;
        ScrollToPosition(ptScroll);

        Invalidate();
    }

    CScrollView::OnTimer(nIDEvent);
}

// ============================================================================
// 菜单/工具栏命令
// ============================================================================

void CECGViewerView::OnFileOpen()
{
    CFileDialog dlg(TRUE, _T(".csv"), nullptr,
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*||"),
        this);

    if (dlg.DoModal() == IDOK)
    {
        AfxGetApp()->OpenDocumentFile(dlg.GetPathName());
    }
}

void CECGViewerView::OnViewZoomIn()
{
    ZoomIn();
}

void CECGViewerView::OnViewZoomOut()
{
    ZoomOut();
}

void CECGViewerView::OnViewZoomFit()
{
    ZoomFit();
}

void CECGViewerView::OnViewZoomReset()
{
    ZoomReset();
}

void CECGViewerView::OnViewToggleGrid()
{
    m_bShowGrid = !m_bShowGrid;
    Invalidate();
}

// ============================================================================
// 滤波与 R 波检测（体现了动态链接库课程知识 — LoadLibrary/GetProcAddress）
// ============================================================================

void CECGViewerView::OnToolsFilter()
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData())
    {
        AfxMessageBox(_T("请先打开心电数据文件！"), MB_ICONINFORMATION);
        return;
    }

    // ---- 加载 ECGFilterDLL.dll（动态链接库） ----
    HMODULE hDll = LoadLibrary(_T("ECGFilterDLL.dll"));
    if (!hDll)
    {
        AfxMessageBox(_T("无法加载 ECGFilterDLL.dll！\n请确保 DLL 文件在程序目录下。"),
                      MB_ICONERROR);
        return;
    }

    // ---- 获取导出函数地址 ----
    typedef int (*BandpassFilterFunc)(const double*, double*, int, int);
    BandpassFilterFunc pBandpassFilter =
        (BandpassFilterFunc)GetProcAddress(hDll, "BandpassFilter");

    if (!pBandpassFilter)
    {
        AfxMessageBox(_T("DLL 中找不到 BandpassFilter 函数！"), MB_ICONERROR);
        FreeLibrary(hDll);
        return;
    }

    // ---- 准备数据 ----
    const auto& signal = pDoc->GetData().GetLead(0).signal;
    int n = (int)signal.size();
    int sr = (int)pDoc->GetData().GetSamplingRate();
    std::vector<double> filtered(n);

    // ---- 调用 DLL 中的滤波函数 ----
    int ret = pBandpassFilter(signal.data(), filtered.data(), n, sr);
    if (ret == 0)
    {
        pDoc->SetFilteredSignal(filtered);
        m_bShowFiltered = true;

        CMainFrame* pFrame = dynamic_cast<CMainFrame*>(GetParentFrame());
        if (pFrame)
            pFrame->UpdateStatusBar(_T("带通滤波完成（ECGFilterDLL.dll — BandpassFilter）"), 0);

        Invalidate();
    }
    else
    {
        AfxMessageBox(_T("滤波失败！参数无效。"), MB_ICONERROR);
    }

    FreeLibrary(hDll);
}

void CECGViewerView::OnToolsDetectR()
{
    CECGViewerDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->HasData())
    {
        AfxMessageBox(_T("请先打开心电数据文件！"), MB_ICONINFORMATION);
        return;
    }

    // ---- 加载 ECGFilterDLL.dll ----
    HMODULE hDll = LoadLibrary(_T("ECGFilterDLL.dll"));
    if (!hDll)
    {
        AfxMessageBox(_T("无法加载 ECGFilterDLL.dll！"), MB_ICONERROR);
        return;
    }

    // ---- 获取 R 波检测函数 ----
    typedef int (*DetectRPeaksFunc)(const double*, int, int*, int*, int);
    DetectRPeaksFunc pDetectRPeaks =
        (DetectRPeaksFunc)GetProcAddress(hDll, "DetectRPeaks");

    typedef int (*HeartRateFunc)(const int*, int, double*, int);
    HeartRateFunc pHeartRateFromRR =
        (HeartRateFunc)GetProcAddress(hDll, "HeartRateFromRR");

    if (!pDetectRPeaks)
    {
        AfxMessageBox(_T("DLL 中找不到 DetectRPeaks 函数！"), MB_ICONERROR);
        FreeLibrary(hDll);
        return;
    }

    // ---- 选择检测源信号 ----
    const std::vector<double>* pSignal;
    if (pDoc->HasFilteredData())
        pSignal = &pDoc->GetFilteredSignal();  // 优先对滤波后信号做检测
    else
        pSignal = &pDoc->GetData().GetLead(0).signal;

    int n = (int)pSignal->size();
    int sr = (int)pDoc->GetData().GetSamplingRate();

    std::vector<int> peaks(n);
    int peakCount = 0;

    int ret = pDetectRPeaks(pSignal->data(), n, peaks.data(), &peakCount, sr);

    if (ret == 0 && peakCount > 0)
    {
        peaks.resize(peakCount);
        pDoc->SetRPeaks(peaks);
        m_bShowRPeaks = true;

        // 计算平均心率
        if (peakCount >= 2 && pHeartRateFromRR)
        {
            std::vector<int> rrIntervals(peakCount - 1);
            std::vector<double> heartRates(peakCount - 1);
            for (int i = 1; i < peakCount; ++i)
                rrIntervals[i - 1] = peaks[i] - peaks[i - 1];

            pHeartRateFromRR(rrIntervals.data(), peakCount - 1,
                             heartRates.data(), sr);

            double avgHR = 0.0;
            for (double hr : heartRates) avgHR += hr;
            avgHR /= (peakCount - 1);

            CMainFrame* pFrame = dynamic_cast<CMainFrame*>(GetParentFrame());
            if (pFrame)
            {
                pFrame->UpdateHeartRate(avgHR);
                CString msg;
                msg.Format(_T("R波检测完成 — 检测到 %d 个R波，平均心率 %.0f BPM"),
                           peakCount, avgHR);
                pFrame->UpdateStatusBar(msg, 0);
            }
        }

        Invalidate();
    }
    else
    {
        CString msg;
        msg.Format(_T("未检测到 R 波峰值。（检测到 %d 个候选点）"), peakCount);
        AfxMessageBox(msg, MB_ICONINFORMATION);
    }

    FreeLibrary(hDll);
}

// ============================================================================
// 动态创建 / 诊断
// ============================================================================

#ifdef _DEBUG
void CECGViewerView::AssertValid() const
{
    CScrollView::AssertValid();
}

void CECGViewerView::Dump(CDumpContext& dc) const
{
    CScrollView::Dump(dc);
}

CECGViewerDoc* CECGViewerView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CECGViewerDoc)));
    return (CECGViewerDoc*)m_pDocument;
}
#endif //_DEBUG

// Release 版本也需要 GetDocument
#ifndef _DEBUG
CECGViewerDoc* CECGViewerView::GetDocument() const
{
    return (CECGViewerDoc*)m_pDocument;
}
#endif
