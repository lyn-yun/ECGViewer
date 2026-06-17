// stdafx.h — ECGViewer MFC precompiled header
// Contains all standard headers needed by the MFC application

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN  // 排除不常用的 Windows 头文件
#endif

#include "targetver.h"

// MFC 核心
#include <afxwin.h>         // MFC 核心和标准组件
#include <afxext.h>         // MFC 扩展（工具栏、状态栏等）
#include <afxdisp.h>        // MFC 自动化
#include <afxdialogex.h>    // MFC 对话框
#include <afxcmn.h>         // MFC 通用控件
#include <afxvisualmanager.h>   // MFC 视觉管理器（CMFCVisualManager）

// GDI+
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// 标准库
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <memory>

// 链接库
#pragma comment(lib, "comctl32.lib")

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
