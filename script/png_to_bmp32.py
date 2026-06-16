#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
将 PNG 图片转换为 32 位 BMP 图片。

用法示例：

1. 保留透明通道，默认模式：
   python png_to_bmp32.py input.png

2. 明确保留透明通道：
   python png_to_bmp32.py input.png --keep-alpha

3. 不保留透明通道，透明区域填充为黑色：
   python png_to_bmp32.py input.png --no-alpha --bg-color "#000000"

4. 不保留透明通道，透明区域填充为白色：
   python png_to_bmp32.py input.png --no-alpha --bg-color "#FFFFFF"

5. 批量转换目录：
   python png_to_bmp32.py ./png_dir -o ./bmp_dir --no-alpha --bg-color "#FFFFFF"

6. 递归转换目录：
   python png_to_bmp32.py ./png_dir -o ./bmp_dir -r --no-alpha --bg-color "#FFFFFF"

支持的颜色格式：
   "#FFFFFF"
   "#FFF"
   "255,255,255"
   "red"
   "black"
   "white"
"""

import argparse
from pathlib import Path
from PIL import Image, ImageColor


def parse_color(value: str):
    """
    解析颜色参数，返回 (R, G, B)。

    支持：
    - "#FFFFFF"
    - "#FFF"
    - "255,255,255"
    - "red"
    - "black"
    - "white"
    """
    value = value.strip()

    if "," in value:
        parts = value.split(",")
        if len(parts) != 3:
            raise argparse.ArgumentTypeError(
                f"颜色格式错误: {value}, 应为 R,G,B，例如 255,255,255"
            )

        try:
            r, g, b = [int(x.strip()) for x in parts]
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"颜色格式错误: {value}, RGB 必须是数字"
            )

        for c in (r, g, b):
            if c < 0 or c > 255:
                raise argparse.ArgumentTypeError(
                    f"颜色值超出范围: {value}, 每个通道应为 0~255"
                )

        return r, g, b

    try:
        return ImageColor.getrgb(value)[:3]
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"无法识别颜色: {value}"
        )


def is_32bit_bmp(path: Path) -> bool:
    """
    检查 BMP 位深。
    BMP 文件头中第 28 字节处是 bits per pixel，长度 2 字节，小端。
    """
    try:
        with path.open("rb") as f:
            data = f.read(30)
            if len(data) < 30:
                return False
            return int.from_bytes(data[28:30], byteorder="little") == 32
    except Exception:
        return False


def make_bmp32_image(
    img: Image.Image,
    keep_alpha: bool,
    bg_color
) -> Image.Image:
    """
    生成 32 位 BMP 所需的 RGBA 图像。

    keep_alpha=True:
        保留 PNG 的 Alpha 通道。

    keep_alpha=False:
        将透明区域按 bg_color 合成，
        最终 Alpha 通道全部设置为 255。
    """
    rgba = img.convert("RGBA")

    if keep_alpha:
        return rgba

    r, g, b = bg_color

    background = Image.new("RGBA", rgba.size, (r, g, b, 255))

    # 正确处理半透明像素，而不是简单替换全透明像素
    composed = Image.alpha_composite(background, rgba)

    # 保证仍然是 32 位 BMP，但 Alpha 全部为 255
    composed.putalpha(255)

    return composed


def convert_png_to_bmp32(
    src: Path,
    dst: Path,
    keep_alpha: bool,
    bg_color,
    overwrite: bool = False
) -> bool:
    if not src.exists():
        print(f"[ERROR] 输入文件不存在: {src}")
        return False

    if src.suffix.lower() != ".png":
        print(f"[SKIP] 不是 PNG 文件: {src}")
        return False

    if dst.exists() and not overwrite:
        print(f"[SKIP] 文件已存在: {dst}")
        return False

    dst.parent.mkdir(parents=True, exist_ok=True)

    try:
        with Image.open(src) as img:
            bmp32 = make_bmp32_image(
                img=img,
                keep_alpha=keep_alpha,
                bg_color=bg_color
            )

            bmp32.save(dst, format="BMP")

        if not is_32bit_bmp(dst):
            print(f"[WARN] 已生成，但检测到不是 32 位 BMP: {dst}")
            return False

        alpha_text = "保留 Alpha" if keep_alpha else f"填充背景色 RGB{bg_color}"
        print(f"[OK] {src} -> {dst} [{alpha_text}]")
        return True

    except Exception as e:
        print(f"[ERROR] 转换失败: {src}, 原因: {e}")
        return False


def collect_png_files(input_path: Path, recursive: bool):
    if input_path.is_file():
        return [input_path]

    if input_path.is_dir():
        if recursive:
            return sorted(input_path.rglob("*.png"))
        return sorted(input_path.glob("*.png"))

    return []


def build_output_path(
    src: Path,
    input_path: Path,
    output_path: Path,
    recursive: bool
) -> Path:
    if input_path.is_file():
        if output_path:
            if output_path.suffix.lower() == ".bmp":
                return output_path
            return output_path / (src.stem + ".bmp")

        return src.with_suffix(".bmp")

    if output_path:
        if recursive:
            rel = src.relative_to(input_path)
            return output_path / rel.with_suffix(".bmp")

        return output_path / (src.stem + ".bmp")

    return src.with_suffix(".bmp")


def main():
    parser = argparse.ArgumentParser(
        description="将 PNG 图片转换为 32 位 BMP 图片"
    )

    parser.add_argument(
        "input",
        help="输入 PNG 文件或目录"
    )

    parser.add_argument(
        "-o", "--output",
        help="输出 BMP 文件或输出目录。不指定时输出到输入文件同目录"
    )

    parser.add_argument(
        "-r", "--recursive",
        action="store_true",
        help="递归处理目录中的 PNG 文件"
    )

    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="覆盖已存在的输出文件"
    )

    alpha_group = parser.add_mutually_exclusive_group()

    alpha_group.add_argument(
        "--keep-alpha",
        action="store_true",
        default=True,
        help="保留透明通道，默认开启"
    )

    alpha_group.add_argument(
        "--no-alpha",
        action="store_false",
        dest="keep_alpha",
        help="不保留透明通道，将透明区域按背景色填充"
    )

    parser.add_argument(
        "--bg-color",
        type=parse_color,
        default=(0, 0, 0),
        help='不保留透明通道时的填充颜色，默认 "#000000"'
    )

    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_path = Path(args.output).resolve() if args.output else None

    png_files = collect_png_files(input_path, args.recursive)

    if not png_files:
        print(f"[ERROR] 未找到 PNG 文件: {input_path}")
        return 1

    success = 0
    failed = 0

    for src in png_files:
        dst = build_output_path(
            src=src,
            input_path=input_path,
            output_path=output_path,
            recursive=args.recursive
        )

        if convert_png_to_bmp32(
            src=src,
            dst=dst,
            keep_alpha=args.keep_alpha,
            bg_color=args.bg_color,
            overwrite=args.overwrite
        ):
            success += 1
        else:
            failed += 1

    print()
    print(f"完成：成功 {success} 个，失败/跳过 {failed} 个")

    return 0 if failed == 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())