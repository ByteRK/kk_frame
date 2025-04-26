/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-02-20 02:07:27
 * @FilePath: /cy_frame/src/windows/page_standBy.h
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Cy, All Rights Reserved.
 *
 */

#ifndef _PAGE_STANDBY_H_
#define _PAGE_STANDBY_H_

#include "base.h"

class StandByPage :public PageBase {
private:
public:
    StandByPage();
    ~StandByPage();
    uint8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;
};

#endif // !_PAGE_STANDBY_H_