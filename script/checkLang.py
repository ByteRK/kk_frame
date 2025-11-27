'''
Author: Ricken
Email: me@ricken.cn
Date: 2025-08-12 07:18:24
LastEditTime: 2025-08-12 07:18:46
FilePath: /kk_frame/script/checkLang.py
Description: 检查目录下所有json文件的键及其对应的列表数量是否一致
BugList: 

Copyright (c) 2025 by Ricken, All Rights Reserved. 

'''
import json
import os

def load_json_structure(file_path):
    """加载 JSON 文件并返回其键及对应值的结构"""
    with open(file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
        return data

def compare_json_structures(directory):
    """比较指定目录下所有 JSON 文件的键及其对应的列表数量"""
    json_files = [f for f in os.listdir(directory) if f.endswith('.json')]
    all_structures = {}

    for json_file in json_files:
        file_path = os.path.join(directory, json_file)
        structure = load_json_structure(file_path)
        all_structures[json_file] = structure

    # 获取第一个文件的结构作为基准
    reference_structure = all_structures[next(iter(all_structures))]

    # 检查每个文件的结构是否与基准结构一致
    for file_name, structure in all_structures.items():
        if file_name == next(iter(all_structures)):
            continue  # 跳过基准文件
        differences = compare_structure(reference_structure, structure)
        if differences:
            print(f"{file_name} 的结构与基准不一致，具体不一致的地方：")
            for key, diff in differences.items():
                print(f"  键 '{key}': {diff}")
        else:
            print(f"{file_name} 的结构与基准一致")

def compare_structure(ref, comp, path=""):
    """比较两个 JSON 结构的键及其对应的值，返回不一致的地方"""
    differences = {}

    if ref.keys() != comp.keys():
        differences['keys'] = {'reference_keys': list(ref.keys()), 'comp_keys': list(comp.keys())}
    
    for key in ref.keys():
        ref_value = ref[key]
        comp_value = comp.get(key)

        current_path = f"{path}.{key}" if path else key

        # 检查值的类型
        if isinstance(ref_value, list) and isinstance(comp_value, list):
            if len(ref_value) != len(comp_value):
                differences[current_path] = {
                    'reference_length': len(ref_value),
                    'comp_length': len(comp_value)
                }
        elif isinstance(ref_value, dict) and isinstance(comp_value, dict):
            nested_diff = compare_structure(ref_value, comp_value, current_path)
            if nested_diff:
                differences.update(nested_diff)
        # else:
        #     if ref_value != comp_value:
        #         differences[current_path] = {'reference_value': ref_value, 'comp_value': comp_value}

    return differences

if __name__ == "__main__":
    directory = input("请输入包含 JSON 文件的目录路径: ")
    compare_json_structures(directory)
