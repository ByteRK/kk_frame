#!/usr/bin/env python3
"""打包并安装「X11 Display Picker」扩展到 VS Code 服务端。

为什么用 VSIX + 官方 CLI 而不是直接复制目录：
  扩展清单（extensions.json）由 VS Code 维护，且每个 profile 各有一份。
  手工复制目录 + 手写清单条目在运行中的服务端里不会被识别，会报
  "command 'pickX11Display' not found"。走官方安装流程才能正确登记，
  并且会落到「当前窗口正在使用的 profile」里。

用法:
    python3 install.py

安装后需要执行一次 “Developer: Reload Window” 才能生效。
"""

from __future__ import annotations

import glob
import json
import os
import re
import subprocess
import sys
import zipfile
from xml.sax.saxutils import escape

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "picker")
VSIX = os.path.join(HERE, "picker.vsix")

# IPC 调用超时（秒）：窗口忙或正在重载时，code CLI 会阻塞在转发上
INSTALL_TIMEOUT = 120
LIST_TIMEOUT = 30

# 只打包这些后缀，避免把无关内容带进 VSIX
INCLUDE_SUFFIXES = (".js", ".json", ".md")

CONTENT_TYPES = """<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".js" ContentType="application/javascript"/>
  <Default Extension=".json" ContentType="application/json"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
  <Default Extension=".md" ContentType="text/markdown"/>
</Types>
"""

MANIFEST = """<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="{name}" Version="{version}" Publisher="{publisher}" />
    <DisplayName>{display_name}</DisplayName>
    <Description xml:space="preserve">{description}</Description>
    <Tags>x11,display,debug</Tags>
    <Categories>Other</Categories>
    <GalleryFlags>Public</GalleryFlags>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code" />
  </Installation>
  <Dependencies />
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true" />
  </Assets>
</PackageManifest>
"""


def log(msg: str) -> None:
    print("[x11] " + msg)


def build_vsix(pkg: dict) -> None:
    """按 package.json 的元信息打包 VSIX（publisher/name/version 必须一致）。"""
    manifest = MANIFEST.format(
        name=escape(pkg["name"]),
        version=escape(pkg["version"]),
        publisher=escape(pkg["publisher"]),
        display_name=escape(pkg.get("displayName", pkg["name"])),
        description=escape(pkg.get("description", "")),
    )

    if os.path.exists(VSIX):
        os.remove(VSIX)

    with zipfile.ZipFile(VSIX, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("[Content_Types].xml", CONTENT_TYPES)
        zf.writestr("extension.vsixmanifest", manifest)

        for root, dirs, files in os.walk(SRC):
            dirs[:] = [d for d in dirs if d not in ("node_modules", ".git")]
            for name in sorted(files):
                if not name.endswith(INCLUDE_SUFFIXES):
                    continue
                full = os.path.join(root, name)
                rel = os.path.relpath(full, SRC)
                zf.write(full, os.path.join("extension", rel))

    log("已生成 %s" % VSIX)


def find_cli():
    """定位与运行中服务端匹配的 code CLI 包装脚本。"""
    servers = os.path.join(os.path.expanduser("~"), ".vscode-server", "cli", "servers")
    parts = ("server", "bin", "remote-cli", "code")

    # 优先匹配 ps 中运行中服务端的 commit，避免选到其它版本的 CLI
    try:
        ps = subprocess.run(
            ["ps", "-eo", "cmd"], capture_output=True, text=True, timeout=10
        ).stdout
        m = re.search(r"servers/Stable-([0-9a-f]{40})", ps)
        if m:
            cand = os.path.join(servers, "Stable-" + m.group(1), *parts)
            if os.access(cand, os.X_OK):
                return cand
    except Exception:
        pass

    # 退回：取最近修改的服务端目录
    cands = glob.glob(os.path.join(servers, "Stable-*", *parts))
    for cand in sorted(cands, key=os.path.getmtime, reverse=True):
        if os.access(cand, os.X_OK):
            return cand
    return None


def main() -> int:
    pkg_path = os.path.join(SRC, "package.json")
    if not os.path.isfile(pkg_path):
        print("[x11] 未找到扩展源码：%s" % SRC, file=sys.stderr)
        return 1

    with open(pkg_path, encoding="utf-8") as fp:
        pkg = json.load(fp)

    # VS Code 会把 publisher 规范化为小写，安装目录也用这个 ID
    ext_id = (pkg["publisher"] + "." + pkg["name"]).lower()

    build_vsix(pkg)

    cli = find_cli()
    if cli is None:
        print("[x11] 未找到 code CLI，无法安装扩展", file=sys.stderr)
        print(
            "[x11] 请改用 VS Code 的 “Extensions: Install from VSIX...” 手动安装：%s"
            % VSIX,
            file=sys.stderr,
        )
        return 1
    log("使用 CLI: %s" % cli)
    # code CLI 会把请求转发给正在运行的窗口并阻塞等待，窗口忙时可能长时间无响应，
    # 所以这些调用都设超时，避免命令无限期挂起。
    log("正在安装（若窗口繁忙可能需等待一会）…")

    # --force 保证重复执行也能覆盖更新
    try:
        rc = subprocess.run(
            [cli, "--install-extension", VSIX, "--force"], timeout=INSTALL_TIMEOUT
        ).returncode
    except subprocess.TimeoutExpired:
        rc = -1
        print(
            "[x11] 安装超时（>%ds）：窗口可能正忙或正在重载。" % INSTALL_TIMEOUT,
            file=sys.stderr,
        )

    if rc != 0:
        if rc != -1:
            print("[x11] 安装失败", file=sys.stderr)
        print(
            "[x11] 可尝试在 VS Code 里执行 “Extensions: Install from VSIX...” 并选择：%s"
            % VSIX,
            file=sys.stderr,
        )
        return 1

    # 校验登记结果（大小写以 VS Code 规范化结果为准）；超时则跳过校验
    try:
        listed = subprocess.run(
            [cli, "--list-extensions"],
            capture_output=True,
            text=True,
            timeout=LIST_TIMEOUT,
        ).stdout
        installed = {
            line.strip().lower() for line in listed.splitlines() if line.strip()
        }
        if ext_id in installed:
            log("安装成功（%s），已登记到当前窗口使用的 profile" % ext_id)
        else:
            print("[x11] 警告：安装后未在扩展列表中看到 %s" % ext_id, file=sys.stderr)
    except subprocess.TimeoutExpired:
        print(
            "[x11] 校验超时（>%ds）：跳过检查，安装命令本身已成功返回"
            % LIST_TIMEOUT,
            file=sys.stderr,
        )

    log("请执行 “Developer: Reload Window” 重载窗口使扩展生效")
    return 0


if __name__ == "__main__":
    sys.exit(main())
