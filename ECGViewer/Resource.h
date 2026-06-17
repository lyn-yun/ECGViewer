// Resource.h — 资源 ID 定义
// 用于菜单、工具栏、对话框等 UI 元素的标识

#pragma once

// ---- 菜单命令 ID ----
#define IDM_FILE_OPEN            32771
#define IDM_FILE_SAVE_AS         32772
#define IDM_FILE_EXIT            32773
#define IDM_VIEW_ZOOM_IN         32774
#define IDM_VIEW_ZOOM_OUT        32775
#define IDM_VIEW_ZOOM_FIT        32776
#define IDM_VIEW_ZOOM_RESET      32777
#define IDM_TOOLS_FILTER         32778
#define IDM_TOOLS_DETECT_R       32779
#define IDM_HELP_ABOUT           32780

// ---- 工具栏按钮 ID ----
#define IDB_TOOLBAR_MAIN         130
#define ID_TOOLBAR_OPEN          (40000 + 0)
#define ID_TOOLBAR_ZOOM_IN       (40000 + 1)
#define ID_TOOLBAR_ZOOM_OUT      (40000 + 2)
#define ID_TOOLBAR_ZOOM_FIT      (40000 + 3)
#define ID_TOOLBAR_FILTER        (40000 + 4)
#define ID_TOOLBAR_R_DETECT      (40000 + 5)

// ---- 新增过滤器对话框 ----
#define IDD_FILTER_DIALOG        200
#define IDC_EDIT_LOWCUT          201
#define IDC_EDIT_HIGHCUT         202
#define IDC_EDIT_WINDOW          203
#define IDC_BTN_APPLY            204
#define IDC_STATIC_LOW           205
#define IDC_STATIC_HIGH          206
#define IDC_STATIC_WINDOW        207
#define IDC_RADIO_RAW            208
#define IDC_RADIO_FILTERED       209

// ---- 字符串资源 ID ----
#define IDS_APP_TITLE            100
#define IDS_FILE_FILTER          101
#define IDS_READY                102
#define IDS_HEART_RATE_FMT       103
#define IDS_CURSOR_POS_FMT       104

// ---- 图标 ----
#define IDI_ICON_MAIN            300
#define IDI_ICON_DOC             301

// ---- 加速键 ----
#define IDR_ACCELERATOR1         400

// ---- 主框架资源 ----
#define IDR_MAINFRAME            128
