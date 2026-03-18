/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 01:07:04
 * @LastEditTime: 2026-03-18 02:44:11
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_base.h
 * @Description: 子键盘通用头
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_BASE_H__
#define __KEYBOARD_BASE_H__

#ifdef __VSCODE__
#include "keyboard/R.h"
#else
#include "R.h"
#endif

#include "cKeyBoard.h"
#include "lib_private_config.h"

/// @brief Lib命名空间
namespace LibRid = THIS_LIB_NAME::R::id;

/// @brief 动态类型转换宏
#ifndef __dc
#define __dc(type, obj) dynamic_cast<type*>(obj)
#endif

#endif // __KEYBOARD_BASE_H__