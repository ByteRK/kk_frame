/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 05:46:03
 * @LastEditTime: 2026-01-19 17:36:32
 * @FilePath: /kk_frame/src/app/page/core/msg.h
 * @Description: 消息类（初始化消息、运行时消息、）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __MSG_H__
#define __MSG_H__

/*
 *************************************** 消息基类 ***************************************
 *
 * clone 接口用于消息的深拷贝，自动管控内存释放
 *
**/

/// @brief 消息基类
struct RunMsgBase {
    MSG_TYPE msgType;                    // 消息类型
    int      msgValue;                   // 消息值

    virtual ~RunMsgBase() = default;
    virtual RunMsgBase* clone() const {
        return new RunMsgBase(*this);
    };
};
template <typename T> // 偷懒用的模板
struct RunMsgBaseT :public RunMsgBase {
    RunMsgBase* clone() const override {
        const T* derived = static_cast<const T*>(this);
        return new T(*derived);
    }
};

/// @brief 初始化消息基类
struct LoadMsgBase {
    virtual ~LoadMsgBase() = default;
    virtual LoadMsgBase* clone() const = 0;
};
template <typename T> // 偷懒用的模板
struct LoadMsgBaseT :public LoadMsgBase {
    LoadMsgBase* clone() const override {
        const T* derived = static_cast<const T*>(this);
        return new T(*derived);
    }
};

/// @brief 状态保存消息基类
struct SaveMsgBase {
    virtual ~SaveMsgBase() = default;
    virtual SaveMsgBase* clone() const = 0;
};
template <typename T> // 偷懒用的模板
struct SaveMsgBaseT :public SaveMsgBase {
    SaveMsgBase* clone() const override {
        const T* derived = static_cast<const T*>(this);
        return new T(*derived);
    }
};


#if 0

/**
 * 用法比较简单
 * 在需要接收消息的窗口/弹窗类中，继承对应的消息类，并实现clone接口
 * 发送消息，则直接new一个消息类，并调用wind_mgr的send接口即可
 * 
 * 若只是发送个状态，没有特殊数据，直接调用带MSG_TYPE参数的通用消息接口
 * 
 * 若自定义消息结构中含有指针，
 * 必须要根据实际情况实现clone接口进行复制并且在析构中进行释放，
 * 否则会导致内存泄漏
 */

/// @brief 消息定义方式(举例用)
struct Demo1Msg :public RunMsgBase {
    int a;
    Demo1Msg() {
        msgType = MSG_GENERAL;
    }
    Demo1Msg* clone() const override {
        return new Demo1Msg(*this);
    }
};

/// @brief 消息定义方式(举例用)
struct Demo2Msg : public RunMsgBaseT<Demo2Msg>{
    int a;
};

#endif

#endif // !__MSG_H__
