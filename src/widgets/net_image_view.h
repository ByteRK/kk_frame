/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-09 22:37:46
 * @LastEditTime: 2026-04-10 23:30:34
 * @FilePath: /kk_frame/src/widgets/net_image_view.h
 * @Description: 网络图片组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __NET_IMAGE_VIEW_H__
#define __NET_IMAGE_VIEW_H__

#include <widget/imageview.h>

#include "http_mgr.h"

/// @brief 网络图片组件
class NetImageView : public ImageView,
    public HttpManager::IEventListener {
public:
    NetImageView(int w, int h);
    NetImageView(Context*ctx, const AttributeSet&attrs);
    ~NetImageView();

    void setCachePath(const std::string& path);
    void setImageResource(const std::string& res);
    void setDefaultImageResource(const std::string& res);

    bool isDownloading() const;
    void cancelDownload(bool reset = true);

protected:
    void init();
    void onHttpEvent(const HttpManager::Event& event) override;

private:
    bool isSupportImageType(const std::string& fileName);

private:
    HttpManager::RequestId mRequestId{ 0 };
    std::string            mDefaultImageRes{ "@null" };
    std::string            mCachePath{ "" };
};

#endif // __NET_IMAGE_VIEW_H__