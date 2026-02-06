/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 19:11:26
 * @LastEditTime: 2026-02-05 19:11:30
 * @FilePath: /kk_frame/src/common.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __COMMON_H__
#define __COMMON_H__

#define ENABLED(NAME) (defined(ENABLE_##NAME) && ENABLE_##NAME == 1)
#define DISABLED(NAME) (!defined(ENABLE_##NAME) || ENABLE_##NAME == 0)

#define CHECK_SWITCH_OFF(D) (access(#D "_0", F_OK) == 0)
#define CHECK_SWITCH_ON(D)  (access(#D "_1", F_OK) == 0)

#endif // !__COMMON_H__
