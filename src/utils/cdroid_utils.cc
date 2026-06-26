/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 01:53:51
 * @LastEditTime: 2026-06-26 11:43:43
 * @FilePath: /kk_frame/src/utils/cdroid_utils.cc
 * @Description: Cdroid相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "cdroid_utils.h"
#include <cdinput.h>
#include <core/inputeventsource.h>

#include <gui/drawables/statelistdrawable.h>
#include <gui/drawables/levellistdrawable.h>

void CdroidUtils::sendKey(int code, bool down) {
    analogInput(code, down ? 1 : 0);
}

void CdroidUtils::analogInput(int code, int value) {
    INPUTEVENT i = { 0 };
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    i.tv_sec = ts.tv_sec;
    i.tv_usec = ts.tv_nsec / 1000;
#ifdef EV_KEY
    i.type = EV_KEY;
#else
    i.type = 0x01;
#endif
    i.code = code;
    i.value = value;
    i.device = INJECTDEV_KEY;
    InputInjectEvents(&i, 1, 1);
}

void CdroidUtils::setFilterBitmap(View * view, bool filter) {
    if (!view) return;
    ImageView* iv = dynamic_cast<ImageView*>(view);
    if (iv) setFilterBitmap(iv->getDrawable(), filter);
}

void CdroidUtils::setFilterBitmap(cdroid::Drawable* drawable, bool filter) {
    if (!drawable) return;
    cdroid::StateListDrawable* d = dynamic_cast<cdroid::StateListDrawable*>(drawable);
    if (d) {
        for (int i = 0; i < d->getStateCount(); i++)
            setFilterBitmap(d->getStateDrawable(i), filter);
    } else {
        cdroid::LevelListDrawable* l = dynamic_cast<cdroid::LevelListDrawable*>(drawable);
        if (l) {
            for (int i = 0; i < l->getChildCount(); i++)
                setFilterBitmap(l->getChild(i), filter);
        } else {
            drawable->setFilterBitmap(true);
        }
    }
}

void CdroidUtils::refreshScreenSaver() {
#if 1
    sendKey(cdroid::KeyEvent::KEYCODE_WINDOW, 1);
#else
    cdroid::InputEventSource::getInstance().closeScreenSaver();
#endif
}
