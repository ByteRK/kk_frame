'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-04-25 12:52:59
LastEditTime: 2026-10-23 11:12:55
FilePath: /kk_frame/build.py
Description: 项目构建脚本
BugList: 

Copyright (c) 2025 by Ricken, All Rights Reserved. 

'''

import os
import subprocess
import shutil
import re
import sys
import fnmatch
import argparse
from datetime import datetime

# 模板项目名
TEMPLATE_NAME = 'kk_frame'

# 模板父目录（cdroid 源码树的 apps 目录），用于读取上游提交 ID 与远程地址
TEMPLATE_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 颜色定义
COLOR = {
    'HEADER': '\033[95m',
    'BLUE': '\033[94m',
    'CYAN': '\033[96m',
    'GREEN': '\033[92m',
    'YELLOW': '\033[93m',
    'RED': '\033[91m',
    'BOLD': '\033[1m',
    'UNDERLINE': '\033[4m',
    'END': '\033[0m'
}

# 需要替换项目名的配置文件（相对项目根目录）
PROJECT_NAME_FILES = [
    'fastCheck.sh',              # x64 快速编译脚本
    'script/build.sh',           # 板端 Release 编译脚本
    'script/release.sh',         # 打包脚本
    'script/x64.sh',             # x64 辅助脚本
    'script/README.md',          # 脚本清单中的文件名
    'board/script/name.sh',      # 板端启动脚本的应用名来源
    'script/kk_frame.ps1',       # Windows 软链脚本（同时会重命名）
]

# 遍历时跳过的目录（不参与重命名与残留检查）
SKIP_DIRS = {'.git', 'tmp', 'dist', 'package', '.venv', '__pycache__', 'node_modules'}

# 复制模板时排除的内容，可写相对路径或文件名 glob
DEFAULT_IGNORE = [
    # 本地环境与缓存
    '.venv',
    '__pycache__',
    '*.pyc',
    'tmp',
    'dist',
    'package',
    '.vscode/x11/display.env',
    '.vscode/x11/picker.vsix',
    # 本机构建产物，首次构建时自动生成
    'config/app_version.h',
    'config/fonts_info.h',
    'fonts/fonts.conf',
    'library/library_config.h',
    'library/*/lib_*_config.h',
    'library/keyboard/pinyin/data/*.txt',
    'R.h',
    'assets/values/ID.xml',
    'project_changes.patch',
    'cdroid_changes.patch',
]

# 需要替换标识符（保留 kk_frame/ 路径引用）的源码目录
SOURCE_DIRS = ('./src/app', './src/widgets')

# 参与标识符替换的源码扩展名
SOURCE_EXTS = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx')

# 残留扫描的扩展名
SCAN_EXTS = (
    '.sh', '.ps1', '.json', '.h', '.hpp', '.c', '.cc', '.cpp',
    '.cmake', '.txt', '.xml', '.conf', '.py', '.md',
)

# 颜色输出辅助函数
def cprint(text, color=None, bold=False):
    """带颜色的打印函数"""
    if color and color in COLOR:
        style = COLOR[color]
        if bold:
            style += COLOR['BOLD']
        print(f"{style}{text}{COLOR['END']}")
    else:
        print(text)

def cinput(prompt, color=None, bold=False):
    """带颜色的输入函数"""
    if color and color in COLOR:
        style = COLOR[color]
        if bold:
            style += COLOR['BOLD']
        return input(f"{style}{prompt}{COLOR['END']}")
    else:
        return input(prompt)

# ---- 基础工具 ----

def git(args, cwd=None, check=False):
    """执行 git 命令，统一返回 CompletedProcess（文本模式）。"""
    return subprocess.run(['git', *args], cwd=cwd, check=check,
                          capture_output=True, text=True)


def read_text(path):
    """读取文本文件，UTF-8 优先，失败降级 GBK，最后容错读取。"""
    with open(path, 'rb') as f:
        data = f.read()
    for enc in ('utf-8', 'gbk'):
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            continue
    return data.decode('utf-8', errors='ignore')


def write_text(path, content):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)


def plain_replace(content, name):
    """裸替换：所有 'kk_frame' -> 项目名。"""
    return content.replace(TEMPLATE_NAME, name)


def id_replace_keep_path(content, name):
    """替换标识符，但保留 'kk_frame/' 路径引用（减少上游 merge 冲突）。"""
    return re.sub(rf'{TEMPLATE_NAME}(?!/)', name, content)


def replace_in_file(path, transform, name):
    """对单文件执行 transform，有变化则写回，返回是否更新。"""
    content = read_text(path)
    new_content = transform(content, name)
    if new_content == content:
        return False
    write_text(path, new_content)
    return True


def walk_replace(dirs, extensions, transform, name, label):
    """遍历目录下指定扩展名文件做替换，返回 (扫描数, 更新数)。"""
    scanned = updated = 0
    for target_dir in dirs:
        if not os.path.isdir(target_dir):
            cprint(f"\n目录 {target_dir} 不存在，跳过处理。", "YELLOW")
            continue

        cprint(f"\n开始处理目录: {target_dir}", "CYAN")
        for root, subdirs, files in os.walk(target_dir):
            subdirs[:] = [d for d in subdirs if d not in SKIP_DIRS]
            for file in files:
                if not file.endswith(extensions):
                    continue
                path = os.path.join(root, file)
                scanned += 1
                try:
                    if replace_in_file(path, transform, name):
                        cprint(f"  - 已更新: {os.path.relpath(path, '.')}", "GREEN")
                        updated += 1
                except Exception as e:
                    cprint(f"处理文件 {path} 时出错: {str(e)}", "RED")

    cprint(f"{label}: 扫描 {scanned} 个文件, 更新 {updated} 个", "CYAN", bold=True)
    return scanned, updated

# 获取Cdroid的Git提交ID
def get_template_parent_commit_id():
    try:
        commit_id = git(['rev-parse', 'HEAD'], cwd=TEMPLATE_PARENT_DIR).stdout.strip()
        if not commit_id:
            raise ValueError('empty commit id')
        cprint(f"\n获取到Cdroid的Git提交ID: {commit_id[:10]}...", "CYAN")
        return commit_id
    except Exception as e:
        cprint(f"\n警告: 无法获取模板父目录的Git提交ID ({e})", "YELLOW")
        return '未知'

# 渲染 README.md（读模板 docs/README_BASE.md，替换占位符）
def render_readme(name):
    base_path = './docs/README_BASE.md'
    output_path = 'README.md'

    if not os.path.isfile(base_path):
        cprint(f"文件 {base_path} 不存在。", "YELLOW")
        return

    content = read_text(base_path)
    updated = (content
               .replace('{{project_name}}', name)
               .replace('{{create_time}}', datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
               .replace('{{commit_id}}', get_template_parent_commit_id()))
    write_text(output_path, updated)
    cprint("已生成 README.md", "GREEN")


# 执行全部内容替换（源码、CMake、配置文件、README、文件名）
def apply_replacements(name):
    # 1. 源码目录：替换标识符，保留 kk_frame/ 路径引用
    walk_replace(list(SOURCE_DIRS), SOURCE_EXTS, id_replace_keep_path, name, '源码标识符')

    # 2. CMakeLists.txt：裸替换
    if os.path.isfile('CMakeLists.txt'):
        if replace_in_file('CMakeLists.txt', plain_replace, name):
            cprint("已生成项目cmake", "GREEN")
    else:
        cprint("文件 CMakeLists.txt 不存在。", "YELLOW")

    # 3. 配置文件列表：裸替换
    updated = 0
    for rel_path in PROJECT_NAME_FILES:
        if not os.path.isfile(rel_path):
            cprint(f"  - 跳过（不存在）: {rel_path}", "YELLOW")
            continue
        try:
            if replace_in_file(rel_path, plain_replace, name):
                cprint(f"  - 已更新配置: {rel_path}", "GREEN")
                updated += 1
        except Exception as e:
            cprint(f"处理文件 {rel_path} 时出错: {str(e)}", "RED")
    cprint(f"配置文件替换: 更新 {updated} 个文件", "GREEN")

    # 4. README 模板渲染
    render_readme(name)

    # 5. 文件名重命名
    rename_project_named_files(name)



# 重命名文件名中包含模板项目名的文件
def rename_project_named_files(new_project_name):
    renamed = 0

    for root, dirs, files in os.walk('.'):
        # 原地裁剪，避免进入无关目录
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]

        for file in files:
            if TEMPLATE_NAME not in file:
                continue

            old_path = os.path.join(root, file)
            new_path = os.path.join(root, file.replace(TEMPLATE_NAME, new_project_name))

            if os.path.exists(new_path):
                cprint(f"  - 跳过（目标已存在）: {new_path}", "YELLOW")
                continue

            try:
                os.rename(old_path, new_path)
                renamed += 1
                cprint(f"  - 已重命名: {os.path.relpath(old_path)} -> {os.path.basename(new_path)}", "GREEN")
            except Exception as e:
                cprint(f"重命名 {old_path} 时出错: {str(e)}", "RED")

    cprint(f"文件重命名: {renamed} 个", "GREEN")

# 扫描新项目中残留的模板名，便于人工确认
def report_remaining_template_refs(target_dir):

    # 有意保留模板名说明的文件
    allow_files = {
        'README.md',                        # 模板仓库门面
        'build.py',                         # 模板初始化脚本自身
        'docs/GUIDE.md',                    # 模板/框架说明
        'script/brand_encode.py',           # 署名示例
        'library/watchdog/README.md',       # 参数示例
        '.agents/build_validation.md',      # AI 指引
    }

    remaining = []

    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]

        for file in files:
            if not file.endswith(SCAN_EXTS):
                continue

            full_path = os.path.join(root, file)
            rel_path = os.path.relpath(full_path, target_dir).replace(os.sep, '/')
            if rel_path in allow_files:
                continue

            try:
                with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
            except Exception:
                continue

            # 忽略文件头 FilePath 等作者元信息行，这类路径不参与替换
            for line in lines:
                if 'FilePath' in line:
                    continue
                if TEMPLATE_NAME in line:
                    remaining.append(rel_path)
                    break

    if not remaining:
        cprint(f"\n项目名检查: 未发现残留的 '{TEMPLATE_NAME}'", "GREEN")
        return

    cprint(f"\n项目名检查: 以下 {len(remaining)} 个文件仍包含 '{TEMPLATE_NAME}'，请人工确认：", "YELLOW", bold=True)
    for rel_path in remaining:
        cprint(f"  - {rel_path}", "YELLOW")

# 获取模板仓库远程地址
def get_template_remote_url(project_dir):
    """依次尝试项目自身 origin、模板父目录 origin，失败返回 None。"""
    for cwd in (project_dir, TEMPLATE_PARENT_DIR):
        result = git(['config', '--get', 'remote.origin.url'], cwd=cwd)
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    return None


# 让用户选择 git 历史策略
def choose_history_strategy():
    cprint("\n" + "=" * 50, "CYAN")
    cprint("Git 历史策略", "BOLD")
    cprint("=" * 50, "CYAN")
    cprint("1. 保留模板历史：可与上游 kk_frame 正常 fetch + merge 同步", "CYAN")
    cprint("2. 全新历史：仓库干净独立，只能 cherry-pick 单个上游提交", "CYAN")
    while True:
        choice = cinput("请选择 (1/2): ", "BOLD").strip()
        if choice in ('1', 'keep'):
            return 'keep'
        if choice in ('2', 'fresh'):
            return 'fresh'
        cprint("请输入 1 或 2", "RED")


# 配置项目 origin
def configure_project_remote(project_dir, template_url, name, origin_url):
    """添加项目 origin；origin_url 为 None 时交互询问。"""
    if not origin_url:
        default_example = (template_url or '').replace(TEMPLATE_NAME, name) or None
        prompt = "请输入项目远程仓库地址"
        if default_example:
            prompt += f" (示例: {default_example})"
        prompt += ": "
        while True:
            origin_url = cinput(prompt, "BOLD").strip()
            if origin_url:
                break
            cprint("错误: 远程仓库地址不能为空", "RED")

    git(['remote', 'add', 'origin', origin_url], cwd=project_dir, check=True)
    git(['config', 'push.default', 'current'], cwd=project_dir)
    git(['config', 'remote.pushDefault', 'origin'], cwd=project_dir)
    cprint(f"已添加项目远程: origin -> {origin_url}", "GREEN")


# 检查 git 用户信息
def ensure_git_identity(project_dir):
    for key in ('user.name', 'user.email'):
        result = git(['config', '--get', key], cwd=project_dir)
        if result.returncode == 0 and result.stdout.strip():
            continue
        cprint(f"警告: 未配置 git {key}，初始提交可能失败", "YELLOW")
        cprint(f'  可稍后执行: git config {key} "your-value"', "CYAN")
        return False
    return True


# 按策略配置 git 仓库与远程
def configure_git(project_dir, name, strategy, add_template_remote, origin_url=None, ask_origin=True):
    git_dir = os.path.join(project_dir, '.git')
    if not os.path.exists(git_dir):
        cprint("警告: 项目目录下没有找到 .git，跳过 git 配置", "YELLOW")
        return

    template_url = get_template_remote_url(project_dir)
    if template_url:
        cprint(f"\n检测到模板远程地址: {template_url}", "BLUE", bold=True)
    else:
        cprint("警告: 无法获取模板远程地址", "YELLOW")

    if strategy == 'fresh':
        cprint("\n重新初始化 Git 仓库（全新历史）...", "CYAN")
        shutil.rmtree(git_dir)
        init_cmd = ['init']
        result = git(['config', '--global', 'init.defaultBranch'])
        if result.returncode != 0 or not result.stdout.strip():
            init_cmd += ['--initial-branch', 'master']
        git(init_cmd, cwd=project_dir, check=True)
        cprint("已重新初始化 Git 仓库（全新仓库）", "GREEN")
    else:
        # 保留模板历史，但清空模板自身 remote，避免误推
        result = git(['remote'], cwd=project_dir)
        for remote in result.stdout.split():
            if remote.strip():
                git(['remote', 'remove', remote.strip()], cwd=project_dir, check=True)
                cprint(f"  - 已删除远程: {remote.strip()}", "YELLOW")

    if add_template_remote and template_url:
        git(['remote', 'add', TEMPLATE_NAME, template_url], cwd=project_dir, check=True)
        git(['remote', 'set-url', '--push', TEMPLATE_NAME, 'DO_NOT_PUSH_TEMPLATE'],
            cwd=project_dir, check=True)
        cprint(f"已添加模板远程（只读）: {TEMPLATE_NAME} -> {template_url}", "GREEN")

    if origin_url:
        configure_project_remote(project_dir, template_url, name, origin_url)
    elif ask_origin:
        choice = cinput("\n是否配置项目远程仓库地址? (y/n): ", "BOLD").strip().lower()
        if choice in ('y', 'yes'):
            configure_project_remote(project_dir, template_url, name, None)
        else:
            git(['config', 'push.default', 'nothing'], cwd=project_dir)
            cprint("未配置项目远程，已设置 push.default='nothing' 防止误推", "YELLOW")
    else:
        git(['config', 'push.default', 'nothing'], cwd=project_dir)
        cprint("未配置项目远程，已设置 push.default='nothing' 防止误推", "YELLOW")

    # 全新历史时创建初始提交
    if strategy == 'fresh':
        if ensure_git_identity(project_dir):
            git(['add', '.'], cwd=project_dir, check=True)
            result = git(['commit', '-m', f'Initial commit for {name}'], cwd=project_dir)
            if result.returncode == 0:
                cprint("已创建初始提交", "GREEN")
            else:
                cprint("注意: 未创建初始提交，可稍后手动提交", "YELLOW")
        else:
            cprint("跳过初始提交（git 用户信息未配置）", "YELLOW")

    cprint("\n" + "=" * 50, "GREEN")
    cprint("Git 配置完成!", "GREEN", bold=True)
    cprint("=" * 50, "GREEN")
    git(['remote', '-v'], cwd=project_dir)

    cprint("\n常用命令:", "CYAN", bold=True)
    if origin_url:
        cprint("  推送代码: git push origin master", "CYAN")
    if strategy == 'keep' and add_template_remote:
        cprint(f"  获取模板更新: git fetch {TEMPLATE_NAME} && git merge {TEMPLATE_NAME}/master", "CYAN")
    elif add_template_remote:
        cprint(f"  挑选上游提交: git fetch {TEMPLATE_NAME} && git cherry-pick <commit>", "CYAN")
    cprint(f"  注意: 不要推送到 '{TEMPLATE_NAME}' 远程（已设置为只读）", "RED")



# 执行替换操作
def perform_replacements_in_target(target_dir, project_name):
    # 切换到目标目录
    original_cwd = os.getcwd()

    try:
        os.chdir(target_dir)
        cprint(f"\n已切换到新目录: {os.getcwd()}", "CYAN")
        apply_replacements(project_name)
    finally:
        os.chdir(original_cwd)
        cprint(f"已切换回原始目录: {os.getcwd()}", "CYAN")

# 复制模板所有内容到目标位置
def copy_directory_contents(src, dst, ignore=None):
    """复制目录内容，支持按相对路径或文件名排除。

    Args:
        src:    源目录路径
        dst:    目标目录路径
        ignore: 排除规则列表，可写相对路径（如 config/app_version.h）
                或文件名 glob（如 *.pyc），相对路径优先匹配
    """
    if ignore is None:
        ignore = []

    skipped = []

    def _should_ignore(dir_path, name):
        rel_path = os.path.relpath(os.path.join(dir_path, name), src).replace(os.sep, '/')
        for pattern in ignore:
            # 含路径分隔符时按相对路径匹配，否则只匹配文件名
            if '/' in pattern:
                if fnmatch.fnmatch(rel_path, pattern):
                    return True
            elif fnmatch.fnmatch(name, pattern):
                return True
        return False

    def _ignore_cb(dir_path, names):
        ignored = []
        for name in names:
            if _should_ignore(dir_path, name):
                ignored.append(name)
                skipped.append(os.path.relpath(os.path.join(dir_path, name), src))
        return ignored

    os.makedirs(dst, exist_ok=True)
    shutil.copytree(src, dst, symlinks=True,
                    ignore=_ignore_cb, dirs_exist_ok=True)

    for item in skipped:
        cprint(f"  - 已跳过: {item}", "YELLOW")

# 创建目标目录
def create_target_directory(target_dir):
    os.makedirs(target_dir, exist_ok=True)
    cprint(f"\n已创建项目目录: {target_dir}", "GREEN")
    return target_dir

# 检查目标路径
def prepare_target_directory(target_dir, assume_yes=False):
    # 检查目标路径是否存在
    if not os.path.exists(target_dir):
        return True
    # 提醒文件已存在，等待操作
    cprint(f"警告: 目录 '{target_dir}' 已存在！", "YELLOW")
    if assume_yes:
        choice = 'y'
    else:
        choice = cinput("是否删除现有目录并继续? (y/n): ", "BOLD").strip().lower()

    # 判断用户操作
    if choice == 'y':
        if os.path.isdir(target_dir):
            shutil.rmtree(target_dir)
        else:
            os.remove(target_dir)
        cprint(f"\n已删除现有目录: {target_dir}", "YELLOW")
        return True

    cprint("操作已取消。", "RED")
    return False

# 检查项目名称是否合法
def is_valid_project_name(name):
    # 使用正则表达式验证：只允许字母、数字和下划线
    if not re.match(r'^[a-zA-Z0-9_]+$', name):
        return False

    # 确保名称不为空
    if len(name.strip()) == 0:
        return False

    # 确保不以数字开头
    if re.match(r'^\d', name):
        return False

    # 禁止使用模板名本身，否则会误删模板
    if name == TEMPLATE_NAME:
        return False

    return True

# 打印项目命名规则（错误提示复用）
def print_name_rules():
    cprint("请确保项目名称:", "RED")
    cprint("- 只包含字母（a-z, A-Z）、数字（0-9）和下划线（_）", "RED")
    cprint("- 不以数字开头", "RED")
    cprint("- 不包含空格、短横线或其他特殊字符", "RED")
    cprint(f"- 不能与模板名 '{TEMPLATE_NAME}' 相同", "RED")


# 获取项目名称（交互式，非法则循环重试）
def get_valid_project_name():
    while True:
        name = cinput("请输入项目名称（只能使用字母、数字和下划线，且不能以数字开头）: ", "BOLD").strip()
        
        if is_valid_project_name(name):
            return name
        
        cprint("错误：项目名称不合法！", "RED")
        print_name_rules()
        cprint()

def parse_args():
    parser = argparse.ArgumentParser(description='从 kk_frame 模板初始化新项目')
    parser.add_argument('--name', help='项目名称（跳过交互）')
    parser.add_argument('--history', choices=['keep', 'fresh'], help='git 历史策略')
    parser.add_argument('--template-remote', action='store_true', help='添加只读模板远程')
    parser.add_argument('--origin', help='项目远程仓库地址')
    parser.add_argument('--yes', action='store_true', help='自动确认所有询问')
    return parser.parse_args()


def main():
    args = parse_args()
    new_project_dir = None
    try:
        # a. 获取项目名称（带验证）
        if args.name:
            new_project_name = args.name.strip()
            if not is_valid_project_name(new_project_name):
                cprint(f"错误：项目名称 '{new_project_name}' 不合法！", "RED")
                print_name_rules()
                sys.exit(1)
        else:
            new_project_name = get_valid_project_name()

        # b. 计算目标目录
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        new_project_dir = os.path.join(parent_dir, new_project_name)

        # c. 检查目标目录
        if not prepare_target_directory(new_project_dir, assume_yes=args.yes):
            return

        # d. 创建目标目录
        create_target_directory(new_project_dir)

        # e. 复制当前目录内容到目标目录
        cprint("\n正在复制项目文件...", "CYAN")
        cprint(f"已排除: {', '.join(DEFAULT_IGNORE)}", "YELLOW")
        copy_directory_contents(current_dir, new_project_dir, ignore=DEFAULT_IGNORE)
        cprint(f"已复制所有文件到: {new_project_dir}", "GREEN")

        # f. 切换到目标目录并执行替换操作
        perform_replacements_in_target(new_project_dir, new_project_name)

        # g. 检查是否还有残留的模板项目名
        report_remaining_template_refs(new_project_dir)

        # h. 配置 git（失败不删除项目）
        strategy = args.history or choose_history_strategy()

        if args.template_remote:
            add_template_remote = True
        elif args.history or args.yes:
            add_template_remote = False
        else:
            add_template_remote = cinput("是否添加只读模板远程（便于同步上游）? (y/n): ", "BOLD").strip().lower() in ('y', 'yes')

        ask_origin = not (args.origin or args.yes or args.history)
        try:
            configure_git(new_project_dir, new_project_name, strategy,
                          add_template_remote, origin_url=args.origin, ask_origin=ask_origin)
        except Exception as e:
            cprint(f"\nGit 配置失败: {str(e)}", "RED")
            cprint("项目文件已生成，可稍后手动配置 git", "YELLOW")

        # i. 输出成功信息
        cprint(f"\n------------------------- SUCCESS -------------------------", "GREEN", bold=True)
        cprint(f"已成功创建项目: '{new_project_name}'", "GREEN", bold=True)
        cprint(f"项目路径: '{new_project_dir}'", "GREEN", bold=True)
        cprint(f"\n后续步骤:", "CYAN")
        cprint(f"1. 打开新项目: {new_project_dir}", "CYAN")
        cprint(f"2. 运行 ./fastCheck.sh 进行编译", "CYAN")
    except KeyboardInterrupt:
        cprint("\n\n操作已取消。", "RED")
        sys.exit(0)
    except Exception as e:
        cprint(f"\n构建过程中出错: {str(e)}", "RED")
        if new_project_dir and os.path.isdir(new_project_dir):
            cprint(f"清理未完成的目录: {new_project_dir}", "YELLOW")
            shutil.rmtree(new_project_dir, ignore_errors=True)
        sys.exit(1)

if __name__ == "__main__":
    main()
