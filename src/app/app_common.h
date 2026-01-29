/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-01-29 10:54:19
 * @FilePath: /kk_frame/src/app/app_common.h
 * @Description: 项目通用头文件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

/**
 * 定义系统相关的数据类型
 * size_t
**/
#include <sys/types.h>

/**
 * 提供精确宽度的整数类型
 * uintxx_t
**/
#include <stdint.h>

/**
 * 标准输入/输出操作
**/
#include <stdio.h>

/**
 * 字符串和内存操作
**/
#include <string.h>

/**
 * 函数对象和操作封装
**/
#include <functional>

/**
 * C++ 标准输入/输出流
**/
#include <iostream>

/**
 * 动态数组容器
**/
#include <vector>

/**
 * 双向链表容器
**/
#include <list>

/**
 * 关联容器（键值对集合）
**/
#include <map>

/**
 * 通用算法操作
 * std::find() std::sort() std::reverse() std::unique() std::remove() std::copy()
**/
#include <algorithm>

/**
 * cdroid日志库
**/
#include <cdlog.h>


/// @brief 释放内存
/// @tparam T 
/// @param p 
template<typename T>
inline void __delete(T*& p) {
    if (p) { delete p; p = nullptr; }
}

/// @brief 动态转换
/// @tparam T 
/// @param p 
/// @return 
template<typename T>
inline T* __dc(void* p) {
    return dynamic_cast<T*>(p);
}

#endif // __APP_COMMON_H__

