/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:44:10
 * @LastEditTime: 2026-07-06 00:54:10
 * @FilePath: /kk_frame/src/comm/packet/packet_base.h
 * @Description: 通讯数据包基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_BASE_H__
#define __PACKET_BASE_H__

#include "encoding_utils.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#pragma pack(1)
/// @brief 收发数据包
typedef struct {
    int16_t  type;    // 包类型
    uint16_t len;     // 数据长度
    uint16_t slen;    // 数据区容量
    uint8_t  buf[1];  // 尾随数据区起始数组
} BuffData;
#pragma pack()

/// @brief 发包编码器
class IAsk {
protected:
    BuffData* mBuf{ nullptr }; // 数据源

public:
    virtual ~IAsk();
    virtual void parse(BuffData* buf);
    virtual void setData(size_t pos, uint8_t data);
    virtual void setData(const uint8_t* data, size_t pos, size_t len);
    virtual void checkCode() = 0; // 校验位设定
};

/// @brief 收包解码器
class IAck {
public:
    using ByteOrder = EncodingUtils::ByteOrder;

protected:
    uint8_t*  mBuf{ nullptr };     // 接收缓存数据区首地址
    uint16_t  mDataLen{ 0 };       // 当前完整协议包总长度（含校验字节）

private:
    BuffData* mPacket{ nullptr };  // 当前绑定的接收缓存

public:
    virtual ~IAck() { }
    virtual int    getType() = 0;  // 业务命令类型

public: // Decoder 调用
    void           parse(BuffData* buf);
    int            add(const uint8_t* data, int len);
    bool           complete();
    virtual bool   check() = 0;    // 数据包校验

public: // 应用层 调用
    const uint8_t* data() const;
    uint16_t       dataLength() const;
    bool           hasRange(size_t offset, size_t length) const;
    bool           readU8(size_t offset, uint8_t& value) const;
    bool           readU16(size_t offset, uint16_t& value, ByteOrder order = ByteOrder::BigEndian) const;
    bool           readU32(size_t offset, uint32_t& value, ByteOrder order = ByteOrder::BigEndian) const;
    bool           readU64(size_t offset, uint64_t& value, ByteOrder order = ByteOrder::BigEndian) const;
    bool           readBytes(size_t offset, size_t length, const uint8_t*& bytes) const;

protected:
    virtual const uint8_t* head() const = 0;            // 协议包头
    virtual uint16_t       headLength() const = 0;      // 协议包头长度
    virtual uint16_t       lengthReadySize() const = 0; // 读取完整包长所需的最小缓存长度
    virtual int32_t        expectedLength() const = 0;  // 完整包长

private:
    bool hasHead() const;
    bool alignHead();
    void discardPrefix(uint16_t length);
    void copyInput(const uint8_t* data, int dataLen, int& consumed, uint16_t targetLen);
    void updateDataLength();
};

#endif // !__PACKET_BASE_H__
