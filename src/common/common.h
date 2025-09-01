/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:49:00
 * @FilePath: /hana_frame/src/common/common.h
 * @Description: 全局引用的头文件
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */
#ifndef __common_h__
#define __common_h__

/**
 * 定义系统相关的数据类型
 * size_t
 */
#include <sys/types.h>

/**
 * 提供精确宽度的整数类型
 * uintxx_t
 */
#include <stdint.h>

/**
 * 标准输入/输出操作
 */
#include <stdio.h>

/**
 * 字符串和内存操作
 */
#include <string.h>

/**
 * 函数对象和操作封装
 */
#include <functional>

/**
 * C++ 标准输入/输出流
 */
#include <iostream>

/**
 * 动态数组容器
 */
#include <vector>

/**
 * 双向链表容器
 */
#include <list>

/**
 * 关联容器（键值对集合）
 */
#include <map>

/**
 * 通用算法操作
 * std::find() std::sort() std::reverse() std::unique() std::remove() std::copy()
 */
#include <algorithm>

/**
 * cdroid日志库
 */
#include <cdlog.h>

#endif

