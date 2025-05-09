/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:13:19
 * @FilePath: /hana_frame/src/windows/pop_lock.h
 * @Description: 童锁弹窗
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
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