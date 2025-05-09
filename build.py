import os
import subprocess
from datetime import datetime

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
    updated_content = content.replace('hana_frame', new_project_name)

    # 将修改后的内容写回文件
    with open(cmake_file_path, 'w', encoding='utf-8') as file:
        file.write(updated_content)

    print(f"已生成项目cmake")

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

# 获取用户输入的新项目名称
new_project_name = input("请输入项目名称: ")

# 调用函数进行替换
replace_project_name_in_cmake(new_project_name)
replace_placeholders_in_readme(new_project_name)

# 输出结果
print(f"------------------------- SUCCESS -------------------------")
print(f"已新建项目： '{new_project_name}'")
print(f"建议更改当前项目所在文件夹为新项目名称")
print(f"返回到 out 目录, 运行touch ../apps/CMakeLists.txt,再make即可")