# X11 调试环境说明

本目录用于解决「在 VS Code 里调试本程序时，`DISPLAY` 该填什么」的问题。

## 背景：为什么不能把 DISPLAY 写死

程序是 X11 图形程序，运行时需要连接到 X 服务器。开发时 X 服务由 **MobaXterm** 提供，
它在本机上的表现和常见 Linux 桌面不同：

| 事实 | 说明 |
|---|---|
| 不是 unix socket | `/tmp/.X11-unix` 为空；MobaXterm 走的是 **TCP 回环转发**，`DISPLAY=localhost:N.0` 对应端口 `127.0.0.1:6000+N` |
| display 号是动态的 | 编号由 sshd 按 SSH 会话分配，**重连或新开会话就会变**。`xauth list` 里常年堆积着一堆历史编号（10/11/12/14/16~19…），但只有当前活跃的少数几个能连上 |
| VS Code 环境里没有 DISPLAY | VS Code Server 是它自己新开的 SSH 会话，MobaXterm 的转发不作用于它，环境里只有 `SSH_CLIENT`/`SSH_CONNECTION`。所以**必须显式指定**，不能指望继承 |
| 端口在监听 ≠ 可用 | 其它会话/用户占用的端口会返回 `MoTTY X11 proxy: Authorisation not recognised`。必须用 `XOpenDisplay` 实测才能确认 |

因此本目录的做法是：**每次启动调试前实时探测**，而不是写死一个编号。

## 目录结构

```
.vscode/x11/
├── README.md          本文件
├── displays.py        探测当前可用的 DISPLAY（唯一实现）
├── display.env        探测/选择的结果，供 envFile 使用（已 gitignore）
├── install.py         打包并安装扩展（一条命令搞定）
└── picker/            弹窗选择扩展的源码
    ├── package.json
    └── extension.js
```

## 两种模式

探测/选择逻辑只有一份（`displays.py`），区别只在「谁来调用它」。

两种模式在 `launch.json` 里**并存为三个配置**，直接用「运行和调试」下拉选即可，
不需要改文件：

| 配置名 | 模式 | DISPLAY 来源 | preLaunchTask |
|---|---|---|---|
| `快速运行` | B 自动 | `envFile` → `display.env` | `FastCheck prepare X11` |
| `编译并运行` | B 自动 | `envFile` → `display.env` | `FastCheck build & prepare X11` |
| `编译并运行(带X11选择)` | A 弹窗 | `environment` → 弹窗实探 | `FastCheck build` |

两种模式本身的差别：

| | 模式 A：弹窗选择 | 模式 B：直接探测（日常用这个） |
|---|---|---|
| 交互 | 启动调试时弹下拉框，手动选 | 无交互，自动取一个 |
| 多显示时 | 由你选 | 取**编号最大（最新会话）**那个 |
| 依赖 | 需要已安装 `picker` 扩展 | 无额外依赖，只要 `python3` + `libX11` |
| 实现 | `inputs(type=command)` + `${input:x11Display}` | `envFile` + `preLaunchTask` 先跑探测 |
| 适用 | 同时开了多个 MobaXterm 窗口，需要区分 | 日常使用，只开一个窗口 |

两种模式共用同一个 `display.env`：模式 B 每次启动前刷新它，模式 A 把你选的值写进去。
所以两者互不干扰，`display.env` 顺便也充当了模式 A 的「上次使用」记忆。

### 为什么模式 B 必须先探测

`envFile` 是**静态读取**的，不会自己去探测。所以模式 B 的配置都挂了 `preLaunchTask`
在启动前刷新 `display.env`；缺了这个任务就会一直用文件里的旧值，会话变化后会
**安静地连不上 X 服务器**（现象是程序启动即报 X 连接失败，且不会自动恢复）。

### 新增配置时怎么填

想再加一个调试配置，照下表抄对应模式的字段即可：

```jsonc
// 模式 B：自动探测
"envFile": "${workspaceFolder}/.vscode/x11/display.env",
"preLaunchTask": "FastCheck prepare X11",   // 需要同时编译则用 "FastCheck build & prepare X11"
```

```jsonc
// 模式 A：弹窗选择。需要文件末尾存在下面的 inputs，否则启动会直接报错
"environment": [
    { "name": "DISPLAY", "value": "${input:x11Display}" }
],
"preLaunchTask": "FastCheck build",   // 只负责编译，探测由扩展弹窗时完成
```

```jsonc
"inputs": [
    {
        "id": "x11Display",
        "type": "command",
        "command": "pickX11Display"
    }
]
```

模式 A 需要扩展已安装，未装就按 F5 会报 `command 'pickX11Display' not found`：

```
命令面板 → Tasks: Run Task → FastCheck install X11 picker
```

> 注意：`${input:...}` 的弹窗发生在 `preLaunchTask` **之前**，所以模式 A 的
> `preLaunchTask` 不需要探测。反过来，模式 B 也无法"先刷新候选再弹窗让用户选"——
> 想要弹窗就只能用模式 A 的 `type: command`。

## 常用任务

`命令面板 → Tasks: Run Task`：

| 任务 | 作用 | 被谁调用 |
|---|---|---|
| `FastCheck prepare X11` | 探测可用 DISPLAY 并写入 `display.env` | `快速运行` 的 preLaunchTask |
| `FastCheck build & prepare X11` | 先探测，再执行 `FastCheck build` | `编译并运行` 的 preLaunchTask |
| `FastCheck install X11 picker` | 打包 VSIX 并安装扩展 | 手动执行（模式 A 依赖它） |

## 命令行用法

```bash
# 列出当前所有可用的 DISPLAY（会话新的在前）
python3 .vscode/x11/displays.py

# 探测并写入 display.env（失败时保留旧值，不破坏已有配置）
python3 .vscode/x11/displays.py --write

# 打包并安装扩展
python3 .vscode/x11/install.py
```

两个脚本**无论成功失败都以 0 退出**，避免探测不到 X11 时中断构建。

## 探测原理

1. **收集候选**：`xauth list` 里登记过的 display 号，加上本机 `127.0.0.1:6000~6063` 中实际在监听的端口。
2. **逐个实测**：用 `XOpenDisplay()`（与程序运行期完全相同的 API）尝试连接，只有真正连得上的才保留。
   每个实测都跑在**子进程**里并带超时（3s，见下），失败/超时都当作不可用；多个候选并行实测，整体耗时不超过一个超时周期。
3. **排序**：按编号降序，即**最新会话优先**。

> 为什么要放子进程：`XOpenDisplay` 在建立连接阶段**没有超时**。转发通道已经死掉的
> “僵尸”端口（`ss` 里在 LISTEN、连上也不被拒绝）会 accept 连接却永不回握手指包，
> 此时程序会永远阻塞在 `poll()` 上，而且进程内无法打断（SIGALRM 也不行：Xlib 对
> EINTR 是内部重试）。只有“子进程 + kill”能可靠把整体耗时控制在 3s 内。

## 排查

| 现象 | 原因与处理 |
|---|---|
| `command 'pickX11Display' not found` | 扩展未安装、未重载，或装到了别的 profile。跑一次 `FastCheck install X11 picker` 任务，再执行 `Developer: Reload Window` |
| 弹窗里没有你要的 display | 确认 MobaXterm 的 X11 转发已开启；手动跑 `displays.py` 看探测结果 |
| 提示 `Authorisation not recognised` | 该端口属于别的会话，不可用。这是正常现象，探测会自动跳过 |
| 手动跑 `displays.py` 长时间无输出 | 旧版表现为卡死（僵尸端口永不应答且 `XOpenDisplay` 无超时）。现已改为子进程 + 3s 超时，最坏 3s 返回；若始终探测不到，重启 MobaXterm 会话恢复 X11 转发 |
| `install.py` 长时间无响应 | `code --install-extension` 是 IPC 调用，会转发给运行中的窗口并阻塞等待。脚本已设超时（安装 120s / 校验 30s），超时会提示；稍后重试或改用 `Extensions: Install from VSIX...` |
| 程序连不上 X 服务器 | 用 `displays.py` 确认当前有哪些可用，然后重选或重新探测 |

## 维护须知

**改了 `picker/` 下的代码后，必须重新安装扩展才会生效**：

```
Tasks: Run Task → FastCheck install X11 picker → Developer: Reload Window
```

原因是扩展是通过 VSIX **安装**（而非目录引用）到 `~/.vscode-server/extensions/` 的。
不用软链接是因为 VS Code 扫描扩展目录时会跳过软链接。

另外两点：

- `install.py` 里的 publisher/ID 从 `picker/package.json` 推导并**强制转小写**——VS Code 会把
  publisher 规范化为小写（`ricken.x11-picker`），不要硬编码 ID。
- 已 gitignore 的文件：`display.env`（本机/会话相关）、`picker.vsix`（构建产物）。
