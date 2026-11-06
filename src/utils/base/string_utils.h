/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 16:07:47
 * @LastEditTime: 2026-11-06 09:37:25
 * @FilePath: /kk_frame/src/utils/base/string_utils.h
 * @Description: 字符串工具（字节操作 / UTF-8 字符操作 / 编码转换）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include <cstddef>
#include <string>
#include <vector>

/// @brief 字符串工具：字节级操作 + UTF-8 字符级操作 + 字符集转码
/// @note 本模块按主题分区：
///       1) 字节/文本基础操作   2) 数值与布尔解析
///       3) UTF-8 字符级操作    4) 字符集转码
namespace StringUtils {

    /// @brief 是否全数字(整数、小数、负数)
    /// @param str 字符
    /// @return true 全数字
    bool isNumeric(const char* str);

    /// @brief 解析布尔字面量（忽略大小写）
    /// @param text 待解析文本
    /// @param out 解析成功时写入结果
    /// @return true 可识别；false 无法识别（out 不被修改）
    /// @note 接受 1/y/yes/true/on/enable/enabled 与
    ///       0/n/no/false/off/disable/disabled；
    ///       EnvUtils::getBool 与 SysNodeUtils::readBool 共用本函数，避免判定不一致
    bool toBool(const std::string& text, bool& out);

    /// @brief 解析整数（整串必须是合法数字，不允许前导/后续空白之外的内容）
    /// @param text 待解析文本
    /// @param out 解析成功时写入结果
    /// @return true 解析成功；false 格式非法或溢出
    /// @note 全仓唯一的“字符串 -> 整数”规则：
    ///       EnvUtils::getInt、SysNodeUtils::readInt、JsonUtils::to<int> 均转发到这里，
    ///       避免各处对溢出/尾巴字符的处理不一致
    bool toInt(const std::string& text, int& out);

    /// @brief 解析 64 位整数
    /// @param text 待解析文本
    /// @param out 解析成功时写入结果
    /// @return true 解析成功；false 格式非法或溢出
    bool toInt64(const std::string& text, int64_t& out);

    /// @brief 解析双精度浮点
    /// @param text 待解析文本
    /// @param out 解析成功时写入结果
    /// @return true 解析成功；false 格式非法或溢出
    bool toDouble(const std::string& text, double& out);

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

    /// @brief 将字符串转换为指定长度的字符串，不足部分用指定字符从尾部补齐
    /// @param str 待填充的字符串
    /// @param len 目标长度
    /// @param end 填充符
    /// @return 结果字符串
    /// @note 与 fill(const int&, int, char) 的填充方向相反，注意区分
    std::string fill(const std::string& str, int len, char end = ' ');

    /// @brief 将字符串按指定分隔符分割
    /// @param str 字符串
    /// @param delimiter 分隔符
    /// @return 分割后的字符串向量
    /// @note 连续分隔符会被合并跳过
    std::vector<std::string> split(const std::string& str, const char delimiter = ',');

    /// @brief 按多个分隔字符中的任意一个分割
    /// @param str 字符串
    /// @param delimiters 分隔字符集合，如 " \t,"
    /// @return 分割后的字符串向量
    /// @note 连续分隔符会被合并跳过
    std::vector<std::string> splitAny(const std::string& str, const char* delimiters);

    /// @brief 将字符串向量用分隔符拼接
    /// @param parts 待拼接的片段
    /// @param separator 分隔符
    /// @return 拼接结果
    /// @note 与 split 对称：join 后再 split 可还原原片段
    std::string join(const std::vector<std::string>& parts, const char* separator = ",");

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

    /// @brief 修剪字符串头部的空白字符
    /// @param str 字符串
    /// @return 修剪空白字符后的字符串
    std::string trimLeft(std::string str);

    /// @brief 修剪字符串两端的空白字符（含 \r\n\t）
    /// @param str 字符串
    /// @return 修剪空白字符后的字符串
    std::string trim(std::string str);

    /// @brief 原地修剪字符串两端的空白字符
    /// @param str 字符串
    /// @note 与 trim() 的区别：不拷贝，适合热路径
    void trimInPlace(std::string& str);

    /// @brief 判断字符串是否以指定前缀开头
    /// @param str 源字符串
    /// @param prefix 前缀
    /// @return true 以指定前缀开头
    bool startsWith(const std::string& str, const std::string& prefix);

    /// @brief 判断字符串是否以指定后缀结尾
    /// @param str 源字符串
    /// @param suffix 后缀
    /// @return true 以指定后缀结尾
    bool endsWith(const std::string& str, const std::string& suffix);

    /// @brief 判断字符串是否以指定后缀结尾（忽略大小写）
    /// @param str 源字符串
    /// @param suffix 后缀
    /// @return true 以指定后缀结尾
    bool endsWithIgnoreCase(const std::string& str, const std::string& suffix);

    /// @brief 判断两个字符串是否相等（忽略大小写）
    /// @param lhs 第一个字符串
    /// @param rhs 第二个字符串
    /// @return true 相等
    bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs);

    /// @brief 不区分大小写的字符串查找
    /// @param haystack 查找的字符串
    /// @param needle 被查找的子串(匹配对象)
    /// @return 子串在字符串中的位置，nullptr表示未找到
    const char* strcasestr(const char* haystack, const char* needle);

    /* ========================================================================
     * UTF-8 字符级操作（按"字符"而非"字节"计数）
     * @note 非法序列一律按单字节处理（不抛错、不丢字节）
    ** ===================================================================== */
    /// @brief 计算字符串中的字符数量（考虑中文字符权重）
    /// @param str 字符串
    /// @param chineseWeight 中文字符的权重（默认为1，可设为2表示一个中文字符算两个）
    /// @return 加权后的字符数量
    size_t characterCount(const char* str, int chineseWeight = 1);

    /// @brief 获取指定数量的字符（不会截断半个多字节字符）
    /// @param str 字符串
    /// @param maxChars 最大字符数
    /// @param chineseWeight 中文字符的权重（默认为1）
    /// @return 截取后的字符串
    std::string substringByChars(const char* str, size_t maxChars, int chineseWeight = 1);

    /// @brief 删除最后一个字符（支持多字节UTF-8字符）
    /// @param str 源字符串
    /// @return 删除最后一个字符后的字符串；空串或仅有一个字符时返回空串
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

    /* ========================================================================
     * 字符集转码（依赖 iconv，glibc 自带）
    ** ===================================================================== */
    /// @brief 将字符串从一种编码转换为另一种编码
    /// @param input 要转换的字符串
    /// @param fromEncoding 原始编码（如 "GBK"）
    /// @param toEncoding 目标编码（如 "UTF-8"）
    /// @return 转换后的字符串；参数非法、编码不受支持或转换失败时返回空字符串
    /// @note 转换失败时不会返回部分结果，调用方需自行区分"空输入"与"转换失败"
    std::string convert(const std::string& input, const char* fromEncoding, const char* toEncoding);

} // StringUtils

#endif // !__STRING_UTILS_H__