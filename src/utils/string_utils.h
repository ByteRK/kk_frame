/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 16:07:47
 * @LastEditTime: 2026-02-11 00:46:46
 * @FilePath: /X5000/src/utils/string_utils.h
 * @Description: 字符串相关的一些操作函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include <string>
#include <vector>

namespace StringUtils {

    /// @brief 是否全数字(整数、小数、负数)
    /// @param str 字符
    /// @return true 全数字
    bool isNumric(const char* str);

    /// @brief 转换为全大写
    /// @param str 
    /// @return 
    void upper(std::string& str);

    /// @brief 转换为全大写
    /// @param str 字符串
    /// @return 大写结果
    std::string upperNew(const std::string& str);

    /// @brief 转换为全小写
    /// @param str 
    /// @return 
    void lower(std::string& str);

    /// @brief 转换为全小写
    /// @param str 字符串
    /// @return 小写结果
    std::string lowerNew(const std::string& str);

    /// @brief 格式化字符串
    /// @param format 格式化字符串
    /// @param ... 可变参数
    /// @return 格式化后的字符串
    std::string format(const char* format, ...);

    /// @brief 将数值转换为指定长度的字符串，不足部分用前缀填充
    /// @param num 待填充的数值
    /// @param len 目标长度
    /// @param pre 填充符
    /// @return 结果字符串
    std::string fill(const int& num, int len, char pre = '0');

    /// @brief 将字符串转换为指定长度的字符串，不足部分用前缀填充
    /// @param str 待填充的字符串
    /// @param len 目标长度
    /// @param pre 填充符
    /// @return 结果字符串
    std::string fill(const std::string& str, int len, char end = ' ');

    /// @brief 将字符串按指定分隔符分割
    /// @param str 字符串
    /// @param delimiter 分隔符
    /// @return 分割后的字符串向量
    std::vector<std::string> split(const std::string& str, const char delimiter = ',');

    /// @brief 替换字符串中的子串
    /// @param src 原字符串
    /// @param oldStr 被替换的子串
    /// @param newStr 替换后的子串
    /// @return 替换后的字符串
    std::string replace(const std::string& src, const std::string& oldStr, const std::string& newStr);

    /// @brief 移除字符串中的指定字符
    /// @param str 原字符串
    /// @param remove 被移除的字符(默认移除反斜杠)
    /// @return 移除指定字符后的字符串
    std::string remove(std::string str, const char remove = '\\');

    /// @brief 修剪字符串尾部的空白字符
    /// @param str 字符串
    /// @return 修剪空白字符后的字符串
    std::string trimRight(std::string str);

    /// @brief 不区分大小写的字符串查找
    /// @param haystack 查找的字符串
    /// @param needle 被查找的子串(匹配对象)
    /// @return 子串在字符串中的位置，nullptr表示未找到
    const char* strcasestr(const char* haystack, const char* needle);

    /// @brief 打印字节数组，以16进制形式输出
    /// @param label 标签
    /// @param buf 字节数组
    /// @param len 字节数组长度
    /// @param width 每行显示的字节数(小于1不换行)
    void hexdump(const char* label, const unsigned char* buf, int len, int width = 30);

    /// @brief 将字节数组转换为16进制字符串
    /// @param buf 字节数组
    /// @param len 字节数组长度
    /// @param width 每行显示的字节数(小于1不换行)
    /// @return 16进制字符串
    std::string hexStr(const unsigned char* buf, int len, int width = 30);

    /// @brief 计算字符串中的字符数量（考虑中文字符权重）
    /// @param str 字符串
    /// @param chineseWeight 中文字符的权重（默认为1，可设为2表示一个中文字符算两个）
    /// @return 加权后的字符数量
    size_t characterCount(const char* str, int chineseWeight = 1);

    /// @brief 获取指定数量的字符
    /// @param str 字符串
    /// @param maxChars 最大字符数
    /// @param chineseWeight 中文字符的权重（默认为1）
    /// @return 截取后的字符串
    std::string substringByChars(const char* str, size_t maxChars, int chineseWeight = 1);

    /// @brief 删除最后一个字符（支持多字节UTF-8字符）
    /// @param str 源字符串
    /// @return 删除最后一个字符后的字符串
    std::string removeLastCharacter(const char* str);

    /// @brief 删除最后一个字符（原地修改字符串版本）
    /// @param str 字符串
    void popLastCharacter(std::string& str);

    /// @brief 保留指定长度的字符，可附加后缀
    /// @param str 源字符串
    /// @param maxChars 最大字符数
    /// @param chineseWeight 中文字符的权重
    /// @param suffix 超过限制时添加的后缀（如"..."）
    /// @return 处理后的字符串
    std::string truncateWithLimit(const char* str, size_t maxChars, 
                             int chineseWeight = 1, const std::string& suffix = "");

} // StringUtils

#endif // !__STRING_UTILS_H__