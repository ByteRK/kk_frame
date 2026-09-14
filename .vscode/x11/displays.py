#!/usr/bin/env python3
"""探测当前真实可用的 X11 DISPLAY。

背景：MobaXterm 的 X11 转发在本机表现为 TCP 回环端口 127.0.0.1:6000+N
（DISPLAY=localhost:N.0 对应端口 6000+N），display 号由 sshd 按会话动态分配，
会随会话增减而变化，因此必须实测而不能写死。

留意两件事：

1. 端口"在监听"并不等于可用——其它会话/用户占用的端口会返回
   "MoTTY X11 proxy: Authorisation not recognised"，所以这里统一用与程序运行期
   相同的 XOpenDisplay 实测。
2. **连接建立阶段没有超时**。转发通道已经死掉的僵尸端口会 accept 连接却永不回
   握手指包，此时 XOpenDisplay 会一直阻塞在 poll() 上；进程内没有任何办法打断它，
   SIGALRM 也不行（Xlib 对 EINTR 是内部重试，信号到不了 Python 层）。
   因此每次实测都放进子进程，超时直接 kill，保证整体耗时可控。

用法:
    python3 displays.py            # 每行输出一个可用 DISPLAY，新会话在前
    python3 displays.py --write    # 把最优的一个写入同目录的 display.env

约定: 无论成功与否都以 0 退出，避免探测失败中断构建或调试。
"""

from __future__ import annotations

import ctypes
import os
import re
import socket
import subprocess
import sys
from concurrent import futures

HERE = os.path.dirname(os.path.abspath(__file__))
ENV_FILE = os.path.join(HERE, "display.env")
ENV_NAME = "DISPLAY"
PORT_BASE = 6000
PORT_COUNT = 64

PROBE_FLAG = "--probe"  # 内部模式：单次实测，退出码即结果（0 可用 / 1 不可用）
PROBE_TIMEOUT = 3.0  # 单个 display 的实测上限（秒）
PROBE_WORKERS = 8  # 并行实测的并发数（整体耗时 ≈ PROBE_TIMEOUT）


def _load_xlib():
    """加载 libX11，没有则返回 None。"""
    try:
        lib = ctypes.CDLL("libX11.so.6")
    except OSError:
        return None
    lib.XOpenDisplay.restype = ctypes.c_void_p
    lib.XOpenDisplay.argtypes = [ctypes.c_char_p]
    lib.XCloseDisplay.argtypes = [ctypes.c_void_p]
    return lib


def _usable(lib, name: str) -> bool:
    """用与程序运行期相同的 XOpenDisplay 实测，能连上才算可用。"""
    dpy = lib.XOpenDisplay(name.encode())
    if not dpy:
        return False
    lib.XCloseDisplay(dpy)
    return True


def _probe(name: str) -> bool:
    """在子进程里实测一个 display，超时即判为不可用。

    放进子进程是为了能被 kill：XOpenDisplay 卡住时进程内无法打断（见模块说明）。
    """
    try:
        proc = subprocess.run(
            [sys.executable, os.path.abspath(__file__), PROBE_FLAG, name],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=PROBE_TIMEOUT,
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return proc.returncode == 0


def _candidates() -> set:
    """候选 display 号：~/.Xauthority 登记过的 + 本机正监听 6000..6063 的。"""
    found = set()

    try:
        out = subprocess.run(
            ["xauth", "list"], capture_output=True, text=True, timeout=5
        ).stdout
        found.update(int(n) for n in re.findall(r":(\d+)\b", out))
    except Exception:
        pass

    for n in range(PORT_COUNT):
        sock = socket.socket()
        sock.settimeout(0.2)
        try:
            if sock.connect_ex(("127.0.0.1", PORT_BASE + n)) == 0:
                found.add(n)
        except Exception:
            pass
        finally:
            sock.close()

    return found


def available_displays() -> list:
    """返回可用 DISPLAY 列表，编号大（会话新）的在前。"""
    if _load_xlib() is None:  # 没有 libX11 就不必起子进程了
        return []

    names = ["localhost:%d.0" % n for n in sorted(_candidates(), reverse=True)]
    if not names:
        return []

    # 并行实测：整体最坏耗时 ≈ PROBE_TIMEOUT，而不是 候选数 x PROBE_TIMEOUT
    with futures.ThreadPoolExecutor(max_workers=min(PROBE_WORKERS, len(names))) as pool:
        usable = list(pool.map(_probe, names))

    return [name for name, ok in zip(names, usable) if ok]


def write_env(display: str) -> None:
    with open(ENV_FILE, "w", encoding="utf-8") as fp:
        fp.write("%s=%s\n" % (ENV_NAME, display))


def main(argv) -> int:
    # 子进程实测模式：只回一个退出码，不产生任何输出
    if len(argv) == 2 and argv[0] == PROBE_FLAG:
        lib = _load_xlib()
        return 0 if lib is not None and _usable(lib, argv[1]) else 1

    displays = available_displays()

    if "--write" in argv:
        if displays:
            write_env(displays[0])
            print("[x11] %s=%s  ->  %s" % (ENV_NAME, displays[0], ENV_FILE))
        else:
            if not os.path.exists(ENV_FILE):
                # 仅是注释行，作为 envFile 使用是安全的
                with open(ENV_FILE, "w", encoding="utf-8") as fp:
                    fp.write("# 未探测到可用的 %s\n" % ENV_NAME)
            print("[x11] 未探测到可用的 X11 DISPLAY（保留已有配置）", file=sys.stderr)
        return 0

    for name in displays:
        print(name)
    return 0


if __name__ == "__main__":
    # 主进程不再直接调用 XOpenDisplay（都在子进程里跑，其 stderr 由 _probe 丢弃），
    # 所以这里保留真实 stderr：探测失败的提示才看得见，不会像以前那样全程静默。
    sys.exit(main(sys.argv[1:]))
