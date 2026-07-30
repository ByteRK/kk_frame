#!/usr/bin/env python3
"""
PNG 图片批量压缩工具
=====================
递归遍历目录下所有 PNG 图片，使用 Pillow + pngquant 进行激进压缩。

用法:
    # 预览模式（不修改文件）
    .venv/bin/python script/png_compress.py path/to/images --dry-run

    # 原地压缩
    .venv/bin/python script/png_compress.py path/to/images

    # 保留原文件到 backup 目录
    .venv/bin/python script/png_compress.py path/to/images --backup

    # 指定另一个目录
    .venv/bin/python script/png_compress.py /path/to/images

压缩策略:
    ① Alpha 全为 255 → 剥离 RGBA → RGB（无损，-25%）
    ② Pillow 256 色调色板量化（有损-视觉无损，-40%~-60%）
    ③ Pillow optimize=True 保存（无损，-5%~-10%）
    ④ pngquant 二次精压（有损-视觉无损，额外 -20%~-40%）

依赖:
    - Pillow (pip install Pillow)  -- 必须
    - pngquant (apt install pngquant || pip install pngquant-cli) -- 可选，未安装时自动跳过第④步
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Optional, Tuple

try:
    from PIL import Image
except ImportError:
    sys.exit("错误: 需要 Pillow 库，请运行: pip install Pillow")

# ---------------------------------------------------------------------------
# 配置常量
# ---------------------------------------------------------------------------
_DEFAULT_PNGQUANT_QUALITY = "92-97"  # pngquant 质量范围
PILLOW_QUANTIZE_COLORS = 256  # Pillow 量化颜色数
MAX_WORKERS = os.cpu_count() or 4  # 并行线程数
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"  # PNG 文件头魔数

# ---------------------------------------------------------------------------
# 工具函数
# ---------------------------------------------------------------------------


def is_png(filepath: Path) -> bool:
    """通过文件头魔数判断是否为 PNG 文件（而非扩展名）。"""
    try:
        with open(filepath, "rb") as f:
            return f.read(8) == PNG_SIGNATURE
    except OSError:
        return False


def file_md5(filepath: Path) -> str:
    """计算文件 MD5。"""
    h = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def fmt_size(num_bytes: int) -> str:
    """人类可读的文件大小。"""
    for unit in ("B", "KB", "MB", "GB"):
        if num_bytes < 1024:
            return f"{num_bytes:.1f} {unit}"
        num_bytes /= 1024
    return f"{num_bytes:.1f} TB"


def _find_pngquant() -> Optional[str]:
    """查找 pngquant 可执行文件路径。
    优先使用当前 Python 解释器同目录下的 pngquant（即 pip install pngquant-cli），
    其次搜索系统 PATH。
    """
    # 优先: 与 Python 解释器同级的 pngquant（venv/bin/pngquant）
    venv_bin = Path(sys.executable).parent / "pngquant"
    if venv_bin.is_file():
        return str(venv_bin)

    # 次选: 系统 PATH
    found = shutil.which("pngquant")
    return found


def check_pngquant() -> Tuple[bool, Optional[str]]:
    """检查 pngquant 是否可用，返回 (可用, 路径)。"""
    path = _find_pngquant()
    if not path:
        return False, None
    try:
        subprocess.run(
            [path, "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        return True, path
    except (FileNotFoundError, subprocess.CalledProcessError):
        return False, None


# ---------------------------------------------------------------------------
# 压缩核心
# ---------------------------------------------------------------------------


def strip_alpha(img: Image.Image) -> Tuple[Image.Image, bool]:
    """
    如果 Alpha 通道全为 255（完全不透明），转换为 RGB。
    返回 (图像, 是否已转换)。
    """
    if img.mode != "RGBA":
        return img, False
    alpha = img.getchannel('A')
    extrema = alpha.getextrema()
    if extrema == (255, 255):
        return img.convert("RGB"), True
    return img, False


def compress_one(
    filepath: Path,
    dry_run: bool = False,
    backup_dir: Optional[Path] = None,
    use_pngquant: bool = True,
    pngquant_quality: str = _DEFAULT_PNGQUANT_QUALITY,
    pngquant_path: str = "pngquant",
    scan_root: Optional[Path] = None,
) -> dict:
    """
    压缩单张 PNG 图片。

    返回:
        {
            "path": str,        # 相对路径
            "original": int,    # 原始大小 (bytes)
            "compressed": int,  # 压缩后大小 (bytes)
            "stages": [str],    # 各阶段记录
            "error": str|None,  # 错误信息
        }
    """
    result = {
        "path": str(filepath),
        "original": 0,
        "compressed": 0,
        "stages": [],
        "error": None,
    }

    try:
        original_size = filepath.stat().st_size
        result["original"] = original_size

        # ---- 第①步: 读取并剥离 Alpha ----
        img = Image.open(filepath)
        img.load()
        img, stripped = strip_alpha(img)
        if stripped:
            result["stages"].append("RGBA→RGB")

        # ---- 第②步: Pillow 调色板量化 ----
        # pngquant 可用时跳过此步骤，避免双重量化叠加降低质量
        if not use_pngquant and img.mode in ("RGBA", "RGB"):
            try:
                if img.mode == "RGBA":
                    # 分离 Alpha 通道，仅量化 RGB 部分后再合并，避免透明度丢失
                    alpha = img.getchannel('A')
                    img = img.convert('RGB').quantize(
                        colors=PILLOW_QUANTIZE_COLORS,
                        method=Image.Quantize.MEDIANCUT,
                    ).convert('RGBA')
                    img.putalpha(alpha)
                    result["stages"].append(f"quantize({PILLOW_QUANTIZE_COLORS}c)+alpha")
                else:
                    img = img.quantize(
                        colors=PILLOW_QUANTIZE_COLORS,
                        method=Image.Quantize.MEDIANCUT,
                    )
                    result["stages"].append(f"quantize({PILLOW_QUANTIZE_COLORS}c)")
            except Exception:
                # 某些图片可能量化失败（如灰度图），跳过
                pass

        # ---- 第③步: Pillow optimize 保存到临时文件 ----
        tmp_path = filepath.with_suffix(filepath.suffix + ".tmp")
        img.save(tmp_path, format="PNG", optimize=True)
        result["stages"].append("pillow-opt")

        # ---- 第④步: pngquant 二次精压 ----
        if use_pngquant:
            tmp2_path = filepath.with_suffix(filepath.suffix + ".tmp2")
            rc = subprocess.run(
                [
                    pngquant_path,
                    "--quality",
                    pngquant_quality,
                    "--speed",
                    "1",
                    "--force",
                    "--output",
                    str(tmp2_path),
                    str(tmp_path),
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            ).returncode
            if rc == 0 and tmp2_path.stat().st_size < tmp_path.stat().st_size:
                tmp_path.unlink()
                tmp_path = tmp2_path
                result["stages"].append(f"pngquant({pngquant_quality})")
            elif tmp2_path.exists():
                tmp2_path.unlink()

        # ---- 确认效果：只有变小才替换 ----
        compressed_size = tmp_path.stat().st_size
        if compressed_size < original_size:
            result["compressed"] = compressed_size
            if not dry_run:
                if backup_dir:
                    rel = filepath.relative_to(scan_root) if scan_root else filepath.name
                    backup_path = backup_dir / rel
                    backup_path.parent.mkdir(parents=True, exist_ok=True)
                    # 仅首次备份
                    if not backup_path.exists():
                        shutil.copy2(filepath, backup_path)
                # 替换原文件
                shutil.move(str(tmp_path), str(filepath))
            else:
                tmp_path.unlink()
        else:
            # 没变小，保留原文件
            result["compressed"] = original_size
            result["stages"].append("(skip: no reduction)")
            tmp_path.unlink()

    except Exception as exc:
        result["error"] = str(exc)
        # 清理可能残留的临时文件
        for suf in (".tmp", ".tmp2"):
            p = filepath.with_suffix(filepath.suffix + suf)
            if p.exists():
                p.unlink()

    return result


# ---------------------------------------------------------------------------
# 扫描器
# ---------------------------------------------------------------------------


def scan_images(root: Path) -> list:
    """
    递归扫描目录下所有 PNG 文件。
    通过魔数判断，不依赖扩展名（也兼容 .jpg 伪装的 PNG）。
    """
    images = []
    for entry in sorted(root.rglob("*")):
        if entry.is_file() and is_png(entry):
            images.append(entry)
    return images


# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="PNG 批量压缩工具 — Pillow + pngquant 激进方案",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s path/to/images --dry-run        # 预览压缩效果
  %(prog)s path/to/images                  # 原地压缩
  %(prog)s path/to/images --backup         # 压缩并保留原文件到 path/to/images_backup/
  %(prog)s path/to/images --no-pngquant    # 跳过 pngquant，仅用 Pillow
  %(prog)s path/to/images --quality 40-70  # 更激进的 pngquant 质量参数
        """,
    )
    parser.add_argument(
        "target",
        help="要压缩的目录路径",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="预览模式: 不修改任何文件，仅输出压缩预估",
    )
    parser.add_argument(
        "--backup",
        action="store_true",
        help="在目标目录同级创建 *_backup 备份原文件",
    )
    parser.add_argument(
        "--no-pngquant",
        action="store_true",
        help="禁用 pngquant，仅使用 Pillow 压缩",
    )
    parser.add_argument(
        "--quality",
        default=_DEFAULT_PNGQUANT_QUALITY,
        help=f"pngquant 质量范围 (默认: {_DEFAULT_PNGQUANT_QUALITY})",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=MAX_WORKERS,
        help=f"并行线程数 (默认: {MAX_WORKERS})",
    )
    args = parser.parse_args()

    pngquant_quality = args.quality

    # ---- 解析目录 ----
    target = Path(args.target).resolve()
    if not target.is_dir():
        sys.exit(f"错误: 目录不存在 —— {target}")

    # ---- pngquant 检测 ----
    use_pngquant, pngquant_path = check_pngquant()
    if args.no_pngquant:
        use_pngquant = False
    if not use_pngquant and not args.no_pngquant:
        print("⚠ 未检测到 pngquant，将仅使用 Pillow 压缩")
        print("  安装: pip install pngquant-cli  或  sudo apt install pngquant\n")

    # ---- 备份目录 ----
    backup_dir = None
    if args.backup:
        backup_dir = target.parent / (target.name + "_backup")

    # ---- 扫描 ----
    print(f"🔍 扫描目录: {target}")
    images = scan_images(target)
    if not images:
        print("  未找到 PNG 文件")
        return

    total_original = sum(p.stat().st_size for p in images)
    print(f"  找到 {len(images)} 张 PNG，原始大小 {fmt_size(total_original)}")
    if args.dry_run:
        print("  [预览模式] 不修改任何文件\n")
    elif args.backup:
        print(f"  备份目录: {backup_dir}\n")
    else:
        print()

    # ---- 并发压缩 ----
    print(f"⚙️  开始压缩 (并行 {args.jobs} 线程)...\n")
    start_time = time.monotonic()

    results = []
    completed = 0

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {
            pool.submit(
                compress_one,
                p,
                dry_run=args.dry_run,
                backup_dir=backup_dir,
                use_pngquant=use_pngquant,
                pngquant_quality=pngquant_quality,
                pngquant_path=pngquant_path or "pngquant",
                scan_root=target,
            ): p
            for p in images
        }
        for fut in as_completed(futures):
            r = fut.result()
            results.append(r)
            completed += 1

            # 实时进度
            fname = futures[fut].name
            orig = r["original"]
            comp = r["compressed"]
            if r["error"]:
                status = "❌"
            elif comp < orig:
                saved = orig - comp
                pct = saved / orig * 100
                status = f"✅ -{fmt_size(saved)} ({pct:.0f}%)"
            else:
                status = "➖ 无变化"
            stages = " → ".join(r["stages"]) if r["stages"] else "-"
            print(f"  [{completed}/{len(images)}] {fname}  {status}  |  {stages}")
            if r["error"]:
                print(f"         错误: {r['error']}")

    elapsed = time.monotonic() - start_time

    # ---- 汇总报告 ----
    success = [r for r in results if not r["error"] and r["compressed"] < r["original"]]
    skipped = [r for r in results if not r["error"] and r["compressed"] >= r["original"]]
    failed = [r for r in results if r["error"]]

    sum_original = sum(r["original"] for r in results)
    sum_compressed = sum(r["compressed"] for r in results)
    saved = sum_original - sum_compressed

    print(f"\n{'=' * 55}")
    print(f"  压缩{'预览' if args.dry_run else ''}完成")
    print(f"{'=' * 55}")
    print(f"  文件总数:    {len(results)}")
    print(f"  成功压缩:    {len(success)}")
    print(f"  跳过(无益):  {len(skipped)}")
    if failed:
        print(f"  失败:        {len(failed)}")
    print(f"  原始大小:    {fmt_size(sum_original)}")
    print(f"  压缩后大小:  {fmt_size(sum_compressed)}")
    if sum_original > 0:
        print(f"  节省空间:    {fmt_size(saved)} ({saved / sum_original * 100:.1f}%)")
    print(f"  耗时:        {elapsed:.1f}s")
    print()

    if args.dry_run:
        print("💡 使用以下命令执行实际压缩:")
        cmd = f"  .venv/bin/python script/png_compress.py {args.target}"
        if args.backup:
            cmd += " --backup"
        if args.no_pngquant:
            cmd += " --no-pngquant"
        print(cmd)


if __name__ == "__main__":
    main()
