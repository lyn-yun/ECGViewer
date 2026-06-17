// ECGViewer.h — MFC 应用程序类
// 继承自 CWinApp，是整个应用程序的入口
//
// 对应课堂知识：MFC 图形基础（框架窗口/文档/视图架构）
// 应用程序对象管理整个程序的生命周期

#pragma once
#include "Resource.h"

class CECGViewerApp : public CWinApp
{
public:
    CECGViewerApp();
    virtual ~CECGViewerApp();

    // ---- 覆盖 ----
    virtual BOOL InitInstance();
    virtual int  ExitInstance();

    DECLARE_MESSAGE_MAP()

private:
    // GDI+ 初始化令牌
    ULONG_PTR m_gdiplusToken;

    // 注册文档模板
    void RegisterDocTemplate();
};

// 全局应用程序对象（MFC 框架通过它启动程序）
extern CECGViewerApp theApp;
