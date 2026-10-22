/**
 * @file  watchdog.cc
 * @brief 应用喂狗（硬件看门狗）实现，接口说明见 watchdog.h
**/

#include "watchdog.h"

#include <cdlog.h>
#include <core/systemclock.h>

#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <linux/watchdog.h>
#endif

#include <signal.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

namespace watchdog {

    namespace {

        constexpr int64_t kMinFeedMs      = 100;    ///< 喂狗周期下限
        constexpr int     kTickPollRounds = 16;     ///< checkEvents 放行间隔（轮数）
        constexpr int64_t kNoSignalWarnMs = 120000; ///< 长时间收不到主线程信号的提示间隔

        /// @brief 信号处理用的裸 fd（异步信号安全，不能碰锁/日志）
        volatile sig_atomic_t gSignalFd = -1;

        /// @brief SIGTERM/SIGINT/SIGHUP：先 magic close 再退出，免得"正常停应用"变成"超时后被复位"
        void onTermSignal(int sig) {
            (void)sig;
            const int fd = (int)gSignalFd;
            if (fd >= 0) {
                const char magic = 'V';
                (void)!::write(fd, &magic, 1);
            }
            static const char msg[] = "WATCHDOG: disarmed on signal\n";
            (void)!::write(STDERR_FILENO, msg, sizeof(msg) - 1);
            _exit(0);
        }

        const char* kConfCandidates[] = {
            "./watchdog.conf",
            "./data/watchdog.conf",
            "./config/watchdog.conf",
        };

        int64_t nowMs() {
            return cdroid::SystemClock::uptimeMillis();
        }

        std::string trim(const std::string& s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) return std::string();
            size_t e = s.find_last_not_of(" \t\r\n");
            return s.substr(b, e - b + 1);
        }

        std::string upper(std::string s) {
            for (size_t i = 0; i < s.size(); ++i) {
                s[i] = (char)::toupper((unsigned char)s[i]);
            }
            return s;
        }

        std::string lower(std::string s) {
            for (size_t i = 0; i < s.size(); ++i) {
                s[i] = (char)::tolower((unsigned char)s[i]);
            }
            return s;
        }

        bool parseBool(const std::string& s, bool& out) {
            const std::string v = lower(trim(s));
            if (v.empty()) return false;
            if (v == "1" || v == "on" || v == "true" || v == "yes" ||
                v == "enable" || v == "enabled") {
                out = true;
                return true;
            }
            if (v == "0" || v == "off" || v == "false" || v == "no" ||
                v == "disable" || v == "disabled") {
                out = false;
                return true;
            }
            return false;
        }

        /// @brief dev 是否表示"不使用硬件设备"，此时只做软件监护
        bool devDisabled(const std::string& dev) {
            const std::string v = lower(trim(dev));
            return v.empty() || v == "none" || v == "off" || v == "false" ||
                v == "disabled" || v == "-";
        }

        bool parseInt(const std::string& s, int& out) {
            if (trim(s).empty()) return false;
            char* end = nullptr;
            const long v = ::strtol(s.c_str(), &end, 10);
            if (end == s.c_str()) return false;
            out = (int)v;
            return true;
        }

        bool parsePolicy(const std::string& s, ResetPolicy& out) {
            const std::string v = lower(trim(s));
            if (v == "latch" || v == "lock" || v == "hard" || v == "reboot" || v == "1") {
                out = ResetPolicy::Latch;
                return true;
            }
            if (v == "pause" || v == "resume" || v == "soft" || v == "cancel" || v == "0") {
                out = ResetPolicy::Pause;
                return true;
            }
            return false;
        }

        /// @brief 归一化配置（自动喂狗周期、策略、动作等）
        void normalize(Config& cfg) {
            cfg.dev    = trim(cfg.dev);
            cfg.action = lower(trim(cfg.action));
            if (cfg.action.empty()) cfg.action = "none";

            int64_t feed = cfg.feedMs;
            if (feed <= 0) {
                /* 自动周期取"卡死阈值"和"硬件超时"两个约束里较严的那个。
                 * 周期必须显著小于卡死阈值，否则判定时刻会被周期量化：例如周期和阈值
                 * 都是 10s 时，检查恰好落在 9.9s 就算健康，要等下一个周期才判出卡死。 */
                const int64_t byStall   = (cfg.stallMs > 0) ? (cfg.stallMs / 5) : 0;
                const int64_t byTimeout = ((cfg.timeoutS > 0) ? (int64_t)cfg.timeoutS * 1000 : 30000) / 3;
                feed = ((byStall > 0) && (byStall < byTimeout)) ? byStall : byTimeout;
            }
            if (feed < kMinFeedMs) feed = kMinFeedMs;
            if ((cfg.stallMs > 0) && (feed * 2 > cfg.stallMs)) {
                /* 周期过大时把周期压到阈值的一半，保证判定精度 */
                feed = (cfg.stallMs / 2 > kMinFeedMs) ? (cfg.stallMs / 2) : kMinFeedMs;
            }
            cfg.feedMs = (int)feed;

            /* 上报间隔必须显著小于卡死阈值，否则“多久没报活”会被上报粒度吃掉：
             * 例如间隔 10s、阈值 10s 时，判定只能推迟到 10~20s。这里限制到阈值的 1/4。 */
            if ((cfg.stallMs > 0) && (cfg.tickMs > 0) && (cfg.tickMs * 4 > cfg.stallMs)) {
                cfg.tickMs = (cfg.stallMs / 4 > 0) ? (cfg.stallMs / 4) : 1;
            }
        }

        /// @brief 应用单个配置项（键名可带 WATCHDOG_ 前缀）
        void applyKey(Config& cfg, const std::string& rawKey, const std::string& rawValue,
            bool warnUnknown) {
            std::string key = upper(trim(rawKey));
            const std::string val = trim(rawValue);
            if (key.empty()) return;
            if (key.compare(0, 9, "WATCHDOG_") == 0) key = key.substr(9);

            bool b = false;
            int  n = 0;

            if (key == "ENABLE" || key == "WATCHDOG" || key == "ON") {
                if (parseBool(val, b)) cfg.enable = b;
                else LOGW("WATCHDOG 无法识别的开关值 '%s'（用 1/0/on/off/true/false）", val.c_str());
            } else if (key == "DEV" || key == "DEVICE" || key == "NODE" || key == "DEV_PATH") {
                cfg.dev = val;
            } else if (key == "TIMEOUT" || key == "TIMEOUT_S" || key == "TIMEOUT_SEC" ||
                key == "TIMEOUT_SECONDS") {
                if (parseInt(val, n)) cfg.timeoutS = n;
            } else if (key == "FEED_MS" || key == "FEED" || key == "FEED_INTERVAL_MS") {
                if (parseInt(val, n)) cfg.feedMs = n;
            } else if (key == "TICK_MS" || key == "TICK" || key == "CHECK_MS" ||
                key == "HEARTBEAT_MS" || key == "LIVENESS_MS") {
                if (parseInt(val, n)) cfg.tickMs = n;
            } else if (key == "STALL_MS" || key == "STALL" || key == "STALL_TIMEOUT_MS") {
                if (parseInt(val, n)) cfg.stallMs = n;
            } else if (key == "GRACE_MS" || key == "GRACE" || key == "STARTUP_GRACE_MS") {
                if (parseInt(val, n)) cfg.graceMs = n;
            } else if (key == "LATCH") {
                if (parseBool(val, b)) cfg.policy = b ? ResetPolicy::Latch : ResetPolicy::Pause;
            } else if (key == "POLICY" || key == "MODE" || key == "RESET_POLICY") {
                ResetPolicy p = cfg.policy;
                if (parsePolicy(val, p)) cfg.policy = p;
                else LOGW("WATCHDOG 无法识别的策略 '%s'（latch/pause）", val.c_str());
            } else if (key == "ACTION") {
                cfg.action = val;
            } else if (key == "DISARM_ON_SIGNAL" || key == "SIGNAL_DISARM") {
                if (parseBool(val, b)) cfg.disarmOnSignal = b;
            } else if (key == "CONF" || key == "CONFIG" || key == "CONF_FILE") {
                /* 配置文件路径在扫描阶段处理，这里忽略 */
            } else if (warnUnknown) {
                LOGW("WATCHDOG 未知配置项 '%s'（已忽略）", rawKey.c_str());
            }
        }

    } // namespace

    /* ======================================================================== */
    /* 配置加载                                                                  */
    /* ======================================================================== */

    std::string resolveConfigFile() {
        const char* env = ::getenv("WATCHDOG_CONF");
        if (env && *env) return std::string(env);

        for (size_t i = 0; i < sizeof(kConfCandidates) / sizeof(kConfCandidates[0]); ++i) {
            if (::access(kConfCandidates[i], R_OK) == 0) return std::string(kConfCandidates[i]);
        }
        return std::string();
    }

    bool loadConfigFromFile(const std::string& path, Config& cfg) {
        if (path.empty()) return false;

        FILE* fp = ::fopen(path.c_str(), "r");
        if (!fp) return false;

        char buf[1024];
        int  lineNo = 0;
        while (::fgets(buf, sizeof(buf), fp)) {
            ++lineNo;
            std::string line(buf);

            const size_t hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            line = trim(line);
            if (line.empty()) continue;

            size_t sep = line.find('=');
            if (sep == std::string::npos) sep = line.find_first_of(" \t");
            if (sep == std::string::npos) {
                LOGW("WATCHDOG %s:%d 格式不正确（应为 KEY=VALUE），已忽略", path.c_str(), lineNo);
                continue;
            }
            applyKey(cfg, line.substr(0, sep), line.substr(sep + 1), true);
        }
        ::fclose(fp);

        cfg.confFile = path;
        LOGI("WATCHDOG 已加载配置文件: %s", path.c_str());
        return true;
    }

    void loadConfigFromEnv(Config& cfg) {
        static const char* kEnvKeys[] = {
            "WATCHDOG_ENABLE", "WATCHDOG", "WATCHDOG_DEV",
            "WATCHDOG_TIMEOUT_S", "WATCHDOG_TIMEOUT",
            "WATCHDOG_FEED_MS", "WATCHDOG_TICK_MS", "WATCHDOG_STALL_MS", "WATCHDOG_GRACE_MS",
            "WATCHDOG_LATCH", "WATCHDOG_POLICY", "WATCHDOG_ACTION",
            "WATCHDOG_DISARM_ON_SIGNAL",
        };
        for (size_t i = 0; i < sizeof(kEnvKeys) / sizeof(kEnvKeys[0]); ++i) {
            const char* v = ::getenv(kEnvKeys[i]);
            if (v && *v) applyKey(cfg, kEnvKeys[i], v, false);
        }
    }

    void loadConfigFromArgs(int argc, const char* const argv[], Config& cfg) {
        for (int i = 1; (argv != nullptr) && (i < argc); ++i) {
            const char* a = argv[i];
            if (!a || a[0] != '-' || a[1] != '-') continue;

            const char* body = a + 2;
            const char* eq = ::strchr(body, '=');
            const char* val = eq ? (eq + 1) : nullptr;
            const std::string key(body, eq ? (size_t)(eq - body) : ::strlen(body));

            /* 支持 "--key 值" 写法 */
            auto needVal = [&](void) -> const char* {
                if (!val && (i + 1 < argc) && argv[i + 1]) val = argv[++i];
                return val;
            };

            if (key == "no-watchdog") {
                cfg.enable = false;
            } else if (key == "watchdog") {
                if (val) {
                    bool b = cfg.enable;
                    if (parseBool(val, b)) cfg.enable = b;
                    else LOGW("WATCHDOG 无法识别的 --watchdog 值 '%s'", val);
                } else {
                    cfg.enable = true;   /* 裸 --watchdog 即打开 */
                }
            } else if (key == "watchdog-dev") {
                const char* v = needVal();
                if (v && *v) cfg.dev = v;
            } else if (key == "watchdog-timeout" || key == "watchdog-timeout-s") {
                const char* v = needVal();
                int n = 0;
                if (v && parseInt(v, n)) cfg.timeoutS = n;
            } else if (key == "watchdog-feed-ms") {
                const char* v = needVal();
                int n = 0;
                if (v && parseInt(v, n)) cfg.feedMs = n;
            } else if (key == "watchdog-tick-ms") {
                const char* v = needVal();
                int n = 0;
                if (v && parseInt(v, n)) cfg.tickMs = n;
            } else if (key == "watchdog-stall-ms") {
                const char* v = needVal();
                int n = 0;
                if (v && parseInt(v, n)) cfg.stallMs = n;
            } else if (key == "watchdog-grace-ms") {
                const char* v = needVal();
                int n = 0;
                if (v && parseInt(v, n)) cfg.graceMs = n;
            } else if (key == "watchdog-latch") {
                bool b = true;
                if (val && parseBool(val, b)) { /* 使用给定值 */ }
                cfg.policy = b ? ResetPolicy::Latch : ResetPolicy::Pause;
            } else if (key == "watchdog-policy") {
                const char* v = needVal();
                ResetPolicy p = cfg.policy;
                if (v && parsePolicy(v, p)) cfg.policy = p;
            } else if (key == "watchdog-action") {
                const char* v = needVal();
                if (v && *v) cfg.action = v;
            } else if (key == "watchdog-no-signal-disarm") {
                cfg.disarmOnSignal = false;
            } else if (key == "watchdog-conf") {
                /* 已在 start() 前置扫描中处理 */
                needVal();
            }
            /* 其它 --xxx 忽略，避免与框架参数冲突 */
        }
    }

    /* ======================================================================== */
    /* Watchdog                                                                  */
    /* ======================================================================== */

    Watchdog& Watchdog::instance() {
        static Watchdog s;
        return s;
    }

    Watchdog::Watchdog() { }

    Watchdog::~Watchdog() {
        /* 进程正常退出时默认不做 magic close：若守护脚本没能把应用拉起来，
         * 让狗咬一口重启系统，比"静默停机"更好。需要干净停机请显式 stop(true)。
         * 此时不再访问主 Looper —— 静态析构阶段它可能已被销毁。 */
        shutdown(false, false);
    }

    void Watchdog::tick() {
        /* 手动上报入口（默认不会被调用）。
         * mLastTickAtMs 同时是 handleEvents 的节流基准。 */
        mLastTickAtMs = nowMs();
        mTickSeq.fetch_add(1, std::memory_order_relaxed);
    }

    int Watchdog::checkEvents() {
        /* 主线程热路径（约每 1ms 一轮）：纯计数门闸，不读时钟、不做原子操作。
         * 每 kTickPollRounds 轮放行一次，真正的时间判断放到 handleEvents 里，
         * 这样无论轮速多快，这里都只有一次自增 + 比较（实测约 2ns）。 */
        if (mTickIntervalMs <= 0) return 1;      /* 0 = 每轮都上报 */
        if (++mPollRounds < kTickPollRounds) return 0;
        mPollRounds = 0;
        return 1;
    }

    int Watchdog::handleEvents() {
        /* 仅在计数门闸放行时进来（默认每 16 轮，空闲约 16~100+ms 一次）：
         * 这里才读时钟做真正的间隔判断，因此上报频率不会被轮速放大。 */
        if (mTickIntervalMs > 0) {
            const int64_t now = nowMs();
            if ((now - mLastTickAtMs) < mTickIntervalMs) return 0;
            mLastTickAtMs = now;
        }
        mTickSeq.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }

    bool Watchdog::registerTickHandler() {
        if (mRegistered.load(std::memory_order_relaxed)) return true;

        cdroid::Looper* looper = cdroid::Looper::getMainLooper();
        if (!looper) {
            LOGW("WATCHDOG 主 Looper 尚未就绪，无法自动检测主线程存活；"
                "请在主循环中调用 watchdog::Watchdog::instance().tick()");
            return false;
        }
        looper->addEventHandler(this);
        mRegistered.store(true, std::memory_order_relaxed);
        return true;
    }

    void Watchdog::unregisterTickHandler() {
        if (!mRegistered.exchange(false, std::memory_order_relaxed)) return;
        cdroid::Looper* looper = cdroid::Looper::getMainLooper();
        if (looper) looper->removeEventHandler(this);
    }

    int Watchdog::openDevice(int& actualTimeoutS) {
        actualTimeoutS = mCfg.timeoutS;
        if (devDisabled(mCfg.dev)) return -ENODEV;

        const int fd = ::open(mCfg.dev.c_str(), O_WRONLY | O_CLOEXEC);
        if (fd < 0) return -errno;

#ifdef WDIOC_SETTIMEOUT
        if (mCfg.timeoutS > 0) {
            int req = mCfg.timeoutS;
            if (::ioctl(fd, WDIOC_SETTIMEOUT, &req) == 0) {
                actualTimeoutS = req;
#ifdef WDIOC_GETTIMEOUT
                int readback = req;
                if (::ioctl(fd, WDIOC_GETTIMEOUT, &readback) == 0) actualTimeoutS = readback;
#endif
            } else {
                LOGW("WATCHDOG 设置超时 %ds 失败: %s（沿用驱动默认值）",
                    mCfg.timeoutS, ::strerror(errno));
            }
        }
#endif

        mFd.store(fd, std::memory_order_relaxed);
        gSignalFd = fd;
        return 0;
    }

    void Watchdog::closeDevice(bool disarm) {
        const int fd = mFd.exchange(-1, std::memory_order_relaxed);
        gSignalFd = -1;
        if (fd < 0) return;

        if (disarm) {
            const char magic = 'V';
            if (::write(fd, &magic, 1) == 1) {
                LOGI("WATCHDOG 已 magic close（nowayout=0 且驱动支持时才真正关闭）");
            } else {
                LOGW("WATCHDOG magic close 写失败: %s", ::strerror(errno));
            }
        }
        ::close(fd);
    }

    bool Watchdog::feed() {
        const int fd = mFd.load(std::memory_order_relaxed);
        if (fd < 0) return false;

        const char keepalive = '1';   /* 内容无关紧要，写一次即喂狗 */
        for (int i = 0; i < 2; ++i) {
            const ssize_t n = ::write(fd, &keepalive, 1);
            if (n == 1) {
                mFeeds.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            if ((n < 0) && (errno == EINTR)) continue;
            LOGW("WATCHDOG 喂狗失败: %s", ::strerror(errno));
            return false;
        }
        return false;
    }

    void Watchdog::triggerAction() {
        const std::string& action = mCfg.action;
        if (action.empty() || action == "none" || action == "off" || action == "log") {
            return;
        }

        if (action == "reboot" || action == "reset") {
#if defined(__linux__)
            LOGE("WATCHDOG 执行动作: reboot()");
            ::sync();
            if (::reboot(RB_AUTOBOOT) != 0) {
                LOGE("WATCHDOG reboot() 失败: %s（需要 root/CAP_SYS_BOOT）", ::strerror(errno));
            }
#else
            LOGE("WATCHDOG 当前平台不支持自动复位，action=reboot 已忽略"
                "（可改用 action=exec:<命令>）");
#endif
            return;
        }

        if (action.compare(0, 5, "exec:") == 0) {
            const std::string cmd = trim(action.substr(5));
            if (cmd.empty()) return;
            LOGE("WATCHDOG 执行动作: %s", cmd.c_str());
            const int rc = ::system(cmd.c_str());
            LOGW("WATCHDOG 动作执行完毕，返回码 %d", rc);
            return;
        }

        LOGW("WATCHDOG 无法识别的动作 '%s'（支持 none/reboot/exec:<命令>）", action.c_str());
    }

    void Watchdog::run() {
        const int64_t feedMs  = mCfg.feedMs;
        const int64_t stallMs = (mCfg.stallMs > 0) ? (int64_t)mCfg.stallMs : 0;
        const int64_t graceMs = (mCfg.graceMs > 0) ? (int64_t)mCfg.graceMs : 0;
        const bool    latch   = (mCfg.policy == ResetPolicy::Latch);

        const int64_t startedAt = nowMs();
        uint32_t lastSeq   = 0;       ///< 上次看到的心跳序号
        bool    haveSignal = false;   ///< 是否已经收到过主线程报活
        int64_t lastChange = 0;       ///< 心跳序号上次变化的时刻（喂狗线程计时）
        bool    tripped    = false;   ///< 已判定卡死
        bool    paused     = false;   ///< 当前处于停喂状态
        int64_t pausedAt   = 0;
        bool    actionDone = false;
        bool    warnedNoSignal = false;

        std::unique_lock<std::mutex> lk(mMtx);
        while (mRun.load(std::memory_order_relaxed)) {
            mCv.wait_for(lk, std::chrono::milliseconds((long)feedMs));
            if (!mRun.load(std::memory_order_relaxed)) break;

            const int64_t now = nowMs();

            bool healthy = true;
            if (stallMs > 0) {
                const uint32_t seq = mTickSeq.load(std::memory_order_relaxed);
                if (seq == 0) {
                    /* 主循环还没跑起来（初始化阶段），不判卡死 */
                    if (!warnedNoSignal && ((now - startedAt) > kNoSignalWarnMs)) {
                        warnedNoSignal = true;
                        LOGW("WATCHDOG 未收到主线程存活信号，卡死保护不生效"
                            "（检查主 Looper 是否在运行）");
                    }
                } else {
                    if (seq != lastSeq) {
                        lastSeq    = seq;
                        lastChange = now;
                    }
                    haveSignal = true;
                    healthy    = ((now - lastChange) < stallMs);
                }
            }

            /* 启动宽限期：应用初始化可能长时间占用主线程，这段时间只喂狗不判定 */
            if (graceMs > 0 && ((now - startedAt) < graceMs)) healthy = true;

            /* 闩锁模式：一旦判定卡死，之后无论主线程是否恢复都不再喂狗 */
            if (latch && tripped) healthy = false;

            if (healthy) {
                if (paused) {
                    LOGW("WATCHDOG 主线程已恢复（共停喂 %lldms），继续喂狗，本次复位取消",
                        (long long)(now - pausedAt));
                    paused = false;
                }
                feed();
                continue;
            }

            const int64_t stalled = haveSignal ? (now - lastChange) : 0;
            mSkips.fetch_add(1, std::memory_order_relaxed);

            if (!paused) {
                paused   = true;
                pausedAt = now;
                tripped  = true;
                if (latch) {
                    LOGE("WATCHDOG 主线程卡死 %lldms（阈值 %lldms），已闩锁停喂；"
                        " 硬件将在约 %dms 后复位（超时 %ds）",
                        (long long)stalled, (long long)stallMs,
                        (int)(mCfg.timeoutS * 1000), mCfg.timeoutS);
                } else {
                    LOGE("WATCHDOG 主线程卡死 %lldms（阈值 %lldms），暂停喂狗；"
                        " 硬件将在约 %dms 后复位，主线程若恢复则取消",
                        (long long)stalled, (long long)stallMs,
                        (int)(mCfg.timeoutS * 1000));
                }
            }

            if (!actionDone) {
                actionDone = true;
                triggerAction();
            }
        }
        lk.unlock();
    }

    int Watchdog::start(int argc, const char* const argv[]) {
        Config cfg;

        /* 命令行里的 --watchdog-conf 优先决定读取哪个配置文件 */
        std::string conf;
        for (int i = 1; (argv != nullptr) && (i < argc); ++i) {
            const char* a = argv[i];
            if (!a) continue;
            if (::strncmp(a, "--watchdog-conf=", 16) == 0) {
                conf = a + 16;
            } else if (::strcmp(a, "--watchdog-conf") == 0 && (i + 1 < argc) && argv[i + 1]) {
                conf = argv[i + 1];
            }
        }
        if (conf.empty()) conf = resolveConfigFile();
        loadConfigFromFile(conf, cfg);

        loadConfigFromEnv(cfg);
        loadConfigFromArgs(argc, argv, cfg);

        return start(cfg);
    }

    int Watchdog::start(const Config& cfg) {
        if (mArmed.load(std::memory_order_relaxed)) return 0;

        mCfg = cfg;
        normalize(mCfg);
        mEnable.store(mCfg.enable, std::memory_order_relaxed);

        if (!mCfg.enable) {
            LOGI("WATCHDOG 未启用（配置文件: %s；可用 WATCHDOG_ENABLE=1 / --watchdog 打开）",
                mCfg.confFile.empty() ? "未找到" : mCfg.confFile.c_str());
            return 1;
        }

        /* 打开硬件看门狗；失败不视为致命错误 —— 其它平台可能根本没有该设备节点，
         * 此时降级为"软件监护 + 可配置动作"，保证同一份代码到处都能跑。 */
        int actualTimeout = mCfg.timeoutS;
        const int err = openDevice(actualTimeout);
        mHardware.store(err == 0, std::memory_order_relaxed);

        if (err == 0) {
            /* 驱动会把请求的超时向上量化到硬件支持的某一档，以读回来的实际值为准 */
            mCfg.timeoutS = actualTimeout;
            /* 打开设备的瞬间硬件计时已经开始，必须马上喂第一口 */
            feed();
        } else if (devDisabled(mCfg.dev)) {
            LOGI("WATCHDOG 未使用硬件设备（dev=%s），仅做软件监护", mCfg.dev.c_str());
        } else {
            LOGW("WATCHDOG 打开 %s 失败: %s —— 降级为软件监护（动作=%s）",
                mCfg.dev.c_str(), ::strerror(-err), mCfg.action.c_str());
        }

        mTickIntervalMs = mCfg.tickMs;
        mLastTickAtMs   = 0;          /* 0 = 首轮放行即上报一次 */
        mPollRounds     = 0;
        mTickSeq.store(0, std::memory_order_relaxed);
        registerTickHandler();

        mRun.store(true, std::memory_order_relaxed);
        mArmed.store(true, std::memory_order_relaxed);
        mThread = std::thread(&Watchdog::run, this);

        if (mCfg.disarmOnSignal) installSignalHandlers();

        LOGI("WATCHDOG 已启动: dev=%s 硬件=%s 超时=%ds 喂狗周期=%dms 上报间隔=%dms",
            mCfg.dev.c_str(), mHardware.load() ? "是" : "否", mCfg.timeoutS, mCfg.feedMs,
            mCfg.tickMs);
        if (mCfg.stallMs > 0) {
            const int policy = (int)mCfg.policy;
            LOGI("WATCHDOG 卡死保护: 主线程连续不可用 >=%dms 即停喂（策略=%s，动作=%s，宽限 %dms）",
                mCfg.stallMs, policy == (int)ResetPolicy::Latch ? "latch" : "pause",
                mCfg.action.c_str(), mCfg.graceMs);
            if (mHardware.load()) {
                LOGI("WATCHDOG 复位延迟约 %dms（阈值 + 硬件超时 %ds），从现在开始卡死算起",
                    mCfg.stallMs + mCfg.timeoutS * 1000, mCfg.timeoutS);
            }
        } else {
            LOGW("WATCHDOG 未开启卡死判定（WATCHDOG_STALL_MS=0）：只能防喂狗线程/进程整体死掉");
        }
        return 0;
    }

    void Watchdog::installSignalHandlers() {
        struct sigaction sa;
        ::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = onTermSignal;
        ::sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        ::sigaction(SIGTERM, &sa, nullptr);
        ::sigaction(SIGINT,  &sa, nullptr);
        ::sigaction(SIGHUP,  &sa, nullptr);
    }

    void Watchdog::stop(bool disarm) {
        shutdown(disarm, true);
    }

    void Watchdog::shutdown(bool disarm, bool unregister) {
        {
            std::lock_guard<std::mutex> lk(mMtx);
            mRun.store(false, std::memory_order_relaxed);
        }
        mCv.notify_all();

        if (mThread.joinable()) {
            if (mThread.get_id() == std::this_thread::get_id()) mThread.detach();
            else mThread.join();
        }

        mArmed.store(false, std::memory_order_relaxed);
        mHardware.store(false, std::memory_order_relaxed);
        if (unregister) unregisterTickHandler();
        closeDevice(disarm);
    }

} // namespace watchdog
