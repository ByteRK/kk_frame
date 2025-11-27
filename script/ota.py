'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-05-03 21:13:27
LastEditTime: 2025-07-10 23:07:53
FilePath: /kk_frame/script/ota.py
Description: OTA包封装脚本，用于往常规的升级包的头部添加32位MD5校验码，并生成.bin文件，适用无法从其他路径获取MD5的项目
BugList: 

Copyright (c) 2025 by Ricken, All Rights Reserved. 

'''

import hashlib
import sys
import os

def calculate_md5(data):
    """计算给定数据的 MD5 哈希值"""
    hash_md5 = hashlib.md5()
    hash_md5.update(data)
    return hash_md5.hexdigest()

def create_bin_file(original_file, output_file=None):
    """创建 .bin 文件，头部为 MD5，尾部为原有数据"""
    md5_hash = calculate_md5_file(original_file)
    
    # 如果提供了输出文件名，则使用它；否则，生成默认的 .bin 文件名
    if output_file:
        bin_file_name = output_file
    else:
        base_name, _ = os.path.splitext(original_file)
        bin_file_name = f"{base_name}.bin"
    
    with open(original_file, "rb") as original:
        original_data = original.read()
    
    with open(bin_file_name, "wb") as bin_file:
        bin_file.write(md5_hash.encode())  # 直接写入 MD5 哈希
        bin_file.write(original_data)
    
    print(f"生成文件: {bin_file_name}，MD5: {md5_hash}")

def calculate_md5_file(file_path):
    """计算文件的 MD5 哈希值"""
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def parse_bin_file(bin_file):
    """解析 .bin 文件，提取 MD5 和原始数据"""
    with open(bin_file, "rb") as f:
        md5_from_file = f.read(32).decode()  # 读取固定长度的 MD5 哈希
        original_data = f.read()  # 读取剩余的原始数据
    
    return md5_from_file, original_data

def save_original_file(original_data, output_file):
    """保存原始数据到指定文件"""
    with open(output_file, "wb") as f:
        f.write(original_data)
    print(f"原始文件已保存为: {output_file}")

def main():
    if len(sys.argv) < 3:
        print("用法: python script.py <操作> <输入文件名> [<输出文件名>]")
        print("操作: 'create' 生成 .bin 文件，'parse' 解析 .bin 文件，'extract' 提取原始文件")
        sys.exit(1)

    operation = sys.argv[1]
    input_file = sys.argv[2]
    
    if operation == 'create':
        output_file = sys.argv[3] if len(sys.argv) > 3 else None
        if not os.path.isfile(input_file):
            print(f"错误: 文件 {input_file} 不存在。")
            sys.exit(1)
        create_bin_file(input_file, output_file)
        
    elif operation == 'parse':
        if not os.path.isfile(input_file):
            print(f"错误: 文件 {input_file} 不存在。")
            sys.exit(1)
        md5_from_file, original_data = parse_bin_file(input_file)
        md5_calculated = calculate_md5(original_data)
        print(f"从文件提取的 MD5: {md5_from_file}")
        print(f"计算的 MD5: {md5_calculated}")

    elif operation == 'extract':
        if len(sys.argv) != 4:
            print("用法: python script.py extract <bin文件名> <输出文件名>")
            sys.exit(1)

        output_file = sys.argv[3]
        if not os.path.isfile(input_file):
            print(f"错误: 文件 {input_file} 不存在。")
            sys.exit(1)
        
        md5_from_file, original_data = parse_bin_file(input_file)
        save_original_file(original_data, output_file)

    else:
        print("错误: 无效的操作。请使用 'create'、'parse' 或 'extract'。")
        sys.exit(1)

if __name__ == "__main__":
    main()
