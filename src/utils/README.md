# utils 速查索引

本目录是项目底座通用工具库，分三层：

| 层 | 目录 | 依赖 | 可独立单测 |
|---|---|---|---|
| base | `base/` | 仅标准库 / libc | ✅ |
| platform | `platform/` | 系统调用（POSIX） | ✅ |
| integration | `integration/` | 三方库（jsoncpp / 图像算法 / cdroid / 库配置） | ❌ |

> 新增工具函数时：先判断**输入域**（时间？文本？文件？网络？），放进对应模块的既有分区，
> 而不是为单个函数新建头文件。

---

## 1. 按需求找函数

### 常用高频

| 我想… | 用 |
|---|---|
| 去掉首尾空白 | `StringUtils::trim` / `trimInPlace` |
| 拆分 / 拼接字符串 | `StringUtils::split` / `splitAny` / `join` |
| 字符串转数字（严格整串校验） | `StringUtils::toInt` / `toInt64` / `toDouble` |
| 生成格式化字符串 | `StringUtils::format` |
| 格式化字节数组 / 容量单位 | `FormatUtils::hexStr` / `sizeVisualization` |
| 判断文件/目录是否存在 | `FileUtils::exists` / `isDir` / `isFile` |
| 读写文本文件 | `FileUtils::read` / `write` / `writeAtomic` |
| 建目录 / 删目录 | `FileUtils::mkdir` / `clearDir` / `removeDir` |
| 查分区剩余空间 | `FileUtils::getUsage` |
| 解析 JSON | `JsonUtils` |

### 文本与编码

| 我想… | 用 |
|---|---|
| 按字符数截断（不会切坏汉字） | `StringUtils::substringByChars` |
| 统计字符数（中文可算 2 格） | `StringUtils::characterCount` |
| 超长加省略号 | `StringUtils::truncateWithLimit` |
| 退格删最后一个字符 | `StringUtils::popLastCharacter` |
| GBK ⇄ UTF-8 | `StringUtils::convert` |
| 十六进制编解码 | `BytesUtils::hexEncode` / `hexDecode` |
| 大小写转换 / 前后缀判断 | `StringUtils::upper` / `startsWith` / `endsWith` |

### 字节与协议

| 我想… | 用 |
|---|---|
| 大端/小端读多字节整数 | `BytesUtils::readBig16` / `readLittle32` / `read16(order)` |
| 写多字节整数 | `BytesUtils::writeBig16` / `write16(order)` |
| 累加和 / 异或校验 | `CheckUtils::checkSum` / `xorCheck` |
| CRC8 / CRC16 / CRC32 | `CheckUtils::crc8MAXIM` / `crc16MODBUS` / `crc32IEEE` |
| MD5 / SHA256 | `HashUtils::md5Check` / `sha256CheckStr` |

### 时间

| 我想… | 用 |
|---|---|
| 当前秒 / 毫秒时间戳 | `TimeUtils::getTimeSec` / `getTimeMSec` |
| 拆成本地年月日时分秒 | `TimeUtils::getTime` / `localTime` |
| 判断闰年 / 某月天数 | `TimeUtils::isLeapYear` / `getMaxDay` |
| 拼年月日为时间戳 | `TimeUtils::makeTime` |
| 取当天 0 点 / 次日 0 点 | `TimeUtils::getZeroTimeSec` / `getNextZeroTimeSec` |
| 日期整数 `yyyymmdd` | `TimeUtils::getTodayDate` / `getDate` |
| 星期几（数字或文本） | `TimeUtils::getDayOnWeek` |
| 格式化时间 `%Y-%m-%d` | `TimeUtils::getTimeFmtStr` / `getTimeFmtStrAP` |
| 时长文本 `3天 05:21:08` | `TimeUtils::formatDuration` |
| **修改**系统时间 | `ClockUtils::setTime`（需权限） |
| 测量耗时（不受校时影响） | `ClockUtils::monotonicMs` |

### 路径与 URL

| 我想… | 用 |
|---|---|
| 拼接路径 / 取文件名 / 扩展名 | `PathUtils::join` / `fileName` / `extension` |
| 归一化路径（消解 `..`） | `PathUtils::normalize` |
| 判断子路径（防穿越） | `PathUtils::isSubPath` |
| 拆 URL 的 scheme/host/port/path | `UrlUtils::scheme` / `host` / `port` / `path` |
| 百分号编解码 | `UrlUtils::percentEncode` / `percentDecode` |

### 系统与平台

| 我想… | 用 |
|---|---|
| 读 `/sys` 节点数值 | `SysNodeUtils::readInt` / `readBool` |
| 写 `/sys` 节点 | `SysNodeUtils::write` |
| 读 `/proc/meminfo` 等多行文本 | `SysNodeUtils::readTextFile` |
| 执行命令并取输出 | `ProcessUtils::run` / `runCommand`（带超时） |
| 异步执行命令 | `ProcessUtils::runAsync` |
| 退出 / 重启 | `ProcessUtils::exit` / `reboot` |
| 读环境变量 | `EnvUtils::getString` / `getInt` / `getBool` |
| 网卡信息 / IP / 网关 / DNS | `NetworkUtils::getInterfaces` / `getIp` / `getDefaultGateway` / `getDnsServers` |
| 校验 IP / MAC / 端口 | `NetworkUtils::isValidIp` / `isValidMac` / `isValidPort` |
| 设置背光 / 音量 | `DeviceUtils::setBrightness` / `setVolume` |

### 其他

| 我想… | 用 |
|---|---|
| 生成随机数 | `MathUtils::rand` |
| 颜色值打包 | `MathUtils::rgb` / `rgba` |
| 比较版本号 | `VersionUtils::compare` |
| 品牌信息 | `BrandUtils` |

---

## 2. 模块一览

### base（纯计算，可独立单测）

| 头文件 | 职责 |
|---|---|
| `bytes_utils.h` | 线路字节序读写、十六进制编解码 |
| `check_utils.h` | 累加和、异或校验、CRC8/16/32 |
| `string_utils.h` | 字节级字符串操作、数值解析、UTF-8 字符级操作、字符集转码 |
| `time_utils.h` | 读时钟、日历换算、时间/时长格式化 |
| `path_utils.h` | 路径字符串处理（不访问磁盘） |
| `url_utils.h` | URL 解析、百分号编解码 |
| `format_utils.h` | 展示用格式化（hexdump、容量单位） |
| `math_utils.h` | 随机数、字节打包、颜色值 |
| `version_utils.h` | 版本号拆分与比较 |

### platform（系统调用）

| 头文件 | 职责 |
|---|---|
| `file_utils.h` | 路径查询 + 读写/目录操作 + 分区容量 |
| `sysnode_utils.h` | `/sys`、`/proc` 虚拟节点读写 |
| `network_utils.h` | 网卡、IP、网关、DNS、MAC |
| `env_utils.h` | 环境变量读取 |
| `process_utils.h` | 执行命令、退出、重启、sync |
| `clock_utils.h` | 设置系统时钟、硬件时钟同步、单调时钟 |
| `device_utils.h` | 产品适配层（背光、音量） |

### integration（三方依赖）

| 头文件 | 职责 | 依赖 |
|---|---|---|
| `json_utils.h` | JSON 读写与类型转换 | jsoncpp |
| `hash_utils.h` | MD5 / SHA256 | `library_config.h`（MD5 库） |
| `image_utils.h` | 高斯模糊 | 图像算法库 |
| `brand_utils.h` | 品牌信息 | 项目配置头 |
| `cdroid_utils.h` | cdroid 框架桥接 | cdroid |

---

## 3. 分层约束

1. **base 不得依赖 platform / integration**。base 必须能被 `tests/` 独立编译。
2. **platform 可依赖 base**，不得依赖 integration。
3. `FileUtils` 中 `exists` / `isDir` / `isFile` / `getUsage` **不受 `ENABLE_FILE_SYSTEM` 开关影响**（启动路径要用），其余读写接口受控。
4. 需要系统的 iconv 转码属 libc 能力，放在 `string_utils`（base）。

---

## 4. 独立单测

```bash
cmake -S src/utils/tests -B build-utils-tests
cmake --build build-utils-tests -j
./build-utils-tests/kk_utils_tests
```

只覆盖 base + platform + `hash_utils`（其余 integration 需要三方库）。
