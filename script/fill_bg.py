#!/usr/bin/env python3
'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-10-27 09:42:38
LastEditTime: 2025-10-27 10:21:34
FilePath: /kk_frame/script/fill_bg.py
Description: fill_bg.py - 为 PNG 批量填充底色、去透明、去元数据，输出最小化图像文件。
BugList: 

Copyright (c) 2025 by Ricken, All Rights Reserved. 

'''
# 具体使用方式运行python fill_bg.py --help

import argparse
import os
import sys
import concurrent.futures

# 检查 Pillow 是否安装
try:
    from PIL import Image, PngImagePlugin
except ImportError:
    print("❌ 缺少依赖：未检测到 Pillow 模块。\n请执行以下命令安装：")
    print("    pip install pillow")
    print("    或者")
    print("    sudo apt install python3-pil")
    sys.exit(1)

# ========== 工具函数部分 ==========

def parse_color(s: str):
    """解析十六进制颜色字符串 -> (r,g,b)"""
    s = s.strip().lower()
    if s.startswith("0x"):
        s = s[2:]
    if s.startswith("#"):
        s = s[1:]
    if len(s) == 3:
        s = ''.join(ch * 2 for ch in s)
    if len(s) != 6:
        raise ValueError(f"invalid color format: '{s}'")
    try:
        r, g, b = int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)
    except ValueError:
        raise ValueError(f"invalid hex digits in color: '{s}'")
    return (r, g, b)

def process_image(path_in: str, path_out: str, bg_rgb: tuple, fmt="PNG", quality=90):
    """叠加背景色，去透明度与元数据，输出最小化图片"""
    img = Image.open(path_in).convert("RGBA")
    bg = Image.new("RGBA", img.size, bg_rgb + (255,))
    composed = Image.alpha_composite(bg, img).convert("RGB")

    clean = Image.new("RGB", composed.size)
    clean.paste(composed)

    save_kwargs = {"format": fmt, "optimize": True}
    if fmt.upper() == "PNG":
        pnginfo = PngImagePlugin.PngInfo()  # 空 metadata，彻底干净
        clean.save(path_out, pnginfo=pnginfo, **save_kwargs)
    else:  # JPEG
        clean.save(path_out, quality=quality, optimize=True,
                   progressive=True, subsampling="4:2:0")

def iter_png_files(input_path: str, recursive: bool):
    """生成 (文件路径, 相对路径)"""
    if os.path.isfile(input_path):
        if input_path.lower().endswith(".png"):
            yield (input_path, os.path.basename(input_path))
        return
    base = os.path.abspath(input_path)
    for root, dirs, files in os.walk(base):
        for fn in files:
            if fn.lower().endswith(".png"):
                full = os.path.join(root, fn)
                rel = os.path.relpath(full, base)
                yield (full, rel)
        if not recursive:
            break

def handle_file(idx, total, full_in, rel, out_full, bg_rgb, fmt, quality, verbose):
    """单文件处理逻辑（可在线程池中调用）"""
    try:
        print(f"[{idx}/{total}] 正在处理：{rel}")
        os.makedirs(os.path.dirname(out_full), exist_ok=True)
        process_image(full_in, out_full, bg_rgb, fmt, quality)
        if verbose:
            print(f"    输出 -> {out_full}")
        return True
    except Exception as e:
        print(f"✖ 处理失败 {rel}: {e}", file=sys.stderr)
        return False

# ========== 主逻辑部分 ==========

def main(argv=None):
    epilog_text = """
example：
  python fill_bg.py -i logo.png --overwrite
      → 直接处理单张图片，输出覆盖原文件

  python fill_bg.py -i ./input --overwrite
      → 使用默认黑底，覆盖输入目录中的图片

  python fill_bg.py -c "#FFFFFF" -i ./input --overwrite
      → 使用白底填充，覆盖输入目录中的图片

  python fill_bg.py -i ./input -o ./out --recursive -v
      → 递归处理 input 目录，输出到 out，并显示详细日志

  python fill_bg.py -i ./input --to-jpg --quality 85
      → 转为 JPG 输出，质量为 85

  python fill_bg.py -i ./input --to-jpg --quality 85 --threads 8
      → 转为 JPG 输出，质量为 85，使用 8 个线程并行处理
"""

    parser = argparse.ArgumentParser(
        description="批量为 PNG 添加底色、去透明与元数据，输出最小体积图像文件。",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog_text
    )

    parser.add_argument("-c", "--color", default="#000000",
                        help="底色（十六进制），默认 '#000000' (黑色)")
    parser.add_argument("-i", "--input", required=True,
                        help="输入文件或目录路径（仅处理 .png 文件）")
    parser.add_argument("-o", "--output",
                        help="输出文件或目录路径；若未指定则覆盖输入路径")
    parser.add_argument("-r", "--recursive", action="store_true",
                        help="当输入为目录时递归子目录")
    parser.add_argument("--overwrite", action="store_true",
                        help="允许覆盖已存在文件（默认跳过）")
    parser.add_argument("--to-jpg", action="store_true",
                        help="输出为 JPG 格式（有损但更小）")
    parser.add_argument("--quality", type=int, default=90,
                        help="JPG 压缩质量 (1-100, 默认 90)")
    parser.add_argument("--threads", type=int, default=0,
                        help="并行线程数（默认 0=关闭并行）")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="输出详细处理信息")

    args = parser.parse_args(argv)


    # 解析颜色
    try:
        bg_rgb = parse_color(args.color)
    except Exception as e:
        print(f"❌ 无效颜色：{e}", file=sys.stderr)
        parser.print_usage(sys.stderr)
        return 2

    if not os.path.exists(args.input):
        print(f"❌ 输入路径不存在：{args.input}", file=sys.stderr)
        return 3

    fmt = "JPEG" if args.to_jpg else "PNG"
    ext = ".jpg" if args.to_jpg else ".png"

    input_is_file = os.path.isfile(args.input)
    output_path = args.output or args.input  # ✅ 自动使用输入路径

    # 单文件模式
    if input_is_file:
        out_file = output_path
        if os.path.isdir(out_file):
            out_file = os.path.join(out_file, os.path.basename(args.input))
        if not out_file.lower().endswith(ext):
            out_file = os.path.splitext(out_file)[0] + ext
        if os.path.exists(out_file) and not args.overwrite:
            print(f"⚠️ 输出文件已存在，跳过：{out_file}")
            return 0
        print(f"🪵 正在处理：{args.input}")
        process_image(args.input, out_file, bg_rgb, fmt, args.quality)
        print(f"✅ 输出：{out_file}")
        return 0

    # 目录模式
    files = list(iter_png_files(args.input, args.recursive))
    total = len(files)
    if total == 0:
        print("⚠️ 未在目录中找到任何 PNG 文件。")
        return 0

    os.makedirs(output_path, exist_ok=True)

    print(f"🧮 共找到 {total} 张图片，开始处理...")

    success, errors, skipped = 0, 0, 0
    tasks = []

    # 并行或串行执行
    if args.threads > 0:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.threads) as executor:
            futures = []
            for idx, (full_in, rel) in enumerate(files, 1):
                out_full = os.path.join(output_path, os.path.splitext(rel)[0] + ext)
                if os.path.exists(out_full) and not args.overwrite:
                    skipped += 1
                    if args.verbose:
                        print(f"[{idx}/{total}] 跳过已存在：{rel}")
                    continue
                futures.append(executor.submit(
                    handle_file, idx, total, full_in, rel, out_full,
                    bg_rgb, fmt, args.quality, args.verbose
                ))
            for f in concurrent.futures.as_completed(futures):
                if f.result():
                    success += 1
                else:
                    errors += 1
    else:
        for idx, (full_in, rel) in enumerate(files, 1):
            out_full = os.path.join(output_path, os.path.splitext(rel)[0] + ext)
            if os.path.exists(out_full) and not args.overwrite:
                skipped += 1
                if args.verbose:
                    print(f"[{idx}/{total}] 跳过已存在：{rel}")
                continue
            ok = handle_file(idx, total, full_in, rel, out_full,
                             bg_rgb, fmt, args.quality, args.verbose)
            if ok:
                success += 1
            else:
                errors += 1

    print(f"\n🎉 处理完成：总计 {total} 张，成功 {success}，跳过 {skipped}，失败 {errors}")
    return 0 if errors == 0 else 1

if __name__ == "__main__":
    sys.exit(main())