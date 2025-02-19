/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-02-18 19:55:07
 * @FilePath: /kk_frame/src/windows/pop_lock.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef _POP_LOCK_H_
#define _POP_LOCK_H_

#include "base.h"

class LockPop :public PopBase {
private:
public:
    LockPop();
    ~LockPop();
    uint8_t getType() const override;
protected:
    void initUI() override;
};

#endif // !_POP_LOCK_H_