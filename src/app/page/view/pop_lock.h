/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2026-01-04 14:06:53
 * @FilePath: /kk_frame/src/app/page/view/pop_lock.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __POP_LOCK_H__
#define __POP_LOCK_H__

#include "pop.h"

class LockPop :public PopBase {
private:
public:
    LockPop();
    ~LockPop();
    int8_t getType() const override;
protected:
    void initUI() override;
};

#endif // !__POP_LOCK_H__