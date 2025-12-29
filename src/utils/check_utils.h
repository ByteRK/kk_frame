/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:52:07
 * @LastEditTime: 2025-12-29 17:17:13
 * @FilePath: /kk_frame/src/utils/check_utils.h
 * @Description: 校验相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CHECK_UTILS_H__
#define __CHECK_UTILS_H__

#include <cstdint>

namespace CheckUtils {

    /// @brief 计算校验和
    /// @tparam T 数据类型
    /// @param data 数据
    /// @param length 数据长度
    /// @return 校验和
    template <typename T>
    inline uint64_t checkSum(const T* data, uint64_t length) {
        uint64_t sum = 0;
        for (int i = 0; i < length; i++)
            sum += data[i];
        return sum;
    }

} // namespace CheckUtils

#endif // !__CHECK_UTILS_H__