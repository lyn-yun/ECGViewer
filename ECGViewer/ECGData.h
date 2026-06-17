// ECGData.h — 心电数据模型
// 使用 MFC 动态链接库课程知识：
//   数据层与显示层分离，Doc 持有数据，View 负责渲染
//
// 存储心电信号的时间序列数据和相关元信息

#pragma once
#include <vector>
#include <string>
#include <afxstr.h>

/// 单个导联的心电数据
struct ECGLead
{
    CString     name;           // 导联名称，如 "Lead II"
    std::vector<double> time;   // 时间数组（秒）
    std::vector<double> signal; // 信号幅值（mV）
    double      samplingRate;   // 采样率（Hz）
};

/// 心电数据文档模型
class CECGData
{
public:
    CECGData();
    ~CECGData();

    // ---- 文件 I/O ----
    bool LoadFromCSV(const CString& filePath);
    void Clear();

    // ---- 访问器 ----
    int          GetLeadCount() const              { return (int)m_leads.size(); }
    ECGLead&     GetLead(int idx)                  { return m_leads[idx]; }
    const ECGLead& GetLead(int idx) const          { return m_leads[idx]; }
    int          GetSampleCount(int leadIdx = 0) const;
    double       GetDuration(int leadIdx = 0) const;
    double       GetSamplingRate(int leadIdx = 0) const;

    CString      GetFilePath() const               { return m_filePath; }
    CString      GetFileName() const;

    // 统计信息
    double       GetMinValue(int leadIdx = 0) const;
    double       GetMaxValue(int leadIdx = 0) const;
    double       GetMeanValue(int leadIdx = 0) const;

    // 获取信号值（带边界检查）
    double       GetSample(int idx, int leadIdx = 0) const;

private:
    std::vector<ECGLead> m_leads;
    CString              m_filePath;
};
