#!/usr/bin/env python3
# -*- coding: utf-8 -*-
'''
Author: Ricken
Email: me@ricken.cn
Description: 署名信息编码工具（生成/查看 config/brand_info.h）
BugList:

说明：
    项目的署名与版权信息不以明文字面量形式存放在源码中，而是以异或编码的
    字节数组存放在 config/brand_info.h，由 src/utils/brand_utils.cc 在运行期
    解码后使用。因此无论 grep 源码还是 strings 二进制，都搜不到明文署名，
    派生项目若要替换必须理解解码逻辑，而非直接文本替换。

用法：
    python3 script/brand_encode.py --show
        解码并打印当前署名信息

    python3 script/brand_encode.py --set "Ricken|2026|kk_frame|https://github.com/ByteRK/kk_frame|GPL-2.0"
        按 owner|year|project|repo|license 顺序更新（可只给前几项，其余保持不变）

Copyright (c) 2026 by Ricken, All Rights Reserved.

'''

import argparse
import os
import re
import sys

# 加解码参数，需与 src/utils/brand_utils.cc 中的实现保持一致
SEED = 0x5A
STEP = 37

# 字段顺序，与 brand_utils.cc 中的下标一致
FIELDS = ('owner', 'year', 'project', 'repo', 'license')

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
HEADER_PATH = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir, 'config', 'brand_info.h'))

ARRAY_RE = re.compile(r'BRAND_PAYLOAD\[\]\s*=\s*\{(.*?)\};', re.S)
BYTE_RE = re.compile(r'0[xX]([0-9a-fA-F]{1,2})')

HEADER_TEMPLATE = '''/*
 * @Description: 署名及版权信息（异或编码存储，请勿直接编辑字面量）
 * @BugList:
 *
 * 本文件由 script/brand_encode.py 生成，如需修改请使用：
 *     查看：python3 script/brand_encode.py --show
 *     修改：python3 script/brand_encode.py --set "owner|year|project|repo|license"
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BRAND_INFO_H__
#define __BRAND_INFO_H__

#include <stdint.h>

/**
 * 署名信息负载
 * 异或编码，密钥随下标变化；以 '\\0' 分隔字段
 * 字段顺序：{fields}
**/
#define BRAND_PAYLOAD_SEED {seed}
static const uint8_t BRAND_PAYLOAD[] = {{
{bytes}
}};

#endif // __BRAND_INFO_H__
'''


def xform(data):
    '''异或变换，编码与解码为同一运算'''
    return bytes((b ^ ((SEED + i * STEP) & 0xFF)) & 0xFF for i, b in enumerate(data))


def decode_payload(raw):
    '''将字节数组切分为字段列表'''
    values = raw.split(b'\0')
    while values and values[-1] == b'':
        values.pop()
    return [v.decode('utf-8', 'replace') for v in values]


def read_header():
    '''读取现有头文件中的字段，不存在时返回空列表'''
    if not os.path.exists(HEADER_PATH):
        return []
    with open(HEADER_PATH, 'r', encoding='utf-8') as fp:
        text = fp.read()
    matched = ARRAY_RE.search(text)
    if not matched:
        return []
    raw = bytes(int(h, 16) for h in BYTE_RE.findall(matched.group(1)))
    return decode_payload(xform(raw))


def write_header(values):
    '''按字段列表生成头文件'''
    plain = b'\0'.join(v.encode('utf-8') for v in values) + b'\0'
    data = xform(plain)

    rows = []
    for i in range(0, len(data), 12):
        rows.append('    ' + ' '.join('0x%02X,' % b for b in data[i:i + 12]))

    content = HEADER_TEMPLATE.format(
        fields=' / '.join(FIELDS),
        seed='0x%02X' % SEED,
        bytes='\n'.join(rows),
    )
    with open(HEADER_PATH, 'w', encoding='utf-8') as fp:
        fp.write(content)


def do_show():
    '''打印当前署名信息'''
    current = read_header()
    if not current:
        print('错误：未能从 %s 解析署名信息' % HEADER_PATH, file=sys.stderr)
        return 1
    for index, name in enumerate(FIELDS):
        print('%-8s: %s' % (name, current[index] if index < len(current) else ''))
    return 0


def do_set(raw):
    '''按 owner|year|project|repo|license 更新署名信息'''
    args = [v.strip() for v in raw.split('|')]
    if not args[0]:
        print('错误：owner 不能为空', file=sys.stderr)
        return 1

    current = read_header()
    values = list(current) + [''] * (len(FIELDS) - len(current))
    for index, value in enumerate(args[:len(FIELDS)]):
        values[index] = value

    write_header(values)
    print('已更新 %s' % HEADER_PATH)
    return do_show()


def main():
    parser = argparse.ArgumentParser(description='署名信息编码工具')
    parser.add_argument('--show', action='store_true', help='查看当前署名信息')
    parser.add_argument('--set', metavar='VALUES', help='按 owner|year|project|repo|license 更新')
    opts = parser.parse_args()

    if opts.set:
        return do_set(opts.set)
    return do_show()


if __name__ == '__main__':
    sys.exit(main())
