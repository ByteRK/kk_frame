/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-12-29 11:36:01
 * @FilePath: /kk_frame/src/app/page/view/pop_lock.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef __POP_LOCK_H__
#define __POP_LOCK_H__

#include "base.h"

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