// ECGViewerView.h — 视图类（核心）
//
// 对应课堂知识（一网打尽）：
//   GDI 图形    — CDC/CPen/CBrush/CFont，绘制心电图波形、网格、坐标轴
//   鼠标        — 左键拖拽平移波形，右键框选区域测量，滚轮缩放
//   键盘        — 方向键平移，+/- 缩放，空格键暂停/恢复动画
//   控件        — 与主框架的状态栏联动显示光标坐标和心率
//
// 继承自 CScrollView，支持大范围波形滚动

#pragma once
#include "ECGFilterDLL.h"  // DLL export declarations

class CECGViewerDoc;  // forward declaration

class CECGViewerView : public CScrollView
{
public:
    CECGViewerView();
    virtual ~CECGViewerView();

    // ---- 覆盖 ----
    virtual void OnInitialUpdate();
    virtual void OnDraw(CDC* pDC);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    CECGViewerDoc* GetDocument() const;

    // ---- 缩放/平移 ----
    void ZoomIn();
    void ZoomOut();
    void ZoomFit();
    void ZoomReset();

    // ---- 滤波 / R波检测（调用 ECGFilterDLL） ----
    void ApplyFilter();
    void DetectRPeaks();

protected:
    DECLARE_MESSAGE_MAP()
    DECLARE_DYNCREATE(CECGViewerView)

    // ---- 鼠标消息 ----
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

    // ---- 键盘消息 ----
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    // ---- 菜单/工具栏命令 ----
    afx_msg void OnFileOpen();
    afx_msg void OnViewZoomIn();
    afx_msg void OnViewZoomOut();
    afx_msg void OnViewZoomFit();
    afx_msg void OnToolsFilter();
    afx_msg void OnToolsDetectR();

    // ---- 定时器（动画播放） ----
    afx_msg void OnTimer(UINT_PTR nIDEvent);

private:
    // ========== 缩放与视图参数 ==========
    double  m_pixelsPerSecond;      // 水平缩放：每秒对应的像素数
    double  m_pixelsPerMillivolt;   // 垂直缩放：每mV对应的像素数
    double  m_zoomFactor;           // 当前缩放因子（1.0 = 标准）

    static constexpr double MIN_PPS = 5.0;      // 最小 5 像素/秒
    static constexpr double MAX_PPS = 2000.0;   // 最大 2000 像素/秒
    static constexpr double DEFAULT_PPS = 100.0; // 默认 100 像素/秒
    static constexpr double DEFAULT_PPMV = 300.0; // 默认 300 像素/mV

    // ========== 绘图区域 ==========
    CPoint  m_ptOrigin;             // 逻辑原点（对应 0秒, 0mV）
    int     m_marginLeft;           // 左边距（像素）
    int     m_marginTop;
    int     m_marginRight;
    int     m_marginBottom;

    // ========== 鼠标状态 ==========
    bool    m_bMouseLDown;          // 左键是否按下（拖拽平移）
    CPoint  m_ptLastMouse;          // 上次鼠标位置

    bool    m_bMouseRDown;          // 右键是否按下（框选测量）
    CPoint  m_ptRStart;             // 右键起始点
    CPoint  m_ptREnd;               // 右键结束点
    bool    m_bHasSelection;        // 是否有一个有效选区

    // ========== 动画播放 ==========
    bool    m_bPlaying;             // 是否正在播放
    double  m_playStartTime;        // 播放起始时间

    // ========== 显示选项 ==========
    bool    m_bShowGrid;            // 显示网格
    bool    m_bShowFiltered;        // 显示滤波后信号
    bool    m_bShowRPeaks;          // 显示R波标记
    COLORREF m_colorRaw;            // 原始信号颜色
    COLORREF m_colorFiltered;       // 滤波后信号颜色
    COLORREF m_colorRPeak;          // R波标记颜色
    COLORREF m_colorGrid;           // 网格颜色
    COLORREF m_colorBg;             // 背景颜色

    // ========== 内部方法 ==========
    void UpdateScrollSizes();
    void DrawGrid(CDC* pDC, const CRect& rcClient);
    void DrawSignal(CDC* pDC, const std::vector<double>& time,
                    const std::vector<double>& signal, COLORREF color,
                    int lineWidth = 1);
    void DrawRPeaks(CDC* pDC, const std::vector<double>& time,
                    const std::vector<int>& peaks, int leadIdx = 0);
    void DrawAxes(CDC* pDC, const CRect& rcClient);
    void DrawSelectionRect(CDC* pDC);
    void DrawPlayhead(CDC* pDC);

    CPoint DataToScreen(double timeSec, double amplitude) const;
    void   ScreenToData(CPoint ptScreen, double& timeSec, double& amplitude) const;
    CRect  GetPlotRect(const CRect& rcClient) const;

    // 获取绘图区域（去掉边距后的矩形）
    CRect  GetClientRectDPI() const;
};
