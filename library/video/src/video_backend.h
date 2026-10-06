/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 14:03:26
 * @FilePath: /kk_frame/library/video/src/video_backend.h
 * @Description: 视频后端实现钩子（内部头文件，由 CMake 保证只编译其中一个 hal 目录）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __VIDEO_BACKEND_H__
#define __VIDEO_BACKEND_H__

#include "video_player.h"

/**
 * HAL层需实现的接口：返回播放器指针以及信息
 * 通过 CMakeLists.txt 将 hal/<backend>/ 下的源码加入编译
**/

namespace video {

    /// @brief 后端输出模式
    VideoMode    backendMode();

    /// @brief 后端名称 [rkadk/ffmpeg/mp/none]
    const char*  backendName();

    /// @brief 创建后端实例，不支持时返回空指针
    VideoPlayer* backendCreate();

}

#endif
