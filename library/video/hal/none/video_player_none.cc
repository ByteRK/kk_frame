/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-09-30 17:51:13
 * @FilePath: /kk_frame/library/video/hal/none/video_player_none.cc
 * @Description: 视频占位后端（ENABLED_VIDEO 关闭时使用）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_backend.h"

namespace video {

    /**
     * @brief 占位后端
     *
     * 仅在 ENABLED_VIDEO 关闭时编译进来：模块本身不提供任何播放能力，
     * VideoPlayer::isSupported() 因此返回 false，组件层据此显示「视频未支持」。
     */
    VideoMode backendMode() {
        return VM_NONE;
    }

    const char* backendName() {
        return "none";
    }

    VideoPlayer* backendCreate() {
        return nullptr;
    }

} // namespace video
