# ECG Viewer — 心电信号波形显示与交互分析系统

> **Windows程序设计 期末作业**  
> 四川大学 · 医学信息工程 · 2024级  
> 基于 MFC 框架开发，使用 GDI 图形绘制心电波形，调用自编 DLL 实现滤波算法。

---

## 📸 程序截图



<img width="1426" height="1052" alt="image" src="https://github.com/user-attachments/assets/84d0df39-10e4-403a-9a08-f21115151a1c" />


---

## ✨ 功能特性

- 📂 **CSV 数据加载** — 支持单列（自动生成时间轴）和双列（时间+幅值）格式，智能跳过标题行
- 🖼️ **波形可视化** — 蓝色原始信号 + 红色滤波信号叠加显示，自适应网格背景和坐标轴标注
- 🔬 **带通滤波** — 0.5–40Hz 级联滤波器，去除基线漂移和高频噪声（ECGFilterDLL.dll 实现）
- 💓 **R 波检测** — 自适应阈值 + 不应期保护的 R 峰检测算法，自动计算平均心率
- 🖱️ **鼠标交互** — 左键拖拽平移、右键框选测 RR 间期、Ctrl+滚轮缩放、光标坐标实时跟踪
- ⌨️ **键盘操作** — 方向键平移、+/- 缩放、空格播放动画、F 适应窗口、R 重置、G 切换网格
- 🎬 **动画播放** — 基于 WM_TIMER 的自动滚动播放（20fps），绿色播放头指示当前位置
- 📊 **状态监控** — 多窗格状态栏实时显示光标位置（秒/mV）和心率（BPM）
- 🧩 **双缓冲绘制** — 内存 DC 渲染后一次性输出，消除波形滚动时的闪烁

---

## 🧰 课堂知识覆盖

本程序综合运用了 **Windows程序设计** 课程的全部六大知识点：

| 知识点 | 程序中的应用 |
|--------|-------------|
| **MFC 动态链接库** | 自编 ECGFilterDLL.dll，导出 C 接口函数，主程序通过 `LoadLibrary`/`GetProcAddress` 动态调用 |
| **MFC 图形基础** | SDI Doc/View 架构：`CWinApp` → `CDocument` → `CScrollView` → `CFrameWnd` 完整链 |
| **MFC 控件** | `CToolBar`（6 个文字按钮）、`CStatusBar`（6 窗格实时更新）、`CFileDialog` 文件选择 |
| **GDI 图形绘制** | `CDC` 双缓冲、`CPen`（实线/虚线/颜色）、`CBrush`（填充）、`CFont`/`TextOut`（标签）、自适应网格 |
| **键盘交互** | `OnKeyDown` 处理 8 种按键：方向键平移、`+`/`-` 缩放、空格动画、F/R/G 快捷操作 |
| **鼠标交互** | `OnMouseMove` 坐标跟踪、`OnLButtonDown/Up` 拖拽平移（`SetCapture`）、`OnRButtonDown/Up` 框选测量、`OnMouseWheel` 缩放 |

---

## 🛠️ 开发环境

| 工具 | 版本 |
|------|------|
| Visual Studio | 2026 (v18) |
| 平台工具集 | v145 |
| MFC | 动态链接 (UseOfMfc=Dynamic) |
| 字符集 | Unicode |
| 目标平台 | x64 |
| 语言 | C++ (MFC + GDI + 标准库) |

---

## 📁 项目结构

```
ECGViewer/
├── ECGViewer.sln                 ← VS 解决方案（双击打开）
├── ECGFilterDLL/                 ← 动态链接库子项目
│   ├── ECGFilterDLL.h/.cpp      → 滑动平均、带通滤波、R 波检测
│   ├── dllmain.cpp              → DLL 入口点
│   └── ECGFilterDLL.vcxproj
├── ECGViewer/                    ← MFC 主程序
│   ├── ECGViewer.h/.cpp         → CWinApp 应用类
│   ├── MainFrm.h/.cpp           → 主框架（工具栏 + 状态栏）
│   ├── ECGViewerDoc.h/.cpp      → 文档类（数据管理）
│   ├── ECGViewerView.h/.cpp     → ★ 视图类（GDI 绘制 + 交互 + DLL 调用）
│   ├── ECGData.h/.cpp           → 心电数据模型（CSV 解析）
│   ├── Resource.h               → 资源 ID 定义
│   ├── ECGViewer.rc             → 资源文件（菜单/对话框/字符串表）
│   └── res/                     → 图标、工具栏位图
└── sample_data/                  ← 心电信号 CSV 样本
    ├── ecg_raw_30s.csv          → 30 秒原始信号（带噪声，360Hz）
    ├── ecg_raw_5s.csv           → 5 秒片段（快速测试）
    ├── ecg_filtered_30s.csv     → 30 秒滤波后信号
    └── ecg_filtered_5s.csv      → 5 秒滤波后片段
```

---

## 🚀 编译运行

1. 用 **Visual Studio 2026** 打开 `ECGViewer.sln`
2. 解决方案有两个项目，先右键 **ECGFilterDLL → 生成**（编译 DLL）
3. 再右键 **ECGViewer → 设为启动项目 → 生成**
4. 按 `F5` 运行
5. 点击 **文件 → 打开**，选择 `sample_data/ecg_raw_30s.csv` 即可看到波形

> 两个项目编译输出到同一目录 `x64/Debug/`，DLL 和 EXE 自动在一起。

---

## 🎬 演示视频脚本（3-5 分钟）

1. 启动程序 → 展示界面布局（菜单栏 + 工具栏 + 状态栏）
2. **文件 → 打开** → 选择 `ecg_raw_30s.csv`
3. 🖱️ **鼠标操作**：Ctrl+滚轮缩放 → 左键拖拽平移 → 右键框选两个 R 波测量 RR 间期
4. ⌨️ **键盘操作**：方向键平移 → `+`/`-` 缩放 → `G` 切换网格 → 空格播放动画
5. **工具 → 带通滤波** → 红色滤波波形叠加显示
6. **工具 → R 波检测** → 橙色 R 波标记 + 状态栏显示平均心率
7. 按 `F` 适应窗口 → 展示完整 30 秒波形

---

## 📝 报告

完整实验报告（6 页 Word 文档）见 `ECGViewer_实验报告.docx`。

---

## 📄 许可

本项目为课程作业，仅供学习参考。
