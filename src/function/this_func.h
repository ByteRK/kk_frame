/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:37:56
 * @FilePath: /hana_frame/src/function/this_func.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef _THIS_FUNC_H_
#define _THIS_FUNC_H_

#include <string>
#include <stdint.h>

// char转short
#define CHARTOSHORT(H,L) (((H) << 8) | (L)) 

// char转int
#define CHARTOINT(D1,D2,D3,D4) (((D4) << 24) | ((D3) << 16) |((D2) << 8) | (D1))

// 发送按键
#define SENDKEY(k,down) analogInput(k,down)

/// @brief 打印项目信息
/// @param name 
void printProjectInfo(const char* name);

/// @brief 打印按键映射
void printKeyMap();

/// @brief 发送按键
/// @param code 
/// @param value 
void analogInput(int code, int value);

/// @brief 刷新屏保
void refreshScreenSaver();

/// @brief 写入当前时间到文件
/// @param filename 
void writeCurrentDateTimeToFile(const std::string& filename);

/// @brief 从文件读取时间
/// @param filename 
void setDateTimeFromFile(const std::string& filename);

/// @brief 默认重启（保存当前时间）
void defaultReboot();

///  @brief 开机表情
void print_cartoon_cat();

#endif