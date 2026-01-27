/*
 * @Author: xialc
 * @Email: 
 * @Date: 2026-01-27 11:27:25
 * @LastEditTime: 2026-01-27 13:50:17
 * @FilePath: /kk_frame/src/widgets/gauss_view.h
 * @Description: 模糊视图
 * @BugList: 
 * 
 * Copyright (c) 2026 by xialc, All Rights Reserved. 
 * 
**/

#if defined(ENABLE_GAUSS_VIEW) || defined(__VSCODE__)

#include <view/view.h>

class GaussViewPrivate;
class GaussView : public View {
protected:
    void initGaussViewData();

public:
    GaussView(Context *ctx, const AttributeSet&attrs);
    GaussView(int w,int h);
    ~GaussView();

    void setRadius(int radius);
    void setOnceDraw(bool flag);
    void setColor(uint color);
    void setFilletRadius(int r);
    void setTopLeftRadius(int r);
    void setTopRightRadius(int r);
    void setBottomLeftRadius(int r);
    void setBottomRightRadius(int r);

protected:
    void draw(Canvas &ctx) override;
    void rotatePos(int cw, int ch, int w, int h, int &x, int &y);

private:
    int      mRadius;
    bool     mOnceDraw;
    int      mTopLeftRadius;
    int      mTopRightRadius;
    int      mBottomLeftRadius;
    int      mBottomRightRadius;
    uint     mColor;
    std::shared_ptr<GaussViewPrivate> d_ptr;
};

#endif
