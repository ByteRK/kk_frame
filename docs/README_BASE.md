# {{project_name}}

> 基于 [Cdroid](https://gitee.com/houstudio/cdroid) 的嵌入式 UI 应用
> Powered by [Ricken](https://ricken.cn)

| 项目 | 内容 |
| --- | --- |
| 项目名称 | `{{project_name}}` |
| 创建时间 | {{create_time}} |
| Cdroid 版本 | `{{commit_id}}` |
| 开源协议 | GPL-2.0 |

## 项目简介

<!-- 填写：项目背景、目标设备、主要使用场景 -->

## 功能特性

<!-- 按项目实际情况保留或删除 -->

- [ ] 页面、弹窗与窗口层级管理
- [ ] 屏幕键盘（含拼音输入）
- [ ] MCU 串口通讯
- [ ] TCP 通讯
- [ ] Wi-Fi 配网与控制
- [ ] 多语言
- [ ] 视频播放
- [ ] OTA 升级
- [ ] 看门狗托管
- [ ] 工厂模式

## 快速开始

```bash
./fastCheck.sh        # 编译（x64 Debug）
./fastCheck.sh -r     # 编译并运行
./fastCheck.sh -R     # 仅运行已有编译产物
./script/build.sh     # 板端 Release 构建
```

环境要求、构建流程与全部参数见 [docs/GUIDE.md](docs/GUIDE.md)。

## 项目结构

```text
{{project_name}}/
├── assets/                  # 界面资源
├── board/                   # 板端部署目录
├── cmake/                   # 功能开关与预构建配置
├── config/                  # 项目配置头文件
├── docs/                    # 项目文档
├── fonts/                   # 字体
├── lang/                    # 多语言资源
├── library/                 # 内置子库
├── script/                  # 构建与资源处理脚本
├── src/
│   ├── app/                 # 应用层
│   │   ├── data/            # 数据结构与持久化
│   │   ├── managers/        # 业务管理器
│   │   ├── page/            # 页面与窗口
│   │   ├── protocol/        # 通讯协议
│   │   └── app_common.h     # 应用通用头文件
│   ├── class/               # 通用业务类
│   ├── comm/                # 通讯框架
│   ├── template/            # 通用模板
│   ├── utils/               # 工具集
│   ├── widgets/             # 自定义控件
│   └── wifi/                # Wi-Fi 封装
└── main.cc                  # 程序入口
```

框架层目录、内置子库与各模块职责见 [docs/GUIDE.md](docs/GUIDE.md)。

## 项目配置

| 位置 | 说明 |
| --- | --- |
| `config/series_info.h` | 硬件参数 |
| `config/config_info.h` | 资源路径与默认配置 |
| `cmake/features.cmake` | 功能开关（Wi-Fi、键盘、视频、JSON 等） |
| `library/options.cmake` | 内置子库开关 |

## 项目文档

- [docs/GUIDE.md](docs/GUIDE.md)：框架与模板开发指南
- [docs/wpa_connect.md](docs/wpa_connect.md)：wpa_supplicant 连接示例
- [src/comm/README.md](src/comm/README.md)：通讯框架说明
- [src/app/page/components/README.md](src/app/page/components/README.md)：窗口组件说明

## 声明

**Base On [Cdroid](https://gitee.com/houstudio/cdroid)**

**Power By [Ricken](https://ricken.cn)**
