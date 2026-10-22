/**
 * @file  watchdog.h
 * @brief 应用喂狗（硬件看门狗）与主线程卡死监护
 *
 * ---------------------------------------------------------------------------
 * 为什么由应用来喂狗
 * ---------------------------------------------------------------------------
 * 内核自己喂狗只能证明"内核还在跑"，界面卡死 / 应用死循环时内核照样在跑，
 * 狗永远不会咬。把喂狗放在应用里，喂狗节奏由应用掌握，才能把"应用卡死"
 * 变成一次系统复位。
 *
 * 本模块做三件事：
 *   1) 打开设备节点并周期性喂狗（独立线程，不受主线程卡死影响）
 *   2) 通过 cdroid 主 Looper 的 EventHandler 感知"主线程还活着"
 *      （checkEvents() 由主线程约每 1ms 调用一次，只做一次计数门闸，实测 ~2ns；
 *        每 16 轮放行一次，handleEvents() 里再做时间判断，达到
 *        WATCHDOG_TICK_MS（默认 100ms）才真正刷新存活信号）
 *   3) 主线程连续不可用超过阈值后：停止喂狗（等待硬件复位），
 *      并可选执行一个动作（none / reboot / exec:<命令>）
 *
 * 应用层只需要启动一次：
 * @code
 *   watchdog::Watchdog::instance().start(argc, argv);
 * @endcode
 * 开关与参数由"配置文件 / 环境变量 / 命令行"决定，换平台只改配置，不改代码。
 *
 * ---------------------------------------------------------------------------
 * 参数优先级（高 -> 低）
 * ---------------------------------------------------------------------------
 *   命令行 > 环境变量 > 配置文件 > 内置默认值
 *
 * 配置文件查找顺序（第一个存在的生效）：
 *   $WATCHDOG_CONF -> ./watchdog.conf -> ./data/watchdog.conf -> ./config/watchdog.conf
 * 文件格式为每行 "KEY=VALUE"（也接受 "KEY VALUE"），'#' 之后为注释。
 * 键名与环境变量同名，且可省略 WATCHDOG_ 前缀，例如：
 * @code
 *   WATCHDOG_ENABLE=1
 *   WATCHDOG_DEV=/dev/watchdog0
 *   WATCHDOG_TIMEOUT_S=30
 *   WATCHDOG_FEED_MS=0
 *   WATCHDOG_STALL_MS=10000
 *   WATCHDOG_GRACE_MS=30000
 *   WATCHDOG_POLICY=latch        # latch=判定即复位 / pause=恢复即取消
 *   WATCHDOG_ACTION=none         # none / reboot / exec:<命令>
 * @endcode
 *
 * 环境变量：
 *   WATCHDOG_ENABLE=1            总开关（也接受 WATCHDOG=1/on/true/yes）  默认 0
 *   WATCHDOG_CONF=<path>         指定配置文件
 *   WATCHDOG_DEV=/dev/watchdog0  设备节点；none/off/disabled 表示纯软件监护
 *   WATCHDOG_TIMEOUT_S=30        硬件超时（秒），<=0 表示沿用驱动默认值
 *   WATCHDOG_FEED_MS=0           喂狗周期（ms），<=0 自动
 *   WATCHDOG_TICK_MS=100         主线程存活上报间隔（ms），<=0 每轮都上报
 *   WATCHDOG_STALL_MS=10000      主线程连续不可用阈值（ms），<=0 关闭卡死判定
 *   WATCHDOG_GRACE_MS=30000      启动宽限期（ms），<=0 关闭
 *   WATCHDOG_POLICY=latch        latch / pause
 *   WATCHDOG_ACTION=none         none / reboot / exec:<命令>
 *   WATCHDOG_DISARM_ON_SIGNAL=1  收到 SIGTERM/SIGINT/SIGHUP 时 magic close
 *
 * 命令行（--key=<值> 或 --key <值>）：
 *   --watchdog[=1|0|on|off]    --no-watchdog
 *   --watchdog-dev=<路径>      --watchdog-timeout=<秒>
 *   --watchdog-feed-ms=<ms>    --watchdog-tick-ms=<ms>    --watchdog-stall-ms=<ms>
 *   --watchdog-grace-ms=<ms>   --watchdog-policy=latch|pause（--watchdog-latch 等价 latch）
 *   --watchdog-action=none|reboot|exec:<命令>
 *   --watchdog-conf=<路径>     --watchdog-no-signal-disarm
 *
 * ---------------------------------------------------------------------------
 * 复位延迟（最容易误解的一点）
 * ---------------------------------------------------------------------------
 * 硬件计时器从"最后一次喂狗"开始倒数，不是从"我们停止喂狗"开始：
 *   实际复位延迟 ≈ 卡死阈值 + 硬件超时
 * 例如阈值 10s、驱动实际生效超时 44s，则约 54 秒后复位。
 * 注意驱动会把请求的超时向上量化到硬件支持的某一档，本模块启动日志里打印的是
 * 读回来的实际生效值，估算延迟请用那个值。
 *
 * ---------------------------------------------------------------------------
 * 两种判定策略
 * ---------------------------------------------------------------------------
 *   Latch（默认）：一旦判定卡死就永久停喂，主线程即使随后恢复也不取消，
 *                 保证"已经卡了这么久，重启一次"。适合无人值守设备。
 *   Pause       ：判定后暂停喂狗，主线程恢复则继续喂，本次复位取消。
 *                 只对"持续不恢复的卡死"生效，卡了 N 秒又自己好了不会重启。
 *
 * ---------------------------------------------------------------------------
 * 平台兼容
 * ---------------------------------------------------------------------------
 *   Linux/Android：直接操作 /dev/watchdog*，支持 WDIOC_SETTIMEOUT 与 magic close。
 *   其它平台     ：无法打开设备时不报错，自动降级为"软件监护 + 可配置动作"
 *                 （action 默认 none，仅打日志）。也可显式写 WATCHDOG_DEV=none。
 *   不同芯片     ：只需改 WATCHDOG_DEV / WATCHDOG_TIMEOUT_S，无需改代码。
**/

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include <core/looper.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace watchdog {

    /// @brief 判定卡死后的喂狗策略
    enum class ResetPolicy {
        Latch = 0,  ///< 闩锁：判定后永久停喂，主线程恢复也不取消（保证复位）
        Pause = 1,  ///< 暂停：判定后暂停喂狗，主线程恢复则继续喂（取消本次复位）
    };

    /// @brief 喂狗配置
    struct Config {
        bool        enable = false;               ///< 总开关
        std::string dev = "/dev/watchdog0";       ///< 设备节点；none/off/disabled=纯软件监护
        int         timeoutS = 30;                ///< 硬件超时（秒），<=0 沿用驱动默认值
        int         feedMs = 0;                   ///< 喂狗周期（ms），<=0 自动
        int         tickMs = 100;                 ///< 主线程存活上报间隔（ms），<=0 每轮上报
        int         stallMs = 10000;              ///< 主线程连续不可用阈值（ms），<=0 关闭判定
        int         graceMs = 30000;              ///< 启动宽限期（ms），<=0 关闭
        ResetPolicy policy = ResetPolicy::Latch;  ///< 卡死后的策略
        std::string action = "none";              ///< none / reboot / exec:<命令>
        bool        disarmOnSignal = true;        ///< 信号退出时 magic close 关掉看门狗
        std::string confFile;                     ///< 本次实际读取的配置文件（只读）
    };

    /// @brief 应用喂狗（硬件看门狗）与主线程卡死监护
    /// @note 单例；start() 之后由内部线程负责喂狗，主线程只需正常跑事件循环。
    class Watchdog : public cdroid::EventHandler {
    public:
        /// @brief 获取单例
        static Watchdog& instance();

        /// @brief 按"配置文件 -> 环境变量 -> 命令行"的顺序装配配置并启动
        /// @param argc 参数个数（可为 0）
        /// @param argv 参数列表（可为 nullptr，仅识别 --watchdog* 参数）
        /// @return 0-已启动（含设备打开失败后降级） 1-按配置未启用 <0-启动失败
        int start(int argc = 0, const char* const argv[] = nullptr);

        /// @brief 使用给定配置启动
        /// @return 0-已启动（含设备打开失败后降级） 1-按配置未启用 <0-启动失败
        int start(const Config& cfg);

        /// @brief 停止喂狗线程
        /// @param disarm true-先写 'V' magic close 再关闭设备（需要 nowayout=0）
        void stop(bool disarm = true);

        /// @brief 主动上报一次"主线程活着"
        /// @note 正常无需调用：内部 EventHandler 已自动报活；
        ///       仅当主 Looper 尚不可用/自定义循环时使用。
        /// @note 必须在主线程调用（checkEvents/handleEvents 同线程）。
        void tick();

        bool armed()  const { return mArmed.load(std::memory_order_relaxed); }
        bool enabled() const { return mEnable.load(std::memory_order_relaxed); }
        bool hardware() const { return mHardware.load(std::memory_order_relaxed); }
        int  actualTimeoutS() const { return mCfg.timeoutS; }

        const Config& config() const { return mCfg; }
        int64_t feeds() const { return mFeeds.load(std::memory_order_relaxed); }
        int64_t skips() const { return mSkips.load(std::memory_order_relaxed); }

    protected:
        Watchdog();
        ~Watchdog() override;

        int checkEvents() override;
        int handleEvents() override;

    private:
        Watchdog(const Watchdog&) = delete;
        Watchdog& operator=(const Watchdog&) = delete;

        void run();                                   ///< 喂狗线程主体
        bool feed();                                  ///< 写一次 KEEPALIVE
        void triggerAction();                         ///< 执行配置的动作（只执行一次）
        int  openDevice(int& actualTimeoutS);         ///< 打开设备，返回 0 成功
        void closeDevice(bool disarm);                ///< 关闭设备（可选 magic close）
        void shutdown(bool disarm, bool unregister);  ///< 停止线程/关闭设备
        bool registerTickHandler();                   ///< 注册主线程存活信号
        void unregisterTickHandler();
        void installSignalHandlers();

    private:
        Config                  mCfg;
        std::atomic<bool>       mRun{ false };
        std::atomic<bool>       mArmed{ false };
        std::atomic<bool>       mEnable{ false };
        std::atomic<bool>       mHardware{ false };
        std::atomic<bool>       mRegistered{ false };
        std::atomic<int>        mFd{ -1 };
        std::atomic<int64_t>    mFeeds{ 0 };
        std::atomic<int64_t>    mSkips{ 0 };
        std::atomic<uint32_t>   mTickSeq{ 0 };        ///< 主线程心跳序号（每次 tick +1，回绕无害）

        /* 以下字段只在主线程访问（checkEvents/handleEvents/tick），无需原子 */
        int64_t                 mTickIntervalMs{ 100 };  ///< 存活上报间隔
        int64_t                 mLastTickAtMs{ 0 };      ///< 上次上报时刻
        int                     mPollRounds{ 0 };        ///< checkEvents 已走过的轮数（门闸计数）

        std::thread             mThread;
        std::mutex              mMtx;
        std::condition_variable mCv;
    };

    /// @brief 解析配置文件（KEY=VALUE 或 KEY VALUE，'#' 起注释）
    /// @return 成功读取返回 true，并在 cfg.confFile 记录实际路径
    bool loadConfigFromFile(const std::string& path, Config& cfg);

    /// @brief 查找配置文件（$WATCHDOG_CONF -> ./watchdog.conf -> ...），未找到返回空
    std::string resolveConfigFile();

    /// @brief 用环境变量覆盖 cfg（只覆盖设置过的项）
    void loadConfigFromEnv(Config& cfg);

    /// @brief 用命令行覆盖 cfg（只识别 --watchdog* 参数）
    void loadConfigFromArgs(int argc, const char* const argv[], Config& cfg);

} // namespace watchdog

#endif // __WATCHDOG_H__
