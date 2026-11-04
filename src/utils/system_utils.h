/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 14:40:21
 * @LastEditTime: 2026-11-04 10:17:52
 * @FilePath: /kk_frame/src/utils/system_utils.h
 * @Description: 系统相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __SYSTEM_UTILS_H__
#define __SYSTEM_UTILS_H__

#include <string>
#include <cstdint>

namespace SystemUtils {

    /// @brief 分区容量信息
    struct DiskUsage {
        bool     valid{ false };  // 是否获取成功
        uint64_t total{ 0 };      // 总容量（字节）
        uint64_t available{ 0 };  // 可用容量（字节）
    };

    /// @brief 数据同步
    /// @note 将内存数据同步到储存
    void sync();

    /// @brief 结束程序
    void exit();

    /// @brief 重启
    void reboot();

    /// @brief 执行系统命令并获取结果
    /// @param cmd 命令
    /// @return 执行结果
    std::string system(const std::string& cmd);

    /// @brief 异步执行系统命令
    /// @param cmd 命令
    /// @return true 成功
    bool asyncSystem(const std::string& cmd);

    /// @brief 设置系统亮度
    /// @param value 亮度值 [0-100]
    /// @param swap 是否反转亮度值
    void setBrightness(int value, bool swap = false);

    /// @brief 设置系统音量
    /// @param value 音量值 [0-100]
    void setVolume(int value);

    /// @brief 设置系统时间
    /// @param timestamp 秒级时间戳
    void setTime(const int64_t& timestamp);

    /// @brief 设置系统时间，参数小于 0 表示保持当前值
    /// @param year 年，大于 0 时生效
    /// @param month 月，1-12 时生效
    /// @param day 日，大于 0 时生效
    /// @param hour 时，0-23 时生效
    /// @param minute 分，0-59 时生效
    /// @param second 秒，0-59 时生效 
    void setTime(int year, int month, int day, int hour, int minute, int second = 0);

    /// @brief 同步硬件时钟
    void syncHWClock();

    /// @brief 恢复系统时间
    void recoverTime();

    /// @brief 清除时间缓存
    void clearTimeCache();

    /// @brief 写入值到系统路径
    /// @param path 路径
    /// @param value 值
    /// @note 可替代echo，节约消耗
    bool sysfs(const std::string& path, const std::string& value);

    /// @brief 读取系统路径中的整数值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的整数，失败返回 def
    /// @note 可替代cat，节约消耗
    int readSysfsInt(const std::string& path, int def = 0);

    /// @brief 读取系统路径中的 64 位整数值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的整数，失败返回 def
    /// @note 可替代cat，节约消耗
    int64_t readSysfsInt64(const std::string& path, int64_t def = 0);

    /// @brief 读取系统路径中的浮点值
    /// @param path 路径
    /// @param def 读取失败时的默认值
    /// @return 读取到的浮点数，失败返回 def
    /// @note 可替代cat，节约消耗
    double readSysfsDouble(const std::string& path, double def = 0.0);

    /// @brief 读取系统路径中的字符串
    /// @param path 路径
    /// @param def 读取失败或内容为空时的默认值
    /// @return 读取到的字符串（已去除首尾空白与换行），失败返回 def
    /// @note 可替代cat，节约消耗
    std::string readSysfsString(const std::string& path, const std::string& def = "");

    /// @brief 读取系统路径中的布尔值
    /// @param path 路径
    /// @param def 读取失败或无法识别时的默认值
    /// @return 1/Y/ON/TRUE/ENABLE 等视为 true，0/N/OFF/FALSE 等视为 false，无法识别返回 def
    /// @note 可替代cat，节约消耗
    bool readSysfsBool(const std::string& path, bool def = false);

    /// @brief 读取文本文件完整内容（适用于 /proc、/sys 等虚拟文件）
    /// @param path 路径
    /// @param content 读取结果，已去除首尾空白与换行
    /// @return true 成功
    bool readTextFile(const std::string& path, std::string& content);

    /// @brief 获取指定路径所在分区的容量信息
    /// @param path 路径
    /// @return 容量信息，获取失败时 valid 为 false
    DiskUsage getDiskUsage(const std::string& path);

    /// @brief 创建跨线程唤醒使用的事件 FD（eventfd + NONBLOCK + CLOEXEC）
    /// @return 文件描述符，失败返回 -1
    int createEventFd();

    /// @brief 向事件 FD 写入一次通知
    /// @param fd 事件 FD
    /// @return true 成功（计数器已满也视为成功）
    bool notifyEventFd(int fd);

    /// @brief 清空事件 FD 中所有待处理通知
    /// @param fd 事件 FD
    /// @return true 成功（无待处理数据也视为成功）
    bool drainEventFd(int fd);

    /// @brief 关闭事件 FD 并将句柄复位为 -1
    /// @param fd 事件 FD 引用，关闭后置为 -1
    /// @return true 已处理（句柄原本无效也视为成功）
    bool destroyEventFd(int& fd);

    /// @brief 构造带 errno 描述的错误信息
    /// @param prefix 前缀描述
    /// @param errorCode 错误码
    /// @return 例如 "read failed, errno=11, Resource temporarily unavailable"
    std::string errnoMessage(const char* prefix, int errorCode);

} // SystemUtils

#endif // !__SYSTEM_UTILS_H__
