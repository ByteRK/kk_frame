# kk_frame 模板开发指南

`kk_frame` 是基于 [Cdroid](https://gitee.com/houstudio/cdroid) 的嵌入式 UI 应用项目模板，
也是由它派生出的项目的通用开发指南。

本文件描述**框架与模板层面**的内容：构建方式、目录职责、模块能力、功能开关、脚本与板端部署。
派生项目请保留本文件，项目自身的说明写在 `README.md`（由 `docs/README_BASE.md` 生成）。

---

## 1. 环境要求

| 依赖 | 说明 |
| --- | --- |
| 系统 | Linux（推荐 Ubuntu / WSL2），需具备 X11 或目标板显示环境 |
| 编译器 | 支持 C++11 的 GCC / Clang |
| CMake | >= 3.13 |
| Python | 3.x（资源处理与板级脚本使用） |
| Cdroid | 项目必须位于 Cdroid 源码树的 `apps/` 目录下 |

安装 Python 依赖（资源处理脚本需要）：

```bash
pip install -r requirements.txt
```

> 项目依赖上层 Cdroid 的编译产物与工具链配置，脱离 Cdroid 源码树无法独立构建。

---

## 2. 基于模板创建新项目

在模板目录执行：

```bash
python3 build.py
```

交互流程与行为：

1. 输入项目名：只允许字母、数字、下划线，且不能以数字开头。
2. 目标目录固定为**模板的父目录**下与项目同名的目录（即 `apps/<新项目名>`）。
3. 目标目录已存在时会询问是否删除后继续。
4. 复制模板内容，排除本地缓存与本机构建产物（`.venv`、`tmp`、`dist`、`__pycache__`、
   `config/app_version.h`、`config/fonts_info.h`、`fonts/fonts.conf`、`R.h`、
   `library/*/lib_*_config.h`、拼音解压数据等），这些内容会在首次构建时重新生成。
5. 自动替换项目名：
   - `README.md`：由 `docs/README_BASE.md` 替换占位符后生成；
   - `R.h`、`CMakeLists.txt`；
   - `src/app/`、`src/widgets/` 中的 `kk_frame` 标识（`kk_frame/` 路径形式会被保留）；
   - 脚本与板端配置：`fastCheck.sh`、`script/build.sh`、`script/release.sh`、
     `script/x64.sh`、`script/README.md`、`board/script/name.sh`、`script/kk_frame.ps1`。
6. 文件名中带项目名的文件会被重命名，例如 `script/kk_frame.ps1` → `script/<新项目名>.ps1`。
7. 替换完成后扫描新项目，列出仍包含模板名的文件，便于确认。
8. Git 处理：可选择保留模板提交历史，或删除 `.git` 重新初始化；重新初始化时可选把
   模板仓库添加为**只读** remote `kk_frame`，并可选配置新项目的 `origin`。

`.vscode/launch.json` 使用 `${workspaceFolder}` 与 `${workspaceFolderBasename}` 推导
可执行文件路径，因此不依赖项目名与用户名，无需替换。

有意保留模板名的文件（不会进入残留提示）：`README.md`（模板门面）、`docs/GUIDE.md`、
`build.py`、`script/brand_encode.py`、`library/watchdog/README.md`、
`.agents/build_validation.md`。

创建完成后需要手动确认的事项：

- `config/series_info.h` 的硬件参数；
- `docs/README_BASE.md` 中的项目简介与功能清单，然后同步生成 `README.md`；
- `script/<新项目名>.ps1` 中的服务器地址与共享路径；
- 若沿用默认模板地址，检查 Git remote 是否符合预期。

---

## 3. 构建

### 3.1 x64 快速编译

```bash
./fastCheck.sh
```

脚本会复用或创建 Cdroid 根目录下的 `outX64-Debug` 构建目录，编译当前项目，并把可执行文件、
`cdroid.pak`、`libcdroid.so`、`libtvhal.so` 复制到 `dist/`。

| 参数 | 说明 |
| --- | --- |
| 无 | 编译当前项目（x64 Debug） |
| `-R` | 仅运行已有编译产物，不编译 |
| `-r` | 编译完成后运行 |
| `-g` | 编译完成后使用 gdb 运行 |
| `-t` | touch `CMakeLists.txt`，强制 CMake 重新配置 |
| `-jN` | 指定并行编译数，例如 `-j8` |

### 3.2 板端 Release 构建

```bash
./script/build.sh          # 默认 sigma / outSIGMA-Release
./script/build.sh -rm      # 删除输出目录后重新配置
```

### 3.3 手动编译

```bash
cd ../../outX64-Debug
make <project> -j8
```

### 3.4 构建流程

1. `CMakeLists.txt` 收集 `main.cc` 与 `src/**/*.{c,cc,cpp}` 源文件。
2. `cmake/features.cmake` 根据功能开关追加宏定义、头文件路径与依赖库。
3. `library/` 编译内置子库，并生成 `library_config.h`。
4. `cmake/prebuild.cmake` 执行 `script/prebuild.sh`：
   - `date2ver` 生成 `config/app_version.h`；
   - `script/fonts.sh` 生成 `config/fonts_info.h` 与 fontconfig 配置。
5. `CreatePAK` 将 `assets/` 打包为 `<project>.pak`，并生成控件 ID 头文件 `R.h`。

---

## 4. 目录结构

```text
<project>/
├── main.cc                  # 程序入口
├── CMakeLists.txt           # 顶层构建配置
├── R.h                      # 自动生成的控件 ID
├── build.py                 # 模板初始化脚本
├── date2ver                 # 版本号生成脚本
├── fastCheck.sh             # x64 快速编译 / 运行脚本
├── requirements.txt         # Python 资源处理依赖
├── assets/                  # 界面资源
│   ├── animator/            # 动画
│   ├── color/               # 颜色状态
│   ├── drawable/            # 图片与背景
│   ├── layout/              # 页面与窗口布局
│   ├── mipmap/              # 图标
│   └── values/              # 颜色、字体、样式、ID
├── board/                   # 板端部署目录
├── cmake/                   # 功能开关与预构建配置
├── config/                  # 项目配置头文件
├── docs/                    # 项目文档
├── fonts/                   # 字体文件与 fontconfig
├── lang/                    # 多语言资源
├── library/                 # 内置子库
├── script/                  # 构建与资源处理脚本
├── src/                     # 应用源码
└── dist/                    # 编译产物汇总（生成）
```

---

## 5. 应用层 `src/app`

| 目录 | 说明 |
| --- | --- |
| `managers/` | 业务管理器：配置、Wi-Fi、音频、HTTP、消息、Telnet、线程、定时器、窗口、任务、鉴权、内存等 |
| `page/core/` | `Page`、`Popup`、`Wind` 等基类与页面 ID 定义 |
| `page/view/` | 具体业务页面 |
| `page/components/` | 可复用窗口组件：Logo、屏保、侧边栏、Toast、弹窗、键盘、童锁、遮罩等 |
| `protocol/` | MCU 与按键通讯协议、协议 ID 及管理器 |
| `data/` | 全局数据、历史数据、统计数据及结构定义 |
| `app_common.h` | 应用通用头文件 |

---

## 6. 通讯框架 `src/comm`

三层结构：

```text
业务管理器（McuMgr / BtnMgr ...）
        │
        ▼
PacketChannel<Transport>
        │  编码、组包、校验、去重、命令分发
        ▼
Transport（TcpClient / TcpServer / UartClient）
        │
        ▼
TCP socket / POSIX UART
```

| 目录 | 说明 |
| --- | --- |
| `core/` | 统一原始通讯接口与跨线程事件分发 |
| `tcp/` | 连接、重连、监听与多客户端管理 |
| `uart/` | 串口配置与读写 |
| `packet/` | 协议包编码、流式解码、缓存复用与业务分发 |

详见 [`src/comm/README.md`](../src/comm/README.md)。

---

## 7. 通用能力

| 目录 | 说明 |
| --- | --- |
| `src/class/` | 自动重启、自动保存、按钮组、农历、NTP 同步、历史 / 统计存储、Wi-Fi 适配等 |
| `src/widgets/` | 弧形进度条、高斯模糊视图 / Drawable、网络图片、时间文本、视频视图、Picker 等 |
| `src/utils/` | 参数、品牌、Cdroid、校验、编码、环境、文件、JSON、数学、网络、项目、字符串、系统、时间、版本等工具 |
| `src/template/` | 单例等通用模板 |
| `src/wifi/` | Wi-Fi HAL 与 wpa_supplicant 客户端封装 |

---

## 8. 内置子库 `library/`

| 子库 | 说明 |
| --- | --- |
| `fastGaussianBlur/` | 快速高斯模糊 |
| `gaussFilter/` | 高斯滤波 |
| `gaussianBlur/` | 高斯模糊 |
| `hvTools/` | 网络工具 |
| `keyboard/` | 屏幕键盘（含拼音输入） |
| `md5/` | MD5 工具 |
| `ntpClient/` | NTP 对时 |
| `video/` | 视频播放（含 rk3506 / x64 / generic HAL） |
| `watchdog/` | 硬件看门狗喂狗 |
| `wpaCtrl/` | wpa_supplicant 控制 |

新增子库参考 `library/CMakeLists.txt.template`；启用开关位于 `library/options.cmake`。

---

## 9. 配置

### 9.1 功能开关 `cmake/features.cmake`

| 开关 | 说明 |
| --- | --- |
| `ENABLED_WIFI` | Wi-Fi 支持，同时启用 CURL；x64 下默认开启 |
| `ENABLED_OPENSSL` | CURL 显式链接 OpenSSL |
| `ENABLED_KEYBOARD` | 屏幕键盘 |
| `ENABLED_KEYBOARD_PINYIN` | 键盘拼音输入 |
| `ENABLED_GAUSS_VIEW` | 高斯模糊视图 |
| `ENABLED_GAUSS_DRAWABLE` | 高斯模糊 Drawable（依赖 Pixman） |
| `ENABLED_FILE_SYSTEM` | 文件系统支持；关闭后 `FileUtils` 全部接口失效（返回 `false` / 空操作），可用 `FileUtils::isSupported()` 判断 |
| `ENABLED_JSON` | JsonCpp 支持；JsonCpp 为常驻依赖，关闭会直接 CMake 报错 |
| `ENABLED_PIXMAN` | Pixman 支持 |
| `ENABLED_VIDEO` | 视频支持 |
| `ENABLED_ALSA` | ALSA 音频播放 |

> 开关生成的编译宏统一为 `-DENABLE_XXX=1`（配合 `quick_define.h` 的 `ENABLED()` / `DISABLED()` 使用）。
> 编辑器（IntelliSense）的开关宏统一在 `.vscode/c_cpp_properties.json` 的 `defines` 中维护，
> 建议与 `features.cmake` 的当前取值保持一致，源码中不再需要 `defined(__VSCODE__)` 之类的兜底判断。

### 9.2 子库开关 `library/options.cmake`

部分开关跟随功能开关（如 `USE_WPACTRL` 跟随 `ENABLED_WIFI`），其余可独立配置。

### 9.3 配置头 `config/`

| 文件 | 说明 | 是否自动生成 |
| --- | --- | --- |
| `series_info.h` | 硬件参数 | 否 |
| `config_info.h` | 资源路径与默认配置 | 否 |
| `app_version.h` | 版本信息 | 是（`date2ver`） |
| `fonts_info.h` | 字体配置 | 是（`script/fonts.sh`） |
| `brand_info.h` | 署名与版权信息 | 是（`script/brand_encode.py`） |

> 自动生成的文件请勿直接编辑，下次构建会被覆盖。

品牌署名使用编码工具维护：

```bash
python3 script/brand_encode.py --show
python3 script/brand_encode.py --set "owner|year|project|repo|license"
```

---

## 10. 资源与多语言

- `assets/layout/`：页面与窗口布局；
- `assets/drawable/`、`assets/mipmap/`：图片与背景；
- `assets/values/`：颜色、字体、样式与 ID；
- `assets/animator/`、`assets/color/`：动画与颜色状态；
- `lang/*.json`：多语言文本，使用 `script/checkLang.py` 校验键一致；
- `fonts/`：字体与 `fonts.conf`，构建时生成 `config/fonts_info.h`。

新增控件会在下一次构建时写入 `R.h`。

---

## 11. 脚本清单

| 脚本 | 说明 |
| --- | --- |
| `fastCheck.sh` | x64 Debug 快速编译、运行 |
| `script/build.sh` | 板端 Release 编译并汇总产物 |
| `script/prebuild.sh` | 构建前准备（版本号、字体配置） |
| `date2ver` | 根据日期与 Git 信息生成版本号 |
| `script/fonts.sh` | 生成 fontconfig 配置 |
| `script/checkLang.py` | 校验多语言 JSON 键及数量一致 |
| `script/check_image_size.py` | 批量校验图片尺寸 |
| `script/fill_bg.py` | 为 PNG 填充底色、去透明、去元数据 |
| `script/png_compress.py` | 批量压缩 PNG |
| `script/png_to_bmp32.py` | PNG 转 32 位 BMP |
| `script/audio_gain_batch.py` | 批量调整音频增益（依赖 ffmpeg） |
| `script/ota.py` | 为 OTA 包添加 MD5 校验头 |
| `script/brand_encode.py` | 生成 / 查看品牌署名信息 |
| `script/cdroid_patch.sh` | 生成 Cdroid 框架修改补丁 |
| `script/project_patch.sh` | 生成项目自身修改补丁 |
| `script/release.sh` | 打包并输出到 `board/` |
| `script/x64.sh` | x64 环境辅助脚本 |
| `build.py` | 从模板创建新项目 |

---

## 12. 板端部署 `board/`

`board/` 是可直接复制到设备的运行目录：

| 路径 | 说明 |
| --- | --- |
| `app.sh` | 启动入口 |
| `script/` | 启动、停止、重启、检查、环境变量、看门狗、wpa 等脚本 |
| `data/` | 设备配置（`devices.xml`） |
| `fonts/` | 设备端 fontconfig |
| `lib/` | 设备端依赖库 |
| `input-devices.xml` | 输入设备配置 |
| `watchdog.conf` | 看门狗配置 |

---

## 13. 文档与维护约定

### 13.1 文档关系

| 文件 | 作用 |
| --- | --- |
| `README.md` | 派生项目的项目说明，由 `build.py` 从 `docs/README_BASE.md` 生成，**不要手改** |
| `docs/README_BASE.md` | 项目说明模板，维护占位符 `{{project_name}}`、`{{create_time}}`、`{{commit_id}}` |
| `docs/GUIDE.md` | 本文件，框架与模板层面的通用说明，派生项目原样保留 |
| `docs/wpa_connect.md` | wpa_supplicant 连接示例 |
| `src/comm/README.md` | 通讯框架详细说明 |
| `src/app/page/components/README.md` | 窗口组件说明 |

修改项目说明的流程：编辑 `docs/README_BASE.md` → 同步 `README.md`（或重新初始化项目）。
修改框架细节的流程：编辑 `docs/GUIDE.md`。

### 13.2 模板维护注意

- 新增脚本或目录时，同步更新本文档第 4、11 节。
- 新增需要随项目名变化的文件时，同步加入 `build.py` 的 `PROJECT_NAME_FILES`。
- 新增本机生成物时，同步加入 `build.py` 的 `DEFAULT_IGNORE`，避免复制进新项目。
- 修改 `build.py` 的替换范围时，同步更新第 2 节。
- 自动生成文件（`R.h`、`config/app_version.h`、`config/fonts_info.h`、
  `config/brand_info.h`、`library/library_config.h`）不应提交手工修改。
- `.agents/` 存放 AI 协作相关的任务级指引，不属于运行时内容。

---

## 声明

**Base On [Cdroid](https://gitee.com/houstudio/cdroid)**

**Power By [Ricken](https://ricken.cn)**
