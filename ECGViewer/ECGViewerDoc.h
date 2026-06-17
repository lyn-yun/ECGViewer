// ECGViewerDoc.h — 文档类
//
// 对应课堂知识：
//   MFC 图形基础（Doc/View 架构）— CDocument 派生，管理数据
//   数据的加载与存储，与视图分离

#pragma once
#include "ECGData.h"

class CECGViewerDoc : public CDocument
{
public:
    CECGViewerDoc();
    virtual ~CECGViewerDoc();

    // ---- 覆盖 ----
    virtual BOOL OnNewDocument();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void Serialize(CArchive& ar);

    // ---- 数据访问 ----
    CECGData&       GetData()       { return m_ecgData; }
    const CECGData& GetData() const { return m_ecgData; }
    BOOL            HasData() const { return m_ecgData.GetLeadCount() > 0; }

    // ---- 派生数据（滤波后） ----
    BOOL            HasFilteredData() const { return !m_filteredSignal.empty(); }
    const std::vector<double>& GetFilteredSignal() const { return m_filteredSignal; }
    const std::vector<int>&    GetRPeaks() const          { return m_rPeaks; }

    void SetFilteredSignal(const std::vector<double>& sig);
    void SetRPeaks(const std::vector<int>& peaks);

protected:
    DECLARE_MESSAGE_MAP()
    DECLARE_DYNCREATE(CECGViewerDoc)

private:
    CECGData              m_ecgData;
    std::vector<double>   m_filteredSignal;  // 滤波后的信号
    std::vector<int>      m_rPeaks;          // R 波峰值位置（样本索引）
};
