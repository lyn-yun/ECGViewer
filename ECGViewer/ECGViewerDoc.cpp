// ECGViewerDoc.cpp — 文档类实现
#include "stdafx.h"
#include "ECGViewerDoc.h"

IMPLEMENT_DYNCREATE(CECGViewerDoc, CDocument)

BEGIN_MESSAGE_MAP(CECGViewerDoc, CDocument)
END_MESSAGE_MAP()

CECGViewerDoc::CECGViewerDoc()
{
}

CECGViewerDoc::~CECGViewerDoc()
{
}

BOOL CECGViewerDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    m_ecgData.Clear();
    m_filteredSignal.clear();
    m_rPeaks.clear();

    return TRUE;
}

BOOL CECGViewerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;

    // 尝试从 CSV 文件加载数据
    if (!m_ecgData.LoadFromCSV(lpszPathName))
    {
        AfxMessageBox(_T("无法加载文件！\n请确保文件为有效的 CSV 格式（时间,幅值）。"),
                      MB_ICONERROR);
        return FALSE;
    }

    m_filteredSignal.clear();
    m_rPeaks.clear();

    return TRUE;
}

void CECGViewerDoc::Serialize(CArchive& ar)
{
    // 心电数据目前仅支持导入 CSV，不支持 MFC 序列化存储
    // 留空即可
    UNREFERENCED_PARAMETER(ar);
}

void CECGViewerDoc::SetFilteredSignal(const std::vector<double>& sig)
{
    m_filteredSignal = sig;
}

void CECGViewerDoc::SetRPeaks(const std::vector<int>& peaks)
{
    m_rPeaks = peaks;
}
