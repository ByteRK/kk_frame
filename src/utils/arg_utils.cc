/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-29 01:23:13
 * @LastEditTime: 2026-04-22 17:43:28
 * @FilePath: /kk_frame/src/utils/arg_utils.cc
 * @Description: 命令行参数解析工具
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "arg_utils.h"
#include <string>
#include <iostream>
#include <core/cxxopts.h>

namespace ArgUtils {
    // 静态解析结果
    static ArgValues mRes;
    // 原始命令行参数
    static std::vector<std::string> mRawArgs;
    // 拼接后的完整命令行参数
    static std::string mRawString;

    const ArgValues& get() {
        return mRes;
    }

    const std::vector<std::string>& getRaw() {
        return mRawArgs;
    }

    const std::string& getRawString() {
        return mRawString;
    }

    void parse(int argc, const char* argv[]) {
        mRawArgs.clear();
        mRawArgs.reserve(argc);
        mRawString.clear();
        for (int i = 0; i < argc; ++i) {
            mRawArgs.emplace_back(argv[i]);
            if (i > 0) mRawString += ' ';
            mRawString += argv[i];
        }

        cxxopts::Options options(argv[0], "Pass in specific arguments to enable special functions.");
        options.add_options()
            ("demo", "demo mode", cxxopts::value<bool>(mRes.isDemo)->default_value("false"))
            ("v,version", "only print version", cxxopts::value<bool>(mRes.version)->default_value("false"))
            ("p,page", "show any page", cxxopts::value<int>(mRes.selectPage)->default_value("0"));
        options.allow_unrecognised_options();
        std::cout << options.help() << std::endl;
        try {
            cxxopts::ParseResult result = options.parse(argc, argv);
        }
        catch (const cxxopts::exceptions::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;
            exit(0);
        }
    }
}
