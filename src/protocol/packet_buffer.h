/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:50:42
 * @FilePath: /hana_frame/src/protocol/packet_buffer.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef __packet_buffer_h__
#define __packet_buffer_h__

#include "packet_base.h"
#include "mcu_ui.h"

/// @brief 数据包缓存基类
class IPacketBuffer {
protected:
    std::list<BuffData *> mBuffs;
    IAsk                 *mSND = nullptr;
    IAck                 *mRCV = nullptr;
public:
    virtual ~IPacketBuffer();
public:
    virtual BuffData   *obtain(bool receive = true, uint16_t dataLen = 0)  = 0;  // 创建(receive用与区分发送还是接收，dataLen用于不定长包)
    virtual void        recycle(BuffData *buf);                                  // 回收
    virtual int         add(BuffData *buf, uint8_t *in_buf, int len);            // 添加数据
    virtual bool        complete(BuffData *buf);                                 // 数据完整
    virtual bool        compare(BuffData *src, BuffData *dst);                   // 对比数据
    virtual bool        check(BuffData *buf);                                    // 校验数据
    virtual std::string str(BuffData *buf);                                      // 格式化字符串
    virtual void        checkCode(BuffData *buf);                                // 生成校验码
    virtual IAck       *ack(BuffData *bf);                                       // 转化成ack
};

/// @brief 按键数据包缓存
class BtnPacketBuffer : public IPacketBuffer {
public:
    BtnPacketBuffer();
    virtual BuffData   *obtain(bool receive, uint16_t dataLen) override;
};

/// @brief 通讯数据包缓存
class McuPacketBuffer : public IPacketBuffer {
public:
    McuPacketBuffer();
    virtual BuffData   *obtain(bool receive, uint16_t dataLen) override;
};

/// @brief 涂鸦数据包缓存
class TuyaPacketBuffer : public IPacketBuffer {
public:
    TuyaPacketBuffer();
    virtual BuffData   *obtain(bool receive, uint16_t dataLen) override;
};

#endif
