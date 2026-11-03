#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
kk_frame 产测「网络测试」本地 API 服务器。

仅使用 Python 标准库，无第三方依赖。配合产测页面的网络测试项使用，
端点覆盖 HttpManager 的连通性 / 下载落盘 / 取消 / 退避重试 / POST 等行为。

用法:
    python3 http_test_server.py [--host 127.0.0.1] [--port 18080]

端点:
    GET  /ping      -> 200 "pong"                       (连通性)
    GET  /download  -> 200 一段可落盘内容               (下载落盘)
    GET  /slow      -> 200 分块慢速发送(约10s)          (取消下载)
    GET  /flaky     -> 前 2 次超时, 第 3 次 200         (退避重试)
    POST /echo      -> 200 回显请求体                   (POST)
"""

import argparse
import os
import random
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18080

# /flaky 前 N 次请求故意延迟(秒)，需大于客户端 requestTimeoutMs 才能触发超时重试
FLAKY_FAIL_COUNT = 2
FLAKY_DELAY = 5.0

# /slow 分块参数：块数 * 块大小 / 间隔
SLOW_CHUNK_COUNT = 20
SLOW_CHUNK_SIZE = 1024
SLOW_CHUNK_INTERVAL = 0.5

# /big 大文件大小（字节）
BIG_SIZE = 4 * 1024 * 1024
BIG_DATA = os.urandom(BIG_SIZE)

# /random 返回 500 的概率
RANDOM_FAIL_RATE = 0.2


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"  # 支持 keep-alive，配合客户端连接池复用
    flaky_counter = 0

    def log_message(self, fmt, *args):
        sys.stderr.write("[%s] %s\n" % (self.log_date_time_string(), fmt % args))

    def _send(self, code, body, ctype="text/plain"):
        data = body.encode("utf-8") if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if data:
            self.wfile.write(data)

    def _send_head(self, code, length, ctype="text/plain"):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(length))
        self.end_headers()

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path == "/ping":
            self._send(200, "pong")
        elif path == "/download":
            self._send(200, "kk_frame http test payload\n" * 64)
        elif path == "/slow":
            self._send_slow()
        elif path == "/flaky":
            self._send_flaky()
        elif path == "/big":
            self._send(200, BIG_DATA, "application/octet-stream")
        elif path == "/random":
            if random.random() < RANDOM_FAIL_RATE:
                self._send(500, "random failure")
            else:
                self._send(200, "random ok")
        else:
            self._send(404, "not found")

    def do_HEAD(self):
        path = self.path.split("?", 1)[0]
        if path == "/ping":
            self._send_head(200, 4)
        elif path == "/big":
            self._send_head(200, BIG_SIZE, "application/octet-stream")
        else:
            self._send_head(404, 9)

    def do_POST(self):
        path = self.path.split("?", 1)[0]
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""
        if path == "/echo":
            self._send(200, "echo: " + body.decode("utf-8", "replace"))
        else:
            self._send(404, "not found")

    def do_PUT(self):
        path = self.path.split("?", 1)[0]
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""
        if path == "/upload":
            self._send(200, "uploaded %d bytes" % len(body))
        else:
            self._send(404, "not found")

    def _send_slow(self):
        total = SLOW_CHUNK_COUNT * SLOW_CHUNK_SIZE
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(total))
        self.end_headers()
        chunk = b"x" * SLOW_CHUNK_SIZE
        for _ in range(SLOW_CHUNK_COUNT):
            try:
                self.wfile.write(chunk)
                self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                # 客户端取消下载主动断开，正常结束
                break
            time.sleep(SLOW_CHUNK_INTERVAL)

    def _send_flaky(self):
        # 确定性行为：前 FLAKY_FAIL_COUNT 次请求故意延迟触发客户端超时重试，
        # 之后一次立即成功并归零计数，使每次「重试」测试都独立完整地跑通
        type(self).flaky_counter += 1
        attempt = type(self).flaky_counter

        if attempt <= FLAKY_FAIL_COUNT:
            # 故意延迟超过客户端超时，触发网络层超时 -> 退避重试
            time.sleep(FLAKY_DELAY)
            try:
                self._send(200, "late")
            except (BrokenPipeError, ConnectionResetError):
                pass
        else:
            self._send(200, "flaky ok (attempt %d)" % attempt)
            # 本次测试成功，归零以便下一次独立测试从头开始
            type(self).flaky_counter = 0


def main():
    parser = argparse.ArgumentParser(description="kk_frame 网络测试本地 API 服务器")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print("listening on http://%s:%d" % (args.host, args.port))
    print("endpoints: /ping /download /slow /flaky /echo")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
