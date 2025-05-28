'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-04-25 12:52:59
LastEditTime: 2025-05-28 18:14:47
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

# 替换源文件中所有文件的 'kk_frame' ID为项目名称
def replace_id_in_source_files(new_project_name, directories):
    total_files = 0
    total_replacements = 0
    
    for target_dir in directories:
        # 检查目录是否存在
        if not os.path.isdir(target_dir):
            print(f"\n目录 {target_dir} 不存在，跳过处理。")
            continue
        
        print(f"\n开始处理目录: {target_dir}")
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
                                
                                print(f"  - 已更新文件: {os.path.relpath(file_path, target_dir)}")
                                dir_replacements += 1
                        
                        dir_files += 1
                except Exception as e:
                    print(f"处理文件 {file_path} 时出错: {str(e)}")
        
        print(f"完成处理: 扫描 {dir_files} 个文件, 更新 {dir_replacements} 个文件")
        total_files += dir_files
        total_replacements += dir_replacements
    
    print(f"\n总计: 扫描 {total_files} 个文件, 更新 {total_replacements} 个文件")

# 替换 README_BASE.md
def replace_placeholders_in_readme(new_project_name):
    # 指定 README_BASE.md 文件路径
    readme_base_path = './docs/README_BASE.md'
    readme_output_path = 'README.md'

    # 检查文件是否存在
    if not os.path.isfile(readme_base_path):
        print(f"文件 {readme_base_path} 不存在。")
        return

    # 获取当前时间
    create_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # 获取上一层目录的 Git 提交 ID
    try:
        commit_id = subprocess.check_output(
            ['git', 'rev-parse', 'HEAD'],
            cwd=os.path.dirname(os.path.abspath(readme_base_path)) + '/..'
        ).strip().decode('utf-8')
    except subprocess.CalledProcessError:
        commit_id = '未知'

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

    print(f"已生成 README.md")

# 替换 CMakeLists.txt
def replace_project_name_in_cmake(new_project_name):
    # 指定 CMakeLists.txt 文件路径
    cmake_file_path = 'CMakeLists.txt'

    # 检查文件是否存在
    if not os.path.isfile(cmake_file_path):
        print(f"文件 {cmake_file_path} 不存在。")
        return

    # 读取文件内容
    with open(cmake_file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # 替换指定文本
    updated_content = content.replace('kk_frame', new_project_name)

    # 将修改后的内容写回文件
    with open(cmake_file_path, 'w', encoding='utf-8') as file:
        file.write(updated_content)

    print(f"已生成项目cmake")

# 执行替换操作
def perform_replacements_in_target(target_dir, project_name):
    # 切换到目标目录
    original_cwd = os.getcwd()
    
    try:
        os.chdir(target_dir)
        print(f"\n已切换到新目录: {os.getcwd()}")
        
        replace_project_name_in_cmake(project_name)
        replace_placeholders_in_readme(project_name)

        # 替换源文件中的项目名称（处理多个目录）
        source_dirs = [
            './src/viewlibs',
            './src/windows'
        ]
        replace_id_in_source_files(project_name, source_dirs)
    finally:
        os.chdir(original_cwd)
        print(f"已切换回原始目录: {os.getcwd()}")

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
    print(f"\n已创建项目目录: {target_dir}")
    return target_dir

# 检查目标路径
def prepare_target_directory(target_dir):
    # 检查目标路径是否存在
    if not os.path.exists(target_dir):
        return True
    # 提醒文件已存在，等待操作
    print(f"警告: 目录 '{target_dir}' 已存在！")
    choice = input("是否删除现有目录并继续? (y/n): ").strip().lower()
    
    # 判断用户操作
    if choice == 'y':
        if os.path.isdir(target_dir):
            shutil.rmtree(target_dir)
        else:
            os.remove(target_dir)
        print(f"\n已删除现有目录: {target_dir}")
        return True
    
    print("操作已取消。")
    return False

# 检查项目名称是否合法
def is_valid_project_name(name):
    # 使用正则表达式验证：只允许字母、数字和下划线
    if not re.match(r'^[a-zA-Z0-9_]+$', name):
        return False
    
    # 确保名称不为空
    if len(name.strip()) == 0:
        return False
    
    # 确保不以数字开头（可选规则，如果需要可以去掉）
    if re.match(r'^\d', name):
        return False
    
    return True

# 获取项目名称
def get_valid_project_name():
    try:
        while True:
            name = input("请输入项目名称（只能使用字母、数字和下划线，且不能以数字开头）: ").strip()
            
            if is_valid_project_name(name):
                return name
            
            print("错误：项目名称不合法！")
            print("请确保项目名称:")
            print("- 只包含字母（a-z, A-Z）、数字（0-9）和下划线（_）")
            print("- 不以数字开头")
            print("- 不包含空格、短横线或其他特殊字符\n")
    except KeyboardInterrupt:
        # 当用户按下 Ctrl+C 时
        print("\n\n操作已取消。")
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
    print("\n正在复制项目文件...")
    copy_directory_contents(current_dir, new_project_dir)
    print(f"已复制所有文件到: {new_project_dir}")
    
    # f. 切换到目标目录并执行替换操作
    perform_replacements_in_target(new_project_dir, new_project_name)
    
    # g. 输出成功信息
    print(f"\n------------------------- SUCCESS -------------------------")
    print(f"已成功创建项目: '{new_project_name}'")
    print(f"项目路径: '{new_project_dir}'")
    print(f"\n后续步骤:")
    print(f"1. 返回到 out 目录")
    print(f"2. 运行 'touch ../apps/CMakeLists.txt'")
    print(f"3. 运行 'make -j10' 编译项目")
    print(f"\n附加步骤:")
    print(f"1. 在当前git仓库基础之上上传新项目至新仓库")
    print(f"2. 后续当前模板有更新，可直接合并更新")

if __name__ == "__main__":
    main()