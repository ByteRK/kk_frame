/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 14:03:10
 * @FilePath: /kk_frame/library/video/src/video_player.cc
 * @Description: 视频播放接口的工厂实现（具体后端由 CMake 选择）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_backend.h"
#include <cdlog.h>

namespace video {

    /// @brief 创建播放器
    /// @return 返回后端实例，若当前平台不支持则返回空指针
    VideoPlayer* VideoPlayer::create() {
        VideoPlayer* player = backendCreate();
        if (player == nullptr) {
            LOGW("video backend is [%s], current platform is not supported", backendName());
        } else {
            LOGI("video backend is [%s], mode=%d", backendName(), backendMode());
        }
        return player;
    }

    /// @brief 检查是否支持视频播放
    /// @return true 支持;false 不支持
    bool VideoPlayer::isSupported() {
        return backendMode() != VM_NONE;
    }

    /// @brief 获取视频播放模式
    /// @return 视频播放模式
    VideoMode VideoPlayer::getMode() {
        return backendMode();
    }

    /// @brief 获取后端名称
    /// @return 后端名称 [rkadk/ffmpeg/mp/none]
    const char* VideoPlayer::getBackend() {
        return backendName();
    }

}
