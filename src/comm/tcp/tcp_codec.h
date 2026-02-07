/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-07 11:44:32
 * @LastEditTime: 2026-02-07 14:05:27
 * @FilePath: /kk_frame/src/comm/tcp/tcp_codec.h
 * @Description: Tcp 包解码&暂存
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

// TODO

#ifndef __TCP_CODEC_H__
#define __TCP_CODEC_H__

#include <vector>
#include <stdint.h>

class TcpCodec {
private:
    std::vector<uint8_t> mBuffer;

public:
    TcpCodec() { };
    ~TcpCodec() { };
    
};


#endif // !__TCP_CODEC_H__
