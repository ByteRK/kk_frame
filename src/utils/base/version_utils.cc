/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 11:15:07
 * @LastEditTime: 2025-12-26 11:35:53
 * @FilePath: /kk_frame/src/utils/version_utils.cc
 * @Description: 版本相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "version_utils.h"
#include <sstream>

std::vector<int> VersionUtils::split(const std::string& version) {
    std::vector<int> result;
    std::stringstream ss(version);
    std::string token;
    while (std::getline(ss, token, '.'))
        result.push_back(std::stoi(token));
    return result;
}

int VersionUtils::compare(const std::string& version1, const std::string& version2) {
    std::vector<int> v1 = split(version1);
    std::vector<int> v2 = split(version2);
    int minLength = std::min(v1.size(), v2.size());
    for (int i = 0; i < minLength; ++i) {
        if (v1[i] > v2[i]) return 1;
        else if (v1[i] < v2[i]) return -1;
    }
    if (v1.size() > v2.size()) return 1;
    if (v1.size() < v2.size()) return -1;
    return 0;
}