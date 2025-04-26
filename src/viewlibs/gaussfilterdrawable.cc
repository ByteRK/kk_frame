#include <gaussfilterdrawable.h>
#include <cdlog.h>
#include <random>
// gauss
#include "gaussFilter.h"
#include "gaussianblur.h"

#define USER_NENO 1
#define USE_PIXMAN 1
#ifdef USE_PIXMAN
#include <pixman.h>
#endif

GaussFilterDrawable::GaussFilterDrawable(View *fromView,Rect rect,int ksize,int scale,float colorDev, int noiseMin, int noiseMax):mGaussRadius(ksize){
    mGaussRegion.setEmpty();
    bitmapRGBData = nullptr;
    bitmapRGBGaussData = nullptr;
    mBitmapData = nullptr;

    mRadii[0] = mRadii[1] = 0;
    mRadii[2] = mRadii[3] = 0;

    mPaddingLeft    = mPaddingTop    = 0;
    mPaddingRight   = mPaddingBottom = 0;

    mColorDev       = colorDev;
    mColorNoiseMin  = noiseMin;
    mColorNoiseMax  = noiseMax;

    mScale = scale;
    mFromView = fromView;
    setGaussBitmip(mFromView,rect);

    computeBitmapGasuss();
}

GaussFilterDrawable::GaussFilterDrawable(View *fromView,Rect rect,int ksize,int scale,int maskColor, bool isGauss):mGaussRadius(ksize){
    mGaussRegion.setEmpty();
    bitmapRGBData = nullptr;
    bitmapRGBGaussData = nullptr;
    mBitmapData = nullptr;

    mRadii[0] = mRadii[1] = 0;
    mRadii[2] = mRadii[3] = 0;

    mPaddingLeft    = mPaddingTop    = 0;
    mPaddingRight   = mPaddingBottom = 0;

    mMaskColor = maskColor;

    mScale = scale;
    mFromView = fromView;
    setGaussColorNoiseRange(-2,2);
    setGaussBitmip(mFromView,rect);

    if(isGauss) computeBitmapGasuss();
}

GaussFilterDrawable::~GaussFilterDrawable(){
    
}

void GaussFilterDrawable::setGaussBitmip(Cairo::RefPtr<Cairo::ImageSurface> &bmp,Rect rect){
    if(rect.width == -1)    rect.width  = bmp->get_width();
    if(rect.height == -1)   rect.height = bmp->get_height();
    
    mBitmap = bmp;

    mBitmapData  = mBitmap->get_data();
    setGaussRegion(rect);
}

void GaussFilterDrawable::setGaussBitmip(View *fromView,Rect rect){
    if(rect.width == -1)    rect.width  = fromView->getWidth();
    if(rect.height == -1)   rect.height = fromView->getHeight();
    
    mBitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,fromView->getWidth(), fromView->getHeight());
    Canvas *mcanvas = new Canvas(mBitmap);
    fromView->draw(*mcanvas);
    delete mcanvas;

    mBitmapData  = mBitmap->get_data();
    setGaussRegion(rect);
}

void GaussFilterDrawable::setGaussRadius(int radius){
    if(mGaussRadius != radius){
        mGaussRadius = radius;
    }
}

void GaussFilterDrawable::setGaussRegion(Rect rect){
    if(mGaussRegion != rect){
        mGaussRegion = rect;
        computeBitmapSize();
        mGaussData   = mBitmapData+(mGaussRegion.top*mBitmap->get_stride()+mGaussRegion.left*4);
        mGaissBitmap = Cairo::ImageSurface::create(mGaussData,Cairo::Surface::Format::ARGB32,mGaussRegion.width, mGaussRegion.height,mBitmap->get_stride());
    }
}

void GaussFilterDrawable::setGaussColorDev(float colorDev){
    mColorDev = colorDev;
}

void GaussFilterDrawable::setGaussColorNoiseRange(int min,int max){
    mColorNoiseMin = min;
    mColorNoiseMax = max;
}

void GaussFilterDrawable::setCornerRadii(int radius){
    mRadii[0] = mRadii[1] = radius;
    mRadii[2] = mRadii[3] = radius;
    
}

void GaussFilterDrawable::setCornerRadii(int topLeftRadius,int topRightRadius,int bottomRightRadius,int bottomLeftRadius){
    mRadii[0] = topLeftRadius;
    mRadii[1] = topRightRadius;
    mRadii[2] = bottomRightRadius;
    mRadii[3] = bottomLeftRadius;
    
}

void GaussFilterDrawable::setPadding(int LeftPadding,int topPadding,int rightPadding,int bottomPadding){
    mPaddingLeft   = LeftPadding;
    mPaddingTop    = topPadding;
    mPaddingRight  = rightPadding;
    mPaddingBottom = bottomPadding;
}

void GaussFilterDrawable::computeBitmapSize(){
    if(mGaussRegion.right() > mBitmap->get_width()){
        mGaussRegion.width = mBitmap->get_width()-mGaussRegion.left;
    } 
    if(mGaussRegion.bottom() > mBitmap->get_height()){
        mGaussRegion.height = mBitmap->get_height()-mGaussRegion.top;
    } 
    mGaussWidth = std::ceil((float)mGaussRegion.width/mScale);
    mGaussHeight = std::ceil((float)mGaussRegion.height/mScale);

    mGaussWidth += mGaussWidth%4;
    mGaussRegion.width = mGaussWidth*mScale;
    LOGE("(%d, %d)mGaussWidth = %d mGaussHeight = %d",mGaussRegion.left,mGaussRegion.top,mGaussWidth,mGaussHeight);
}

void GaussFilterDrawable::computeBitmapGasuss(){

    int64_t startTime = SystemClock::uptimeMillis();
    LOGI("start Time = %lld mGaussWidth = %d mGaussHeight = %d",startTime,mGaussWidth,mGaussHeight);
    int bitmapPos,gaussPos;
    bitmapRGBData      = (unsigned char *)malloc(mGaussWidth*mGaussHeight*4);
#if defined(USER_NENO) && (USER_NENO)
    bitmapRGBGaussData = (unsigned char *)malloc(mGaussWidth*mGaussHeight*4);
#endif

#if defined(USE_PIXMAN) && (USE_PIXMAN)

    // 创建一个 Pixman 图像表面(源数据的image)
    pixman_image_t* srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(), (uint32_t *)mBitmapData, mBitmap->get_width() * 4);

    // 创建目标图像
    pixman_image_t *dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBData, mGaussWidth * 4);

    // 设置缩放和转换参数
    pixman_transform_t transform;
    pixman_transform_init_scale(&transform, pixman_double_to_fixed(mScale),pixman_double_to_fixed(mScale));

    // 执行缩放和格式转换
    pixman_image_set_filter(srcImage, PIXMAN_FILTER_NEAREST, NULL, 0);
    pixman_image_set_transform(srcImage, &transform);
    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, mGaussRegion.left/mScale,mGaussRegion.top/mScale, 0, 0, 0, 0, mGaussWidth, mGaussHeight);

    // 创建一个 Pixman 蒙版
    pixman_color_t color_t = {
        ((mMaskColor >> 16) & 0xFF) << 8,   // 16位红色分量
        ((mMaskColor >> 8) & 0xFF) << 8,    // 16位绿色分量
        ( mMaskColor & 0xFF) << 8,          // 16位蓝色分量
        ((mMaskColor >> 24) & 0xFF) << 8}; // 16位透明度分量

    pixman_image_t* maskColorImage = pixman_image_create_solid_fill(&color_t);
    // // 将 maskColorImage 设置为 dstImage 的蒙版
    pixman_image_composite(PIXMAN_OP_OVER, maskColorImage, maskColorImage, dstImage, 0, 0, 0, 0, 0, 0, mGaussWidth, mGaussHeight);

    // // 释放资源
    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);
    pixman_image_unref(maskColorImage);

    LOGI("diff Time = %lld",SystemClock::uptimeMillis()-startTime);
    
    int64_t startTime2 = SystemClock::uptimeMillis();
    //////////////////////////////////////////////////////////////////////////
#if defined(USER_NENO) && (USER_NENO)
    // 使用Neon的指令集进行模糊的计算 
    gaussianFilter_u8_Neon(bitmapRGBData, bitmapRGBGaussData, mGaussHeight , mGaussWidth, 4, mGaussRadius);
#else 
    // 使用纯算法进行模糊的计算
    GaussianBlurFilter(bitmapRGBData, mGaussWidth ,mGaussHeight, mGaussRadius);
#endif
    /////////////////////////////////////////////////////////////////////////
    LOGI("gaussianFilter_u8_Neon time = %ld USER_NENO = %d",SystemClock::uptimeMillis()-startTime2,USER_NENO);
    int64_t startTime3 = SystemClock::uptimeMillis();

    // 执行缩放的image
#if defined(USER_NENO) && (USER_NENO)
    srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBGaussData, mGaussWidth * 4);
#else
    srcImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mGaussWidth, mGaussHeight, (uint32_t *)bitmapRGBData, mGaussWidth * 4);
#endif
    dstImage = pixman_image_create_bits(PIXMAN_a8r8g8b8, mBitmap->get_width(), mBitmap->get_height(),(uint32_t *)mBitmapData, mBitmap->get_width() * 4);
    
    // 设置缩放和转换参数
    pixman_transform_init_scale(&transform, pixman_double_to_fixed(1.0f/mScale),pixman_double_to_fixed(1.0f/mScale));

    // 执行缩放和格式转换
    pixman_image_set_filter(srcImage, PIXMAN_FILTER_NEAREST, NULL, 0);
    pixman_image_set_transform(srcImage, &transform);
    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage, 0, 0, 0, 0, mGaussRegion.left,mGaussRegion.top, mGaussWidth*mScale, mGaussHeight*mScale);

    LOGI("diff Time = %ld",SystemClock::uptimeMillis()-startTime3);

    // // 释放资源
    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);

    free(bitmapRGBData);
#if defined(USER_NENO) && (USER_NENO)
    free(bitmapRGBGaussData);
#endif
    LOGI("diff Time = %ld",SystemClock::uptimeMillis()-startTime);
#else //USE_PIXMAN
    
    for(int i=0; i<mGaussHeight; i++){
        for(int j=0; j<mGaussWidth; j++){
            bitmapPos = (i*mScale+mGaussRegion.top)*mBitmap->get_width()+(j*mScale+mGaussRegion.left);
            gaussPos = i*mGaussWidth+j;
            bitmapRGBData[gaussPos*3+0] = mBitmapData[bitmapPos*4+0];
            bitmapRGBData[gaussPos*3+1] = mBitmapData[bitmapPos*4+1];
            bitmapRGBData[gaussPos*3+2] = mBitmapData[bitmapPos*4+2];
        }
    }

    int64_t startTime2 = SystemClock::uptimeMillis();
    LOGV("start Time2 = %lld",startTime2);
    LOGI("diff Time = %ld",SystemClock::uptimeMillis()-startTime);

    gaussianFilter_u8_Neon(bitmapRGBData, bitmapRGBGaussData, mGaussHeight , mGaussWidth, 3, mGaussRadius);

    LOGI("gaussianFilter_u8_Neon time = %ld",SystemClock::uptimeMillis()-startTime2);
    int64_t startTime3 = SystemClock::uptimeMillis();
    std::random_device rd;
    std::mt19937 gen(rd());
    // 定义生成随机数的范围(修改范围，从而实现不同的杂色效果，（-2，2）效果也还可以，目前颗粒感较重，水波纹基本去掉)
    std::uniform_int_distribution<int> distribution(mColorNoiseMin, mColorNoiseMax);
    // 将模糊后的数据，覆盖到原来图片中，并添加杂色
    LOGI("mColorDev = %f",mColorDev);
    for(int i=0; i<mGaussHeight; i++){
        for(int j=0; j<mGaussWidth; j++){
            bitmapPos = (i*mScale+mGaussRegion.top)*mBitmap->get_width()+(j*mScale+mGaussRegion.left);
            gaussPos = i*mGaussWidth+j;
            int colorNoise = distribution(gen);
            // *mColorDev 将模糊区域变暗
            int r = bitmapRGBGaussData[gaussPos*3+0]*mColorDev+colorNoise;
            int g = bitmapRGBGaussData[gaussPos*3+1]*mColorDev+colorNoise;
            int b = bitmapRGBGaussData[gaussPos*3+2]*mColorDev+colorNoise;

            r = r>255?255:(r<0?0:r);
            g = g>255?255:(g<0?0:g);
            b = b>255?255:(b<0?0:b);

            LOGI("gaussPos = %d   (%d,%d,%d)",gaussPos,r,g,b);

            mBitmapData[bitmapPos*4+0] = r;
            mBitmapData[bitmapPos*4+1] = g;
            mBitmapData[bitmapPos*4+2] = b;

            if((i*mScale+mGaussRegion.top+1) < mBitmap->get_height()){
                mBitmapData[(mBitmap->get_width()+bitmapPos)*4+0] = r;
                mBitmapData[(mBitmap->get_width()+bitmapPos)*4+1] = g;
                mBitmapData[(mBitmap->get_width()+bitmapPos)*4+2] = b;
            }

            if((j*mScale+mGaussRegion.left+1) < mBitmap->get_width()){
                mBitmapData[(1+bitmapPos)*4+0] = r;
                mBitmapData[(1+bitmapPos)*4+1] = g;
                mBitmapData[(1+bitmapPos)*4+2] = b;
            }

            if((i*mScale+mGaussRegion.top+1) < mBitmap->get_height() && (j*mScale+mGaussRegion.left+1) < mBitmap->get_width()){
                mBitmapData[(1+mBitmap->get_width()+bitmapPos)*4+0] = r;
                mBitmapData[(1+mBitmap->get_width()+bitmapPos)*4+1] = g;
                mBitmapData[(1+mBitmap->get_width()+bitmapPos)*4+2] = b;
            }
        }
    }
    LOGI("diff Time = %ld",SystemClock::uptimeMillis()-startTime3);
    free(bitmapRGBData);
    free(bitmapRGBGaussData);
    LOGI("diff Time = %ld",SystemClock::uptimeMillis()-startTime);
    // mGaissBitmap->write_to_png("gaussBg.png"); // 检查模糊区域是否正确
    // mBitmap->write_to_png("gaussBg.png");      // 检查模糊区域是否正确（原图+模糊区域）
#endif //USE_PIXMAN
    
}   

void GaussFilterDrawable::draw(Canvas&canvas){
    if(mBitmap == nullptr) return;
    LOGV("mGaussWidth = %d mGaussHeight = %d mGaussRadius = %d",mGaussWidth,mGaussHeight,mGaussRadius);
    int64_t startTime = SystemClock::uptimeMillis();

    // 实现四边圆角（负角度仅个人项目需求：提示框的右下角是反圆角差不多的效果，因此做了个反圆角的判断）
    if(mRadii[0]||mRadii[1]||mRadii[2]||mRadii[3]){
        const double degrees = M_PI / 180.f;
        const int width = mBounds.width;
        const int height= mBounds.height;
	    canvas.begin_new_sub_path();
        // 左上角
        if(mRadii[0] >= 0 ) canvas.arc( mRadii[0]+mPaddingLeft, mRadii[0]+mPaddingTop, mRadii[0], 180 * degrees, 270 * degrees);
        else                canvas.arc_negative( mRadii[0]+mPaddingLeft, -mRadii[0]+mPaddingTop, -mRadii[0], 0 * degrees, -90 * degrees);
        // 右上角
        if(mRadii[1] > 0)   canvas.arc( width - mRadii[1]-mPaddingRight, mRadii[1]+mPaddingTop, mRadii[1], -90 * degrees, 0 * degrees);   
        else                canvas.arc_negative( width - mRadii[1]-mPaddingRight, -mRadii[1]+mPaddingTop, -mRadii[1], -90 * degrees, -180 * degrees); 
        // 右下角
        if(mRadii[2] > 0)   canvas.arc( width - mRadii[2]-mPaddingRight, height - mRadii[2]-mPaddingBottom, mRadii[2], 0 * degrees, 90 * degrees);
        else                canvas.arc_negative( width - mRadii[2]-mPaddingRight, height - (-mRadii[2])-mPaddingBottom, -mRadii[2], 180 * degrees, 90 * degrees);
        // 左下角
        if(mRadii[3] > 0)   canvas.arc( mRadii[3]+mPaddingLeft, height - mRadii[3]-mPaddingBottom, mRadii[3], 90 * degrees, 180 * degrees);
        else                canvas.arc_negative( mRadii[3]+mPaddingLeft, height - (-mRadii[3])-mPaddingBottom, -mRadii[3], 90 * degrees, 0 * degrees);
        canvas.close_path();
        canvas.clip();
    }
    canvas.save();
    canvas.rectangle(mBounds.left,mBounds.top,mBounds.width,mBounds.height); // 限制绘画区域，加快刷新速率
    canvas.clip();
    canvas.set_operator(Cairo::Context::Operator::SOURCE);  // bitmapDrawable 的做法是判断元素是否完全不透，完全不透则设置 SOURCE 模式，加快刷新速率
    canvas.set_source(mGaissBitmap,0,0);
    canvas.get_source_for_surface()->set_filter(Cairo::SurfacePattern::Filter::NEAREST);    // 设置刷新模式为 临近插值
    canvas.paint();
    canvas.restore();
    LOGV("(%d,%d,%d,%d)  get_width = %d",mBounds.left,mBounds.top,mBounds.width,mBounds.height,mGaissBitmap->get_width());
}
