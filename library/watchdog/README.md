# watchdog —— 应用喂狗（硬件看门狗）

应用自己持有 `/dev/watchdog*` 并由独立线程周期性喂狗；主线程通过 cdroid 主 Looper
的 `EventHandler` 上报存活，连续超过阈值不可用即停止喂狗，由硬件把系统复位。

> 为什么不是内核喂狗：内核喂狗只能证明"内核还在跑"。界面卡死 / 应用死循环时内核
> 照样在跑，狗永远不会咬。把喂狗放在应用里，才能把"应用卡死"变成一次复位。

## 应用接入

只需一行（`main.cc` 已接入）：

```cpp
watchdog::Watchdog::instance().start(argc, argv);
```

- 内部自行注册事件处理，用来感知主线程存活，应用不需要写心跳代码。
- 是否启用、设备节点、超时等全部由**配置文件 / 环境变量 / 命令行**决定。

## 配置

优先级：**命令行 > 环境变量 > 配置文件 > 内置默认值**

配置文件查找顺序（第一个存在的生效）：

```
$WATCHDOG_CONF  ->  ./watchdog.conf  ->  ./data/watchdog.conf  ->  ./config/watchdog.conf
```

格式为 `KEY=VALUE`（也接受 `KEY VALUE`），`#` 之后为注释；键名与环境变量同名，
可省略 `WATCHDOG_` 前缀。仓库内附带示例：`board/watchdog.conf`。

| 键 | 默认 | 说明 |
|---|---|---|
| `WATCHDOG_ENABLE` | `0` | 总开关 |
| `WATCHDOG_DEV` | `/dev/watchdog0` | 设备节点；`none/off/disabled` = 纯软件监护 |
| `WATCHDOG_TIMEOUT_S` | `30` | 硬件超时（秒），<=0 沿用驱动默认值 |
| `WATCHDOG_FEED_MS` | `0` | 喂狗周期，0 = 自动 `min(阈值/5, 超时/3)`，下限 100ms |
| `WATCHDOG_TICK_MS` | `100` | 主线程存活上报间隔，0 = 每轮都上报；会被限制到不超过阈值 1/4 |
| `WATCHDOG_STALL_MS` | `10000` | 主线程连续不可用阈值，<=0 关闭卡死判定 |
| `WATCHDOG_GRACE_MS` | `30000` | 启动宽限期，期间只喂狗不判定 |
| `WATCHDOG_POLICY` | `latch` | `latch`=判定即停喂直到复位；`pause`=恢复即取消 |
| `WATCHDOG_ACTION` | `none` | `none` / `reboot` / `exec:<命令>` |
| `WATCHDOG_DISARM_ON_SIGNAL` | `1` | 收到 SIGTERM/SIGINT/SIGHUP 时 magic close |

命令行示例：

```sh
./kk_frame --watchdog                       # 打开，其余用配置/默认值
./kk_frame --watchdog-dev=/dev/watchdog1 --watchdog-timeout=11
./kk_frame --watchdog-stall-ms=5000 --watchdog-policy=pause
./kk_frame --no-watchdog                    # 强制关闭
```

## 存活上报的实现

主 Looper 每轮（约 1ms）都会调用 `checkEvents()`，所以这个函数必须足够便宜。
这里用**纯计数门闸**：不读时钟、不做原子操作，实测约 2ns。

```cpp
int checkEvents() {                          // 热路径：只自增+比较
    if (mTickIntervalMs <= 0) return 1;      // 0 = 每轮都上报
    if (++mPollRounds < kTickPollRounds) return 0;   // 16 轮放行一次
    mPollRounds = 0;
    return 1;
}

int handleEvents() {                         // 冷路径：放行时才进来
    if (mTickIntervalMs > 0) {
        const int64_t now = nowMs();
        if ((now - mLastTickAtMs) < mTickIntervalMs) return 0;  // 时间未到，跳过
        mLastTickAtMs = now;
    }
    mTickSeq.fetch_add(1, std::memory_order_relaxed);  // 喂狗线程据此判活
    return 0;
}
```

两层过滤的原因：`checkEvents()` 的调用频率会随事件量上涨（有动画/触摸时远高于
1kHz），只靠轮数会把上报频率放大；只靠时钟又会让热路径付出 ~25ns 的
`clock_gettime`。分开后热路径 ~2ns，上报频率由 `WATCHDOG_TICK_MS` 精确控制。

## 复位延迟

硬件计时从**最后一次喂狗**开始倒数，所以：

```
实际复位延迟 ≈ WATCHDOG_STALL_MS + 硬件超时
```

例如阈值 10s、驱动实际生效超时 44s，则卡死后约 54 秒复位。驱动会把请求的超时
向上量化到最接近的支持档位，**启动日志里打印的是实际生效值**，估算请用它。

## 两种策略

| 策略 | 行为 | 适用 |
|---|---|---|
| `latch`（默认） | 判定卡死即永久停喂，主线程恢复也不取消 | 无人值守设备，宁可重启 |
| `pause` | 暂停喂狗，主线程恢复则继续喂，本次复位取消 | 卡顿可能自愈的场景 |

## 平台兼容

| 平台 | 行为 |
|---|---|
| Linux / Android | 直接操作 `/dev/watchdog*`，支持 `WDIOC_SETTIMEOUT` 与 magic close |
| 其它平台 / 无设备节点 | 不报错，自动降级为"软件监护 + 可配置动作"（默认 `action=none` 只打日志） |
| 不同芯片 | 只改 `WATCHDOG_DEV` / `WATCHDOG_TIMEOUT_S`，无需改代码 |

需要完全关掉硬件操作时把 `WATCHDOG_DEV` 设为 `none`；需要在无硬件狗平台仍能恢复，
把 `WATCHDOG_ACTION` 设为 `reboot` 或 `exec:<命令>`。

## 板级脚本

`board/script/watchdog.sh` 提供现场常用操作：

```sh
sh board/script/watchdog.sh status    # 是否武装 / 超时 / 剩余时间 / 谁持有
sh board/script/watchdog.sh off       # 关闭（应用未运行时有效）
sh board/script/watchdog.sh hwreset   # 用看门狗复位系统
```

配置查找顺序：`$WATCHDOG_CONF` → 当前目录 → 脚本目录 → `board/` → 应用根目录，
所以脚本放在 `board/script/` 下也能自动找到 `board/watchdog.conf`。

`board/script/stop.sh` 在 `kill -9` 后会自动调用 `watchdog.sh off`，避免"停应用
之后系统被复位"。

## 注意事项

- 设备是单次打开，应用运行期间其它进程无法打开它。
- 进程**正常退出**默认不做 magic close：若守护脚本没把应用拉起来，让看门狗咬一口
  重启系统比静默停机更安全。需要干净停机请显式调用 `stop(true)`。
- `SIGKILL` 捕获不到，只能靠 `stop.sh` 事后补写 `'V'`。
