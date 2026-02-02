'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-04-25 12:52:59
LastEditTime: 2026-02-02 08:01:35
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

# 替换 R.h
def replace_r_h(new_project_name):
    # 指定 R.h 文件路径
    r_h_file_path = 'R.h'

    # 检查文件是否存在
    if not os.path.isfile(r_h_file_path):
        cprint(f"文件 {r_h_file_path} 不存在。", "YELLOW")
        return

    # 读取文件内容
    with open(r_h_file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # 替换指定文本
    updated_content = content.replace('kk_frame', new_project_name)

    # 将修改后的内容写回文件
    with open(r_h_file_path, 'w', encoding='utf-8') as file:
        file.write(updated_content)

    cprint(f"已生成项目R.h", "GREEN")

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
def update_git_remote(project_dir, new_project_name, template_name="kk_frame"):
    """更新新项目的Git远程仓库配置"""
    git_dir = os.path.join(project_dir, '.git')
    
    # 检查是否存在.git目录
    if not os.path.exists(git_dir):
        cprint(f"警告: 项目目录下没有找到.git目录", "YELLOW")
        return
    
    try:
        # 获取原始的origin URL（模板地址）
        result = subprocess.run(
            ['git', 'config', '--get', 'remote.origin.url'],
            cwd=project_dir,
            capture_output=True,
            text=True
        )
        
        template_url = None
        if result.returncode == 0:
            template_url = result.stdout.strip()
            cprint(f"\n检测到模板远程地址: {template_url}", "BLUE", bold=True)
        else:
            cprint("警告: 无法获取origin远程地址", "YELLOW")
            # 如果获取失败，尝试直接读取config文件获取
            git_config_path = os.path.join(git_dir, 'config')
            if os.path.exists(git_config_path):
                config = configparser.ConfigParser()
                config.read(git_config_path)
                if 'remote "origin"' in config and 'url' in config['remote "origin"']:
                    template_url = config['remote "origin"']['url']
                    cprint(f"从配置文件读取到模板远程地址: {template_url}", "BLUE", bold=True)
        
        # 如果没有获取到模板地址，尝试从环境或默认获取
        if not template_url:
            # 尝试从构建脚本所在目录获取模板地址
            current_script_dir = os.path.dirname(os.path.abspath(__file__))
            try:
                # 获取模板目录的远程地址
                result = subprocess.run(
                    ['git', 'config', '--get', 'remote.origin.url'],
                    cwd=os.path.dirname(current_script_dir),
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    template_url = result.stdout.strip()
                    cprint(f"从模板目录获取到远程地址: {template_url}", "BLUE", bold=True)
            except:
                pass
            
            if not template_url:
                cprint("提示: 无法获取模板远程地址", "YELLOW")
                # 提供一个默认的模板地址示例
                default_template_url = "https://github.com/username/kk_frame.git"
                template_url = cinput(f"请输入模板远程地址 (示例: {default_template_url}): ", "BOLD").strip()
                if not template_url:
                    template_url = default_template_url
        
        # 步骤3: 询问用户是否重新初始化Git仓库
        cprint("\n" + "="*50, "CYAN")
        cprint("Git仓库初始化选项", "BOLD")
        cprint("="*50, "CYAN")
        
        cprint("\n您有以下两种选择:", "CYAN")
        cprint("1. 保留现有Git仓库（包含模板的提交历史）", "CYAN")
        cprint("2. 重新初始化Git仓库（创建全新的提交历史）", "CYAN")
        
        reinit_choice = cinput("\n是否要删除.git目录并重新初始化Git仓库? (y/n): ", "BOLD").strip().lower()
        cprint("注意: 重新初始化会删除所有现有的提交历史，创建全新的仓库", "YELLOW")
        
        # 初始化变量，确保它们在所有分支中都有定义
        project_choice = 'n'  # 默认不配置项目远程
        template_choice = 'n'  # 默认不配置模板远程
        
        if reinit_choice in ['y', 'yes']:
            # 重新初始化Git仓库（全新仓库，不保留模板关联）
            try:
                # 删除.git目录
                shutil.rmtree(git_dir)
                cprint(f"\n已删除.git目录: {git_dir}", "YELLOW")
                
                # 初始化新仓库
                init_cmd = ['git', 'init']
                
                # 检查用户是否已配置默认分支名
                try:
                    default_branch_result = subprocess.run(
                        ['git', 'config', '--global', 'init.defaultBranch'],
                        capture_output=True,
                        text=True
                    )
                    if default_branch_result.returncode != 0 or not default_branch_result.stdout.strip():
                        # 使用master作为默认分支
                        init_cmd.extend(['--initial-branch', 'master'])
                        cprint("使用master作为默认分支", "CYAN")
                except:
                    pass
                
                # 执行初始化
                subprocess.run(init_cmd, cwd=project_dir, check=True)
                cprint("已重新初始化Git仓库（全新仓库）", "GREEN")
                
                # 询问是否要添加模板远程作为参考（可选）
                cprint("\n" + "-"*50, "CYAN")
                cprint("模板远程配置（可选）", "CYAN")
                cprint("-"*50, "CYAN")
                
                template_choice = cinput(f"是否要添加模板远程 '{template_name}' 作为参考? (y/n): ", "BOLD").strip().lower()
                
                if template_choice in ['y', 'yes']:
                    # 添加模板远程（只读）
                    subprocess.run(['git', 'remote', 'add', template_name, template_url], 
                                cwd=project_dir, check=True)
                    subprocess.run(['git', 'remote', 'set-url', '--push', template_name, 'DO_NOT_PUSH_TEMPLATE'], 
                                cwd=project_dir, check=True)
                    cprint(f"已添加模板远程（只读）: {template_name} -> {template_url}", "GREEN")
                    cprint("注意: 此远程仅供参考，没有提交历史关联", "YELLOW")
                else:
                    cprint("未添加模板远程", "YELLOW")
                
                # 步骤4: 询问是否配置项目远程仓库（可选）
                cprint("\n" + "="*50, "CYAN")
                cprint("项目远程仓库配置（可选）", "BOLD")
                cprint("="*50, "CYAN")
                
                project_choice = cinput("\n是否要配置项目远程仓库地址? (y/n): ", "BOLD").strip().lower()
                
                if project_choice in ['y', 'yes']:
                    # 获取用户的项目仓库地址
                    default_example = template_url.replace('kk_frame', new_project_name)
                    
                    while True:
                        project_url = cinput(f"请输入您的项目远程仓库地址 (示例: {default_example}): ", "BOLD").strip()
                        
                        if project_url:
                            break
                        cprint("错误: 远程仓库地址不能为空", "RED")
                    
                    # 添加项目远程
                    subprocess.run(['git', 'remote', 'add', 'origin', project_url], 
                                cwd=project_dir, check=True)
                    cprint(f"已添加项目远程: origin -> {project_url}", "GREEN")
                    
                    # 设置安全配置
                    try:
                        subprocess.run(['git', 'config', 'push.default', 'current'], 
                                    cwd=project_dir, check=True)
                    except:
                        pass
                else:
                    cprint("未配置项目远程仓库地址", "YELLOW")
                    cprint("您可以稍后使用以下命令配置:", "CYAN")
                    cprint(f"  git remote add origin <您的仓库地址>", "CYAN")
                    # 设置更安全的默认配置
                    try:
                        subprocess.run(['git', 'config', 'push.default', 'nothing'], 
                                    cwd=project_dir, check=True)
                        cprint(f"已设置push.default为'nothing'，防止意外推送", "GREEN")
                    except:
                        pass
                
                # 添加初始提交
                subprocess.run(['git', 'add', '.'], cwd=project_dir, check=True)
                commit_result = subprocess.run(
                    ['git', 'commit', '-m', f'Initial commit for {new_project_name}'],
                    cwd=project_dir,
                    capture_output=True,
                    text=True
                )
                if commit_result.returncode == 0:
                    cprint("已创建初始提交", "GREEN")
                else:
                    cprint("注意: 未创建初始提交 (可能没有文件变更或未配置用户信息)", "YELLOW")
                    cprint("您可以稍后手动提交:", "CYAN")
                    cprint(f"  git add . && git commit -m 'Initial commit'", "CYAN")
                
                cprint("\n" + "="*50, "GREEN")
                cprint("Git仓库已重新初始化并配置完成!", "GREEN", bold=True)
                cprint("="*50, "GREEN")
                
                # 显示最终配置
                subprocess.run(['git', 'remote', '-v'], cwd=project_dir)
                
                cprint(f"\n配置说明:", "YELLOW", bold=True)
                if template_choice in ['y', 'yes']:
                    cprint(f"1. 模板远程 '{template_name}' 已设置为只读模式（仅参考）", "YELLOW")
                if project_choice in ['y', 'yes']:
                    cprint(f"2. 项目远程 'origin' 已配置为您指定的地址", "YELLOW")
                else:
                    cprint(f"2. 未配置项目远程仓库地址", "YELLOW")
                cprint(f"3. 这是一个全新的Git仓库，与模板无提交历史关联", "YELLOW")
                
                return
                
            except Exception as e:
                cprint(f"删除并重新初始化Git仓库时出错: {str(e)}", "RED")
                return
        else:
            # 不重新初始化，保留现有Git仓库（与模板有关联）
            
            # 步骤1: 清空所有现有的远程地址
            cprint("\n正在清理现有的远程配置...", "CYAN")
            
            # 获取所有远程地址
            result = subprocess.run(
                ['git', 'remote'],
                cwd=project_dir,
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                remotes = result.stdout.strip().split('\n')
                for remote in remotes:
                    if remote.strip():
                        subprocess.run(['git', 'remote', 'remove', remote.strip()], 
                                    cwd=project_dir, check=True)
                        cprint(f"  - 已删除远程: {remote.strip()}", "YELLOW")
            
            cprint("所有远程地址已清理完成", "GREEN")
            
            # 步骤2: 添加模板地址并设置为只读
            cprint(f"\n正在添加模板远程地址...", "CYAN")
            subprocess.run(['git', 'remote', 'add', template_name, template_url], 
                          cwd=project_dir, check=True)
            
            # 设置push URL为只读标记
            subprocess.run(['git', 'remote', 'set-url', '--push', template_name, 'DO_NOT_PUSH_TEMPLATE'], 
                          cwd=project_dir, check=True)
            
            cprint(f"已添加模板远程（只读）: {template_name} -> {template_url}", "GREEN")
            cprint(f"已设置模板远程为只读模式（push URL: DO_NOT_PUSH_TEMPLATE）", "GREEN")
            
            # 步骤3: 询问是否配置项目远程仓库（可选）
            cprint("\n" + "="*50, "CYAN")
            cprint("项目远程仓库配置（可选）", "BOLD")
            cprint("="*50, "CYAN")
            
            project_choice = cinput("\n是否要配置项目远程仓库地址? (y/n): ", "BOLD").strip().lower()
            
            if project_choice in ['y', 'yes']:
                # 获取用户的项目仓库地址
                default_example = template_url.replace('kk_frame', new_project_name)
                
                while True:
                    project_url = cinput(f"请输入您的项目远程仓库地址 (示例: {default_example}): ", "BOLD").strip()
                    
                    if project_url:
                        break
                    cprint("错误: 远程仓库地址不能为空", "RED")
                
                # 添加项目远程
                subprocess.run(['git', 'remote', 'add', 'origin', project_url], 
                             cwd=project_dir, check=True)
                cprint(f"已添加项目远程: origin -> {project_url}", "GREEN")
                
                # 设置安全配置
                try:
                    subprocess.run(['git', 'config', 'push.default', 'current'], 
                                cwd=project_dir, check=True)
                    subprocess.run(['git', 'config', 'remote.pushDefault', 'origin'], 
                                cwd=project_dir, check=True)
                except:
                    pass
            else:
                cprint("未配置项目远程仓库地址", "YELLOW")
                cprint("您可以稍后使用以下命令配置:", "CYAN")
                cprint(f"  git remote add origin <您的仓库地址>", "CYAN")
                # 设置更安全的默认配置
                try:
                    subprocess.run(['git', 'config', 'push.default', 'nothing'], 
                                cwd=project_dir, check=True)
                    cprint(f"已设置push.default为'nothing'，防止意外推送", "GREEN")
                except:
                    pass
        
        # 显示最终配置
        cprint("\n" + "="*50, "GREEN")
        cprint("Git远程配置完成!", "GREEN", bold=True)
        cprint("="*50, "GREEN")
        
        subprocess.run(['git', 'remote', '-v'], cwd=project_dir)
        
        cprint(f"\n配置说明:", "YELLOW", bold=True)
        cprint(f"1. 模板远程 '{template_name}' 已设置为只读模式", "YELLOW")
        
        if project_choice in ['y', 'yes']:
            cprint(f"2. 项目远程 'origin' 已配置为您指定的地址", "YELLOW")
            cprint(f"3. 默认push到 'origin' 远程", "YELLOW")
        else:
            cprint(f"2. 未配置项目远程仓库地址", "YELLOW")
            cprint(f"3. 已设置push.default为'nothing'，防止意外推送", "YELLOW")
        
        if reinit_choice not in ['y', 'yes']:
            cprint(f"4. 此仓库保留了模板的提交历史", "YELLOW")
        else:
            cprint(f"4. 这是一个全新的Git仓库", "YELLOW")
        
        cprint(f"\n常用命令:", "CYAN", bold=True)
        if project_choice in ['y', 'yes']:
            cprint(f"推送代码到项目仓库:", "CYAN")
            cprint(f"  git push origin master", "CYAN")
        
        # 只有在保留现有仓库且没有重新初始化时，才显示模板获取命令
        if reinit_choice not in ['y', 'yes']:
            cprint(f"获取模板更新:", "CYAN")
            cprint(f"  git fetch {template_name}", "CYAN")
            cprint(f"  git merge {template_name}/master", "CYAN")
        
        cprint(f"\n注意: 不要尝试推送到 '{template_name}' 远程（已设置为只读）", "RED")
        
    except subprocess.CalledProcessError as e:
        cprint(f"执行Git命令时出错: {e}", "RED")
    except Exception as e:
        cprint(f"更新Git远程配置时出错: {str(e)}", "RED")

# 执行替换操作
def perform_replacements_in_target(target_dir, project_name):
    # 切换到目标目录
    original_cwd = os.getcwd()
    
    try:
        os.chdir(target_dir)
        cprint(f"\n已切换到新目录: {os.getcwd()}", "CYAN")
        
        replace_r_h(project_name)
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
    while True:
        name = cinput("请输入项目名称（只能使用字母、数字和下划线，且不能以数字开头）: ", "BOLD").strip()
        
        if is_valid_project_name(name):
            return name
        
        cprint("错误：项目名称不合法！", "RED")
        cprint("请确保项目名称:", "RED")
        cprint("- 只包含字母（a-z, A-Z）、数字（0-9）和下划线（_）", "RED")
        cprint("- 不以数字开头", "RED")
        cprint("- 不包含空格、短横线或其他特殊字符\n", "RED")

# 主程序
def main():
    try:
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
        update_git_remote(new_project_dir, new_project_name, "kk_frame")
        
        # h. 输出成功信息
        cprint(f"\n------------------------- SUCCESS -------------------------", "GREEN", bold=True)
        cprint(f"已成功创建项目: '{new_project_name}'", "GREEN", bold=True)
        cprint(f"项目路径: '{new_project_dir}'", "GREEN", bold=True)
        cprint(f"\n后续步骤:", "CYAN")
        cprint(f"1. 返回到 out 目录", "CYAN")
        cprint(f"2. 运行 'touch ../apps/CMakeLists.txt'", "CYAN")
        cprint(f"3. 运行 'make -j3' 编译项目", "CYAN")
        cprint(f"\n", "YELLOW")
    except KeyboardInterrupt:
        # 当用户按下 Ctrl+C 时
        cprint("\n\n操作已取消，已进行的操作残留需自行清理~", "RED")
        sys.exit(0)  # 优雅地退出程序

if __name__ == "__main__":
    main()