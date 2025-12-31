'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-04-25 12:52:59
LastEditTime: 2025-12-31 10:03:44
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
from datetime import datetime
import configparser

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

# 获取Cdroid的Git提交ID
def get_template_parent_commit_id():
    # 获取当前脚本所在目录（模板目录）
    current_script_dir = os.path.dirname(os.path.abspath(__file__))
    # 获取模板的父目录
    template_parent_dir = os.path.dirname(current_script_dir)
    
    try:
        # 在模板的父目录中执行git命令
        commit_id = subprocess.check_output(
            ['git', 'rev-parse', 'HEAD'],
            cwd=template_parent_dir
        ).strip().decode('utf-8')
        cprint(f"\n获取到Cdroid的Git提交ID: {commit_id[:10]}...", "CYAN")
        return commit_id
    except subprocess.CalledProcessError:
        cprint("\n警告: 无法获取模板父目录的Git提交ID", "YELLOW")
        return '未知'
    except Exception as e:
        cprint(f"\n获取Git提交ID时出错: {str(e)}", "RED")
        return '未知'

# 替换源文件中所有文件的 'kk_frame' ID为项目名称
def replace_id_in_source_files(new_project_name, directories):
    total_files = 0
    total_replacements = 0
    
    for target_dir in directories:
        # 检查目录是否存在
        if not os.path.isdir(target_dir):
            cprint(f"\n目录 {target_dir} 不存在，跳过处理。", "YELLOW")
            continue
        
        cprint(f"\n开始处理目录: {target_dir}", "CYAN")
        dir_files = 0
        dir_replacements = 0
        
        # 遍历目录中的所有文件
        for root, dirs, files in os.walk(target_dir):
            for file in files:
                file_path = os.path.join(root, file)
                
                try:
                    # 只处理文本文件（根据扩展名判断）
                    if file_path.endswith(('.c', '.cc', '.cpp', '.h', '.hpp')):
                        # 读取文件内容
                        with open(file_path, 'r', encoding='utf-8') as f:
                            content = f.read()
                        
                        # 检查是否需要替换（排除顶部注释）
                        if 'kk_frame' in content:
                            # 第一步：将 "kk_frame/" 替换为临时标记 "kk__frame/"
                            temp_content = content.replace('kk_frame/', 'kk__frame/')
                            
                            # 第二步：将剩余的 "kk_frame" 替换为新项目名称
                            new_content = temp_content.replace('kk_frame', new_project_name)
                            
                            # 第三步：将临时标记 "kk__frame" 恢复为 "kk_frame"
                            final_content = new_content.replace('kk__frame', 'kk_frame')
                            
                            # 只有在内容发生变化时才写回文件
                            if final_content != content:
                                # 写回文件
                                with open(file_path, 'w', encoding='utf-8') as f:
                                    f.write(final_content)
                                
                                cprint(f"  - 已更新文件: {os.path.relpath(file_path, target_dir)}", "GREEN")
                                dir_replacements += 1
                        
                        dir_files += 1
                except Exception as e:
                    cprint(f"处理文件 {file_path} 时出错: {str(e)}", "RED")
        
        cprint(f"完成处理: 扫描 {dir_files} 个文件, 更新 {dir_replacements} 个文件", "CYAN")
        total_files += dir_files
        total_replacements += dir_replacements
    
    cprint(f"\n总计: 扫描 {total_files} 个文件, 更新 {total_replacements} 个文件", "CYAN", bold=True)

# 替换 README_BASE.md
def replace_placeholders_in_readme(new_project_name):
    # 指定 README_BASE.md 文件路径
    readme_base_path = './docs/README_BASE.md'
    readme_output_path = 'README.md'

    # 检查文件是否存在
    if not os.path.isfile(readme_base_path):
        cprint(f"文件 {readme_base_path} 不存在。", "YELLOW")
        return

    # 获取当前时间
    create_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # 获取Cdroid的 Git 提交 ID
    commit_id = get_template_parent_commit_id()

    # 读取 README_BASE.md 文件内容
    with open(readme_base_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # 替换占位符
    updated_content = content.replace('{{project_name}}', new_project_name)
    updated_content = updated_content.replace('{{create_time}}', create_time)
    updated_content = updated_content.replace('{{commit_id}}', commit_id)

    # 将修改后的内容写入 README.md
    with open(readme_output_path, 'w', encoding='utf-8') as file:
        file.write(updated_content)

    cprint(f"已生成 README.md", "GREEN")

# 替换 CMakeLists.txt
def replace_project_name_in_cmake(new_project_name):
    # 指定 CMakeLists.txt 文件路径
    cmake_file_path = 'CMakeLists.txt'

    # 检查文件是否存在
    if not os.path.isfile(cmake_file_path):
        cprint(f"文件 {cmake_file_path} 不存在。", "YELLOW")
        return

    # 读取文件内容
    with open(cmake_file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # 替换指定文本
    updated_content = content.replace('kk_frame', new_project_name)

    # 将修改后的内容写回文件
    with open(cmake_file_path, 'w', encoding='utf-8') as file:
        file.write(updated_content)

    cprint(f"已生成项目cmake", "GREEN")

# 更新Git远程仓库地址
def update_git_remote(project_dir, template_name="kk_frame"):
    """更新新项目的Git远程仓库配置"""
    git_config_path = os.path.join(project_dir, '.git', 'config')
    
    # 检查git配置文件是否存在
    if not os.path.exists(git_config_path):
        cprint(f"警告: Git配置文件不存在: {git_config_path}", "YELLOW")
        return
    
    try:
        # 读取当前的origin地址
        config = configparser.ConfigParser()
        config.read(git_config_path)
        
        # 获取原始的origin URL
        if 'remote "origin"' in config and 'url' in config['remote "origin"']:
            original_url = config['remote "origin"']['url']
            cprint(f"\n当前Git远程地址: {original_url}", "BLUE", bold=True)
        else:
            cprint("警告: 无法找到origin远程地址", "YELLOW")
            return
        
        # 询问用户是否要更新git地址
        choice = cinput("\n是否要更新Git远程仓库地址? (y/n): ", "BOLD").strip().lower()
        
        if choice != 'y' and choice != 'yes':
            cprint("跳过Git远程地址更新。", "YELLOW")
            return
        
        # 获取新的仓库地址
        while True:
            new_url = cinput("请输入新的Git仓库地址: ", "BOLD").strip()
            if new_url:
                break
            cprint("错误: Git仓库地址不能为空", "RED")
        
        # 询问用户是否要重新初始化Git仓库
        reinit_choice = cinput("\n是否要删除.git目录并重新初始化Git仓库? (y/n): ", "BOLD").strip().lower()
        
        if reinit_choice == 'y' or reinit_choice == 'yes':
            # 重新初始化Git仓库
            git_dir = os.path.join(project_dir, '.git')
            try:
                shutil.rmtree(git_dir)
                cprint(f"已删除.git目录: {git_dir}", "YELLOW")
                
                # 检查用户是否已配置默认分支名
                try:
                    result = subprocess.run(
                        ['git', 'config', '--global', 'init.defaultBranch'],
                        capture_output=True,
                        text=True,
                        cwd=project_dir
                    )
                    has_default_branch = result.returncode == 0 and result.stdout.strip()
                except:
                    has_default_branch = False
                
                # 使用--initial-branch参数避免警告
                if not has_default_branch:
                    # 使用master作为默认分支名
                    subprocess.run(
                        ['git', 'init', '--initial-branch=master'], 
                        cwd=project_dir, 
                        check=True
                    )
                    cprint("\n已重新初始化Git仓库 (使用master分支)", "GREEN")
                else:
                    subprocess.run(['git', 'init'], cwd=project_dir, check=True)
                    cprint("\n已重新初始化Git仓库", "GREEN")
                
                # 添加新的远程仓库
                subprocess.run(['git', 'remote', 'add', 'origin', new_url], cwd=project_dir, check=True)
                cprint(f"已添加新的origin远程: {new_url}", "GREEN")
                
                cprint("\nGit仓库已重新初始化并配置完成!", "GREEN", bold=True)
                cprint("\n注意: 后续若模板更新需手动同步", "YELLOW")
                
            except Exception as e:
                cprint(f"删除并重新初始化Git仓库时出错: {str(e)}", "RED")
        else:
            # 不重新初始化，保留原来的origin并重命名为模板名称
            # 1. 将原来的origin重命名为模板名称
            if f'remote "{template_name}"' not in config:
                config[f'remote "{template_name}"'] = config['remote "origin"']
                cprint(f"已添加模板远程: {template_name} -> {original_url}", "GREEN")
            
            # 2. 更新origin为新地址
            config['remote "origin"']['url'] = new_url
            cprint(f"已更新origin远程: origin -> {new_url}", "GREEN")
            
            # 写入配置文件
            with open(git_config_path, 'w', encoding='utf-8') as configfile:
                config.write(configfile)
            
            cprint("\nGit远程仓库配置已更新:", "GREEN", bold=True)
            cprint(f"  origin: {new_url}", "GREEN")
            cprint(f"  {template_name}: {original_url}", "GREEN")
            cprint(f"\n注意: 保留了模板远程地址，可以通过 'git pull {template_name} master' 获取模板更新", "YELLOW")
        
    except Exception as e:
        cprint(f"更新Git远程配置时出错: {str(e)}", "RED")

# 执行替换操作
def perform_replacements_in_target(target_dir, project_name):
    # 切换到目标目录
    original_cwd = os.getcwd()
    
    try:
        os.chdir(target_dir)
        cprint(f"\n已切换到新目录: {os.getcwd()}", "CYAN")
        
        replace_project_name_in_cmake(project_name)
        replace_placeholders_in_readme(project_name)

        # 替换源文件中的项目名称（处理多个目录）
        source_dirs = [
            './src/app',
            './src/widgets'
        ]
        replace_id_in_source_files(project_name, source_dirs)
    finally:
        os.chdir(original_cwd)
        cprint(f"已切换回原始目录: {os.getcwd()}", "CYAN")

# 复制模板所有内容到目标位置
def copy_directory_contents(src, dst):
    os.makedirs(dst, exist_ok=True)
    
    # 遍历替换
    for item in os.listdir(src):
        src_path = os.path.join(src, item)
        dst_path = os.path.join(dst, item)
        
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dst_path, symlinks=True, 
                           ignore_dangling_symlinks=True, dirs_exist_ok=True)
        else:
            shutil.copy2(src_path, dst_path)

# 创建目标目录
def create_target_directory(target_dir):
    os.makedirs(target_dir, exist_ok=True)
    cprint(f"\n已创建项目目录: {target_dir}", "GREEN")
    return target_dir

# 检查目标路径
def prepare_target_directory(target_dir):
    # 检查目标路径是否存在
    if not os.path.exists(target_dir):
        return True
    # 提醒文件已存在，等待操作
    cprint(f"警告: 目录 '{target_dir}' 已存在！", "YELLOW")
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
    
    return True

# 获取项目名称
def get_valid_project_name():
    try:
        while True:
            name = cinput("请输入项目名称（只能使用字母、数字和下划线，且不能以数字开头）: ", "BOLD").strip()
            
            if is_valid_project_name(name):
                return name
            
            cprint("错误：项目名称不合法！", "RED")
            cprint("请确保项目名称:", "RED")
            cprint("- 只包含字母（a-z, A-Z）、数字（0-9）和下划线（_）", "RED")
            cprint("- 不以数字开头", "RED")
            cprint("- 不包含空格、短横线或其他特殊字符\n", "RED")
    except KeyboardInterrupt:
        # 当用户按下 Ctrl+C 时
        cprint("\n\n操作已取消。", "RED")
        sys.exit(0)  # 优雅地退出程序

# 主程序
def main():
    # a. 获取项目名称（带验证）
    new_project_name = get_valid_project_name()
    
    # b. 计算目标目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(current_dir)
    new_project_dir = os.path.join(parent_dir, new_project_name)
    
    # c. 检查目标目录
    if not prepare_target_directory(new_project_dir):
        return
    
    # d. 创建目标目录
    create_target_directory(new_project_dir)
    
    # e. 复制当前目录内容到目标目录
    cprint("\n正在复制项目文件...", "CYAN")
    copy_directory_contents(current_dir, new_project_dir)
    cprint(f"已复制所有文件到: {new_project_dir}", "GREEN")
    
    # f. 切换到目标目录并执行替换操作
    perform_replacements_in_target(new_project_dir, new_project_name)
    
    # g. 更新Git远程仓库配置
    update_git_remote(new_project_dir, "kk_frame")
    
    # h. 输出成功信息
    cprint(f"\n------------------------- SUCCESS -------------------------", "GREEN", bold=True)
    cprint(f"已成功创建项目: '{new_project_name}'", "GREEN", bold=True)
    cprint(f"项目路径: '{new_project_dir}'", "GREEN", bold=True)
    cprint(f"\n后续步骤:", "CYAN")
    cprint(f"1. 返回到 out 目录", "CYAN")
    cprint(f"2. 运行 'touch ../apps/CMakeLists.txt'", "CYAN")
    cprint(f"3. 运行 'make -j10' 编译项目", "CYAN")
    cprint(f"\n", "YELLOW")

if __name__ == "__main__":
    main()