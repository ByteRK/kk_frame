/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-29 11:59:20
 * @LastEditTime: 2025-12-29 11:59:41
 * @FilePath: /kk_frame/src/template/Singleton.h
 * @Description: 单例模板
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template<typename T>
class Singleton {
protected:
    Singleton() = default;
    ~Singleton() = default;
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    static T* instance() {
        static T inst;
        return &inst;
    }
};

#endif // !__SINGLETON_H__
