#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
============================================================
Audio Gain Tool（Non-Recursive, Flat Directory）
------------------------------------------------------------
功能：
    对音频文件增加增益，仅处理单层目录文件（不递归）。

依赖：
    系统需安装 ffmpeg
        Linux: sudo apt install ffmpeg

使用：
    python audio_gain_batch.py
    python audio_gain_batch.py -i ./music -o ./out -g 6

特点：
    - 只处理当前目录文件
    - 自动跳过所有子目录
    - 默认输出 ./output_audio
    - 同名文件直接覆盖
============================================================
"""

import argparse
from pathlib import Path
import subprocess


SUPPORTED_FORMATS = {
    ".mp3", ".wav", ".flac", ".aac", ".m4a", ".ogg", ".wma"
}


def is_audio_file(file_path: Path) -> bool:
    return file_path.suffix.lower() in SUPPORTED_FORMATS


def collect_audio_files(input_path: Path):
    """
    收集音频文件（仅当前目录，不递归）
    """
    if input_path.is_file():
        return [input_path] if is_audio_file(input_path) else []

    files = []

    for f in input_path.iterdir():
        if f.is_file() and is_audio_file(f):
            files.append(f)

    return files


def process_file(src_file: Path, dst_file: Path, gain_db: float):
    try:
        dst_file.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            "ffmpeg",
            "-y",                      # 覆盖输出文件
            "-i", str(src_file),
            "-filter:a", f"volume={gain_db}dB",
            str(dst_file),
        ]

        subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )

        print(f"[OK] {src_file} -> {dst_file} (+{gain_db} dB)")

    except subprocess.CalledProcessError as e:
        print(f"[FAIL] {src_file}: ffmpeg 执行失败")
        print(e.stderr)

    except Exception as e:
        print(f"[FAIL] {src_file}: {e}")


def main():
    parser = argparse.ArgumentParser(description="音频增益工具（单层目录）")

    parser.add_argument(
        "-i", "--input",
        default=".",
        help="输入文件或目录（默认当前目录）"
    )

    parser.add_argument(
        "-o", "--output",
        default=None,
        help="输出路径（默认 ./output_audio）"
    )

    parser.add_argument(
        "-g", "--gain",
        type=float,
        default=6.0,
        help="增益(dB)，默认 6.0"
    )

    args = parser.parse_args()

    input_path = Path(args.input).resolve()

    if not input_path.exists():
        raise FileNotFoundError(f"输入路径不存在: {input_path}")

    # 输出路径
    if args.output is None:
        output_path = Path("./output_audio").resolve()
    else:
        output_path = Path(args.output).resolve()

    # === 单文件模式 ===
    if input_path.is_file() and output_path.suffix:
        process_file(input_path, output_path, args.gain)
        return

    # === 目录模式 ===
    output_path.mkdir(parents=True, exist_ok=True)

    audio_files = collect_audio_files(input_path)

    if not audio_files:
        print("未找到音频文件")
        return

    for src in audio_files:
        dst = output_path / src.name
        process_file(src, dst, args.gain)


if __name__ == "__main__":
    main()