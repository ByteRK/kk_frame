#!/bin/sh
# ============================================================
# watchdog.sh — 硬件看门狗现场查看 / 关闭 / 复位辅助
#
# 看门狗由应用自己持有并喂（见 watchdog.conf：WATCHDOG_ENABLE / WATCHDOG_DEV）。
# 内核 watchdog 设备是"单次打开"（watchdog_dev.c: single open）：
#   * 应用运行时别的进程打不开它 => 开启/改参数只能通过应用配置
#   * 关闭 => 应用自己 magic close；或应用已退出后由本脚本补写 'V'
#
# 用法：
#   sh watchdog.sh status    # 是否武装 / 超时 / 剩余时间 / 谁持有
#   sh watchdog.sh off       # 关闭看门狗（应用未运行才有效，运行中会 EBUSY）
#   sh watchdog.sh hwreset   # 用看门狗复位系统（打开后不喂，等硬件超时）
#
# 说明：
#   stop.sh 停应用（kill -9）后会自动调用本脚本 off —— 应用来不及自己 magic
#   close，若不补这一刀，约一个超时周期后系统会被复位。
#
# 配置查找顺序：$WATCHDOG_CONF -> 当前目录 -> 脚本目录 -> board/ -> 应用根目录
# 设备节点取值顺序：$WATCHDOG_DEV -> watchdog.conf 中的 WATCHDOG_DEV -> /dev/watchdog0
# ============================================================

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

# 查找配置文件：$WATCHDOG_CONF -> 当前目录 -> 脚本目录 -> 上级目录(board/) -> 上上级(应用根)
find_conf() {
    [ -n "$WATCHDOG_CONF" ] && [ -f "$WATCHDOG_CONF" ] && { echo "$WATCHDOG_CONF"; return; }
    for c in "./watchdog.conf" "$SCRIPT_DIR/watchdog.conf" \
             "$SCRIPT_DIR/../watchdog.conf" "$SCRIPT_DIR/../../watchdog.conf"; do
        [ -f "$c" ] && { echo "$c"; return; }
    done
}
CONF=$(find_conf)

if [ -z "$WATCHDOG_DEV" ] && [ -n "$CONF" ]; then
    WATCHDOG_DEV=$(sed -n 's/^[[:space:]]*WATCHDOG_DEV[[:space:]]*=[[:space:]]*//p' "$CONF" | tail -n 1)
fi
WD_DEV=${WATCHDOG_DEV:-/dev/watchdog0}
WD_SYS="/sys/class/watchdog/$(basename "$WD_DEV")"

sysfs_get() {
    if [ -r "$WD_SYS/$1" ]; then
        cat "$WD_SYS/$1" 2>/dev/null
    else
        echo "?"
    fi
}

holder() {
    for f in /proc/[0-9]*/fd/*; do
        [ -e "$f" ] || continue
        case "$(readlink "$f" 2>/dev/null)" in
            "$WD_DEV")
                d=$(dirname "$(dirname "$f")")
                basename "$d"
                return
                ;;
        esac
    done
    echo "-"
}

case "$1" in
    status)
        echo "设备     : $WD_DEV"
        if [ ! -e "$WD_DEV" ]; then
            echo "          (节点不存在: 内核未使能 wdt、设备名不同，或该平台无硬件看门狗)"
        fi
        if [ -d "$WD_SYS" ]; then
            echo "identity : $(sysfs_get identity)"
            echo "state    : $(sysfs_get state)      # active=已武装 / inactive=已停"
            echo "timeout  : $(sysfs_get timeout) 秒"
            echo "timeleft : $(sysfs_get timeleft) 秒   # 剩余倒计时，应被持续刷新"
            echo "nowayout : $(sysfs_get nowayout)      # 1=不可关闭(写 V 无效)"
        else
            echo "          (无 $WD_SYS: 驱动未注册 sysfs)"
        fi
        echo "持有者   : $(holder)   # 应用 PID; '-' 表示没人持有(未武装)"
        ;;
    off)
        if [ ! -e "$WD_DEV" ]; then
            echo "off: $WD_DEV 不存在, 无需处理"
            exit 0
        fi
        # 本就没武装时不要碰它（打开 watchdog 设备本身就会把它武装起来）
        st=$(sysfs_get state)
        if [ "$st" = "inactive" ]; then
            echo "off: 当前已是 inactive, 无需处理"
            exit 0
        fi
        # magic close: 写 'V' 后 close，需驱动 WDIOF_MAGICCLOSE 且 nowayout=0
        if printf 'V' > "$WD_DEV" 2>/dev/null; then
            echo "off: 已写 magic close 'V' 到 $WD_DEV"
            if [ -d "$WD_SYS" ]; then
                echo "     state=$(sysfs_get state) (inactive 即已停)"
            fi
        else
            h=$(holder)
            if [ "$h" != "-" ]; then
                echo "off: 失败 —— 设备被 pid=$h 持有（应用正在喂狗），内核单次打开，打不开。"
                echo "     请先停应用，或给应用发 SIGTERM 让它自己 magic close。"
            else
                echo "off: 失败 —— 打开 $WD_DEV 出错（权限? 节点类型不对?）"
            fi
            exit 1
        fi
        ;;
    hwreset|reboot)
        if [ ! -e "$WD_DEV" ]; then
            echo "hwreset: $WD_DEV 不存在"
            exit 1
        fi
        # 打开后不再喂狗，等硬件超时复位（应用在喂时会因单次打开失败）
        exec 9> "$WD_DEV" || {
            echo "hwreset: 打开失败（可能被应用持有，先停应用）"
            exit 1
        }
        echo "hwreset: 已打开 $WD_DEV 且不再喂狗，等待硬件超时复位..."
        while :; do sleep 1; done
        ;;
    ""|-h|--help)
        sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
        ;;
    *)
        echo "未知子命令: $1 (可用: status / off / hwreset)"
        exit 1
        ;;
esac
