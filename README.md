# kk_frame

基于 [Cdroid](https://gitee.com/houstudio/cdroid) 的嵌入式 UI 应用项目模板。

本仓库是**模板**，不是一个可直接交付的应用。用它创建新项目后，`README.md` 会被替换为
项目说明，模板与框架层面的细节统一记录在 [`docs/GUIDE.md`](docs/GUIDE.md)。

## 快速创建项目

```bash
cd cdroid/apps/kk_frame
python3 build.py
```

脚本会在 `apps/` 下创建同名新目录，完成项目名替换（含脚本与板端配置）、`R.h` /
`CMakeLists.txt` 重写、`README.md` 生成、文件名重命名（`script/kk_frame.ps1`）以及
Git 远程配置，并在最后列出仍包含模板名的文件供确认。

创建完成后需手动确认：

- `config/series_info.h` 的硬件参数；
- `docs/README_BASE.md` 中的项目简介与功能清单，并同步 `README.md`；
- `script/<新项目名>.ps1` 中的服务器地址与共享路径。

## 模板包含

| 能力 | 位置 |
| --- | --- |
| 页面、弹窗与窗口层级管理 | `src/app/page/` |
| 业务管理器（配置、Wi-Fi、音频、HTTP、任务等） | `src/app/managers/` |
| MCU / 按键通讯协议 | `src/app/protocol/` |
| 通讯框架（TCP / 串口 / 协议包） | `src/comm/` |
| 通用业务类与自定义控件 | `src/class/`、`src/widgets/` |
| 工具集 | `src/utils/` |
| 内置子库（键盘、视频、看门狗、高斯模糊、NTP、wpa 等） | `library/` |
| 构建脚本与资源处理工具 | `script/`、`date2ver`、`fastCheck.sh` |
| 板端部署目录 | `board/` |

## 直接编译本模板

```bash
./fastCheck.sh        # 编译（x64 Debug）
./fastCheck.sh -r     # 编译并运行
```

`kk_frame` 必须在 Cdroid 源码树的 `apps/` 目录下才能构建。

## 文档

| 文件 | 作用 |
| --- | --- |
| [`docs/GUIDE.md`](docs/GUIDE.md) | 模板与框架开发指南：环境、构建、目录、模块、配置开关、脚本、板端部署 |
| [`docs/README_BASE.md`](docs/README_BASE.md) | 派生项目的 `README.md` 模板（含占位符，由 `build.py` 生成） |
| [`docs/wpa_connect.md`](docs/wpa_connect.md) | wpa_supplicant 连接示例 |
| [`src/comm/README.md`](src/comm/README.md) | 通讯框架说明 |
| [`src/app/page/components/README.md`](src/app/page/components/README.md) | 窗口组件说明 |
| [`AGENTS.md`](AGENTS.md) | AI 协作入口指引 |

> 自动生成的文件（`R.h`、`config/app_version.h`、`config/fonts_info.h`、
> `config/brand_info.h`、`library/library_config.h`）不要手工修改。

## 声明

**Base On [Cdroid](https://gitee.com/houstudio/cdroid)**

**Power By [Ricken](https://ricken.cn)**
