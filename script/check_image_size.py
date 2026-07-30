#!/usr/bin/env python3
"""
图片尺寸校验工具
===============
遍历指定目录下所有图片，检查尺寸是否符合预期，不符合的打印出来。

用法:
    .venv/bin/python script/check_image_size.py 1920 1080 /path/to/images

    # 递归遍历子目录
    .venv/bin/python script/check_image_size.py 800 600 /path/to/images -r

    # 只检查特定格式
    .venv/bin/python script/check_image_size.py 1920 1080 /path/to/images -e png,jpg

参数:
    width               期望宽度
    height              期望高度
    folder              要检查的文件夹路径

可选参数:
    -r, --recursive     递归遍历子目录（默认只检查顶层）
    -e, --extensions    指定图片扩展名，逗号分隔（默认: png,jpg,jpeg,bmp,gif,webp）

依赖:
    - Pillow (pip install Pillow)
"""

import argparse
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("错误: 需要安装 Pillow 库，请执行: pip install Pillow")
    sys.exit(1)

# 默认支持的图片扩展名
DEFAULT_EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}


def collect_images(folder: str, extensions: set, recursive: bool) -> list:
    """收集目录下所有匹配扩展名的图片文件路径"""
    images = []
    if recursive:
        for root, _, files in os.walk(folder):
            for f in files:
                if os.path.splitext(f)[1].lower() in extensions:
                    images.append(os.path.join(root, f))
    else:
        for f in os.listdir(folder):
            full = os.path.join(folder, f)
            if os.path.isfile(full) and os.path.splitext(f)[1].lower() in extensions:
                images.append(full)
    return sorted(images)


def main():
    parser = argparse.ArgumentParser(
        description="检查目录下图片尺寸是否符合预期"
    )
    parser.add_argument("width", type=int, help="期望宽度")
    parser.add_argument("height", type=int, help="期望高度")
    parser.add_argument("folder", type=str, help="要检查的文件夹路径")
    parser.add_argument(
        "-r", "--recursive", action="store_true", help="递归遍历子目录"
    )
    parser.add_argument(
        "-e", "--extensions", type=str, default="png,jpg,jpeg,bmp,gif,webp",
        help="指定图片扩展名，逗号分隔（默认: png,jpg,jpeg,bmp,gif,webp）"
    )
    args = parser.parse_args()

    if not os.path.isdir(args.folder):
        print(f"错误: 文件夹不存在: {args.folder}")
        sys.exit(1)

    extensions = {f".{ext.strip().lower()}" for ext in args.extensions.split(",") if ext.strip()}
    expected = (args.width, args.height)

    images = collect_images(args.folder, extensions, args.recursive)

    if not images:
        print(f"未找到任何图片文件（扩展名: {args.extensions}）")
        return

    mismatch_count = 0
    total_count = len(images)

    print(f"期望尺寸: {args.width} x {args.height}")
    print(f"检查目录: {args.folder}")
    print(f"图片数量: {total_count}")
    print("-" * 50)

    for img_path in images:
        try:
            with Image.open(img_path) as img:
                size = img.size  # (width, height)
        except Exception as e:
            print(f"[错误] {img_path} — 无法读取: {e}")
            mismatch_count += 1
            continue

        if size != expected:
            mismatch_count += 1
            print(f"[尺寸不符] {img_path} — 实际: {size[0]} x {size[1]}")

    print("-" * 50)
    print(f"总计: {total_count} 张, 符合: {total_count - mismatch_count} 张, 不符: {mismatch_count} 张")

    if mismatch_count > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
