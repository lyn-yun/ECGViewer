// ECGData.cpp — 心电数据模型实现
#include "stdafx.h"
#include "ECGData.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

CECGData::CECGData()
{
}

CECGData::~CECGData()
{
    Clear();
}

void CECGData::Clear()
{
    m_leads.clear();
    m_filePath.Empty();
}

// ---- 从 CSV 加载数据 ----
// 格式：第一行为可选的标题行（如 "time_s,amplitude_mv"）
//       后续每行为 "时间,幅值"
//       若只有一列数据则自动生成时间轴
bool CECGData::LoadFromCSV(const CString& filePath)
{
    Clear();

    // Read entire file (use CW2A variable, not temporary, for ifstream)
    CW2A pszPath(filePath);
    std::ifstream file(pszPath);
    if (!file.is_open())
        return false;

    // Read line by line
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#')
            continue;
        lines.push_back(line);
    }
    file.close();

    if (lines.empty())
        return false;

    // 检测第一行是否为标题行
    size_t dataStart = 0;
    {
        std::stringstream ss(lines[0]);
        std::string token;
        bool hasText = false;
        while (std::getline(ss, token, ','))
        {
            // 去除首尾空白
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            if (!token.empty() && !std::isdigit(token[0]) && token[0] != '-' && token[0] != '+' && token[0] != '.')
            {
                hasText = true;
                break;
            }
        }
        if (hasText) dataStart = 1;
    }

    // 解析数据
    std::vector<double> col0, col1;
    for (size_t i = dataStart; i < lines.size(); ++i)
    {
        std::stringstream ss(lines[i]);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ','))
            tokens.push_back(token);

        if (tokens.empty()) continue;

        try
        {
            double v0 = std::stod(tokens[0]);
            col0.push_back(v0);
            if (tokens.size() >= 2)
            {
                double v1 = std::stod(tokens[1]);
                col1.push_back(v1);
            }
        }
        catch (...)
        {
            continue;  // 跳过无效行
        }
    }

    if (col0.empty())
        return false;

    // 构造 ECGLead
    ECGLead lead;
    lead.name = _T("Lead II");

    if (col1.empty())
    {
        // 单列数据 → 自动生成时间轴
        lead.samplingRate = 360.0;
        lead.signal = col0;
        lead.time.resize(col0.size());
        for (size_t i = 0; i < col0.size(); ++i)
            lead.time[i] = i / lead.samplingRate;
    }
    else
    {
        // 两列数据 → 时间 + 幅值
        lead.time = col0;
        lead.signal = col1;
        // 估算采样率
        if (col0.size() >= 2)
            lead.samplingRate = 1.0 / (col0[1] - col0[0]);
        else
            lead.samplingRate = 360.0;
    }

    m_leads.push_back(lead);
    m_filePath = filePath;
    return true;
}

// ---- 访问器 ----

int CECGData::GetSampleCount(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size())
        return 0;
    return (int)m_leads[leadIdx].signal.size();
}

double CECGData::GetDuration(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size())
        return 0.0;
    const auto& t = m_leads[leadIdx].time;
    if (t.empty()) return 0.0;
    return t.back() - t.front();
}

double CECGData::GetSamplingRate(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size())
        return 0.0;
    return m_leads[leadIdx].samplingRate;
}

CString CECGData::GetFileName() const
{
    int pos = m_filePath.ReverseFind(_T('\\'));
    if (pos >= 0)
        return m_filePath.Mid(pos + 1);
    pos = m_filePath.ReverseFind(_T('/'));
    if (pos >= 0)
        return m_filePath.Mid(pos + 1);
    return m_filePath;
}

double CECGData::GetMinValue(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size()) return 0.0;
    const auto& s = m_leads[leadIdx].signal;
    if (s.empty()) return 0.0;
    return *std::min_element(s.begin(), s.end());
}

double CECGData::GetMaxValue(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size()) return 0.0;
    const auto& s = m_leads[leadIdx].signal;
    if (s.empty()) return 0.0;
    return *std::max_element(s.begin(), s.end());
}

double CECGData::GetMeanValue(int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size()) return 0.0;
    const auto& s = m_leads[leadIdx].signal;
    if (s.empty()) return 0.0;
    double sum = 0.0;
    for (double v : s) sum += v;
    return sum / s.size();
}

double CECGData::GetSample(int idx, int leadIdx) const
{
    if (leadIdx < 0 || leadIdx >= (int)m_leads.size()) return 0.0;
    const auto& s = m_leads[leadIdx].signal;
    if (idx < 0 || idx >= (int)s.size()) return 0.0;
    return s[idx];
}
