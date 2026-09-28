/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-28 10:00:00
 * @LastEditTime: 2026-09-28 10:00:00
 * @FilePath: /kk_frame/src/utils/brand_utils.cc
 * @Description: 署名及版权信息（数据源见 config/brand_info.h）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "brand_utils.h"
#include "brand_info.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace {

    /// @brief 负载字段数量，与 config/brand_info.h 中的写入顺序一致
    const size_t BRAND_FIELD_COUNT = 5;

    /// @brief 异或解码（密钥随下标变化，与 script/brand_encode.py 保持一致）
    inline uint8_t brandDecode(size_t index, uint8_t value) {
        const uint8_t key = static_cast<uint8_t>(BRAND_PAYLOAD_SEED + index * 37u);
        return static_cast<uint8_t>(value ^ key);
    }

    /// @brief 解码并按 '\0' 切分负载，仅首次调用时执行
    const std::vector<std::string>& brandFields() {
        static std::vector<std::string> cache;
        if (cache.empty()) {
            std::string field;
            for (size_t i = 0; i < sizeof(BRAND_PAYLOAD); ++i) {
                const char ch = static_cast<char>(brandDecode(i, BRAND_PAYLOAD[i]));
                if (ch == '\0') {
                    cache.push_back(field);
                    field.clear();
                } else {
                    field.push_back(ch);
                }
            }
            if (!field.empty()) cache.push_back(field);
            // 负载缺失时补齐，避免访问越界
            while (cache.size() < BRAND_FIELD_COUNT) cache.push_back(std::string());
        }
        return cache;
    }

    /// @brief 按下标读取字段
    inline const std::string& brandField(size_t index) {
        static const std::string empty;
        const std::vector<std::string>& fields = brandFields();
        return index < fields.size() ? fields[index] : empty;
    }

} // namespace

const std::string& BrandUtils::owner() { return brandField(0); }
const std::string& BrandUtils::year() { return brandField(1); }
const std::string& BrandUtils::project() { return brandField(2); }
const std::string& BrandUtils::repo() { return brandField(3); }
const std::string& BrandUtils::license() { return brandField(4); }

std::string BrandUtils::copyrightLine() {
    return "Copyright (c) " + year() + " by " + owner() + ", All Rights Reserved.";
}

std::string BrandUtils::basedOnLine() {
    return "Project Based On " + project() + " [" + repo() + "]";
}

std::string BrandUtils::licenseLine() {
    return "Licensed under " + license() + ", see LICENSE for details.";
}

std::string BrandUtils::notice() {
    return copyrightLine() + "\n" + basedOnLine() + "\n" + licenseLine();
}
