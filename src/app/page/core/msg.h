/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 05:46:03
 * @LastEditTime: 2025-12-31 15:47:41
 * @FilePath: /kk_frame/src/app/page/core/msg.h
 * @Description: 消息类（初始化消息、运行时消息、）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __MSG_H__
#define __MSG_H__

/// @brief 消息类型
typedef enum {
    MSG_TYPE_GENERAL = 0, // 通用消息
} MSG_TYPE;

/*
 *************************************** 消息基类 ***************************************
 *
 * clone 接口用于消息的深拷贝，自动管控内存释放
 *
**/

/// @brief 消息基类
struct RunMsgBase {
    MSG_TYPE msgType;                    // 消息类型

    virtual ~RunMsgBase() = default;
    virtual RunMsgBase* clone() const {
        return new RunMsgBase(*this);
    };
};

/// @brief 初始化消息基类
struct LoadMsgBase {
    virtual ~LoadMsgBase() = default;
    virtual LoadMsgBase* clone() const = 0;
};

/// @brief 状态保存消息基类
struct SaveMsgBase {
    virtual ~SaveMsgBase() = default;
    virtual SaveMsgBase* clone() const = 0;
};


#if 0
/// @brief 通用消息(举例用)
/// @note 
struct GeneralMsg :public RunMsgBase {
    GeneralMsg() :RunMsgBase() {
        msgType = MSG_TYPE_GENERAL;
    }
    RunMsgBase* clone() const override {
        return new GeneralMsg(*this);
    }
};
#endif

#endif // !__MSG_H__
