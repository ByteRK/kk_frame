/*
 * @Author: cy
 * @Email: patrickcchan@163.com
 * @Date: 2025-11-25 17:24:28
 * @LastEditTime: 2026-01-27 09:09:10
 * @FilePath: /kk_frame/library/gaussFilter/gaussFilter.c
 * @Description: 高斯滤波
 * @BugList: 
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
*/


#include <printf.h>
#include <stdint.h>
#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "cdlog.h"
#include "gaussFilter.h"
#if defined(PRODUCT_X64) || defined(__VSCODE__)
#include "NEON_2_SSE.h"
#else
#include "arm_neon.h"
#endif

#define MAX_KERNEL_SIZE   79
#define XA_U8_SIZE        16
#define XA_F32_SIZE       4
#define PI 3.14159
#define NORM_SHIFT        8

#define OMP_THREADS       2

#if ENABLE_OMP
#include "omp.h"
#endif

static unsigned int getGaussianInt(uint16_t *kernel, int ksize);

static void verticalFilterNeonU8(U8* src, U8* dst, int height, int width, int channel, uint16_t* kernel, int ksize);

static void horizonFilterNeonU8(U8* src, U8* dst, int height, int width, int channel, uint16_t* kernel, int ksize);

static unsigned int getGaussianInt(uint16_t *kernel, int ksize)
{
    if(kernel == NULL){
        return 0;
    }
    int s = 0;

    for(int i = 0; i < ksize / 2 + 1; i++){
        kernel[i] = 2 * (i + 1) - 1;
        s += kernel[i];
    }
    for(int i = ksize / 2 + 1; i < ksize; i++){
        kernel[i] = kernel[ksize - i - 1];
        s += kernel[i];
    }
    return s;
}

static void verticalFilterNeonU8(U8* src, U8* dst, int height, int width, int channel, uint16_t* kernel, int ksize)
{
    int kCenter = ksize / 2;

    unsigned char* in = (unsigned char*) calloc(sizeof(unsigned char), width * (height + ksize) * channel);
    memcpy(in, src, kCenter * width * channel);
    memcpy(in + kCenter * width * channel, src, height * width * channel);
    memcpy(in + (kCenter + height) * width * channel, in + height * width * channel, kCenter * width * channel);

    if(channel == 1){
        for(int i = 0; i < height; i++){
            unsigned char*p_dst = dst + i * width * channel;
            int n = width / XA_U8_SIZE;
            int end = n * (XA_U8_SIZE * channel);
            int count = 0;

            for(int j = 0; j < end; j += XA_U8_SIZE){
                uint16x8_t laccum_u16, haccum_u16;
                laccum_u16 = vmovq_n_u16(0);
                haccum_u16 = vmovq_n_u16(0);

                for(int k = 0; k < ksize; k++){
                    uint8x16_t data = vld1q_u8(in + (i + k) * width * channel + j);
                    uint8x8_t lp_u8 = vget_low_u8( data );
                    uint8x8_t hp_u8 = vget_high_u8( data );

                    uint16x8_t lp_u16 = vmovl_u8( lp_u8 );
                    uint16x8_t hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16 = vmlaq_n_u16(laccum_u16, lp_u16, kernel[k]);
                    haccum_u16 = vmlaq_n_u16(haccum_u16, hp_u16, kernel[k]);
                }
                laccum_u16 = vshrq_n_u16(laccum_u16, NORM_SHIFT);
                haccum_u16 = vshrq_n_u16(haccum_u16, NORM_SHIFT);
                uint8x8_t laccum_u8 = vmovn_u16( laccum_u16 );
                uint8x8_t haccum_u8 = vmovn_u16( haccum_u16 );
                uint8x16_t accum_u8 = vcombine_u8( laccum_u8, haccum_u8 );
                vst1q_u8(p_dst + j, accum_u8);
                count += 16;
            }

            for(int j = count; j < width; j++){
                U8 s = 0;
                for(int k = 0; k < ksize; k++){
                    s += (in[(i + k) * width * channel + j * channel] * kernel[k]) >> NORM_SHIFT;
                }
                if(s < 0)
                    s = 0;
                if(s > 255)
                    s = 255;
                p_dst[j * channel] = (U8)s;
            }
        }
    }
    else if(channel == 4)
    {
#if ENABLE_OMP
    omp_set_num_threads(OMP_THREADS);
    #pragma omp parallel
    {
        #pragma omp for
#endif
        //printf(" here is thread %d\n",omp_get_thread_num());
        
        for(int i = 0; i < height; i++){
            
            int n = width / XA_U8_SIZE;
            int end = n * (XA_U8_SIZE * channel);
            unsigned char*p_dst = dst + i * width * channel;
            int count = 0;

            for(int j = 0; j < end; j+= XA_U8_SIZE * channel){
                uint16x8_t laccum_u16_r = vmovq_n_u16(0);
                uint16x8_t haccum_u16_r = vmovq_n_u16(0);

                uint16x8_t laccum_u16_g = vmovq_n_u16(0);
                uint16x8_t haccum_u16_g = vmovq_n_u16(0);

                uint16x8_t laccum_u16_b = vmovq_n_u16(0);
                uint16x8_t haccum_u16_b = vmovq_n_u16(0);

                uint8x8_t lp_u8;
                uint8x8_t hp_u8;
                uint16x8_t lp_u16;
                uint16x8_t hp_u16;

                for(int k = 0; k < ksize; k++){
                    uint8x16x4_t data = vld4q_u8(in + (i + k) * width * channel + j);

                    // r
                    lp_u8 = vget_low_u8( data.val[0]);
                    hp_u8 = vget_high_u8( data.val[0]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_r = vmlaq_n_u16(laccum_u16_r, lp_u16, kernel[k]);
                    haccum_u16_r = vmlaq_n_u16(haccum_u16_r, hp_u16, kernel[k]);

                    // g
                    lp_u8 = vget_low_u8( data.val[1]);
                    hp_u8 = vget_high_u8( data.val[1]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_g = vmlaq_n_u16(laccum_u16_g, lp_u16, kernel[k]);
                    haccum_u16_g = vmlaq_n_u16(haccum_u16_g, hp_u16, kernel[k]);

                    // b
                    lp_u8 = vget_low_u8( data.val[2]);
                    hp_u8 = vget_high_u8( data.val[2]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_b = vmlaq_n_u16(laccum_u16_b, lp_u16, kernel[k]);
                    haccum_u16_b = vmlaq_n_u16(haccum_u16_b, hp_u16, kernel[k]);
                }

                laccum_u16_r = vshrq_n_u16(laccum_u16_r, NORM_SHIFT);
                haccum_u16_r = vshrq_n_u16(haccum_u16_r, NORM_SHIFT);
                uint8x8_t laccum_u8_r = vmovn_u16( laccum_u16_r );
                uint8x8_t haccum_u8_r = vmovn_u16( haccum_u16_r );
                uint8x16_t accum_u8_r = vcombine_u8( laccum_u8_r, haccum_u8_r);

                laccum_u16_g = vshrq_n_u16(laccum_u16_g, NORM_SHIFT);
                haccum_u16_g = vshrq_n_u16(haccum_u16_g, NORM_SHIFT);
                uint8x8_t laccum_u8_g = vmovn_u16( laccum_u16_g );
                uint8x8_t haccum_u8_g = vmovn_u16( haccum_u16_g );
                uint8x16_t accum_u8_g = vcombine_u8( laccum_u8_g, haccum_u8_g);

                laccum_u16_b = vshrq_n_u16(laccum_u16_b, NORM_SHIFT);
                haccum_u16_b = vshrq_n_u16(haccum_u16_b, NORM_SHIFT);
                uint8x8_t laccum_u8_b = vmovn_u16( laccum_u16_b );
                uint8x8_t haccum_u8_b = vmovn_u16( haccum_u16_b );
                uint8x16_t accum_u8_b = vcombine_u8( laccum_u8_b, haccum_u8_b);

                uint8x16x4_t res;
                res.val[0] = accum_u8_r;
                res.val[1] = accum_u8_g;
                res.val[2] = accum_u8_b;

                // 设置A通道为全不透明
                uint8x16_t alpha = vdupq_n_u8(255);  // 创建一个包含全255的向量
                res.val[3] = alpha;  // 将A通道的值设置为255，表示全不透明
                
                vst4q_u8(p_dst + j, res);

                count += 64;
            }
            // 处理剩余部分
            for (int j = count / 4; j < width; j++) {
                int s[4] = {0};
                for (int k = 0; k < ksize; k++) {
                    s[0] += (in[(i + k) * width * channel + j * channel + 0] * kernel[k]);
                    s[1] += (in[(i + k) * width * channel + j * channel + 1] * kernel[k]);
                    s[2] += (in[(i + k) * width * channel + j * channel + 2] * kernel[k]);
                }
                s[0] = s[0] >> NORM_SHIFT;
                s[1] = s[1] >> NORM_SHIFT;
                s[2] = s[2] >> NORM_SHIFT;

                // alpha 不需要做越界处理
                for (int m = 0; m < channel-1; m++) {
                    if (s[m] < 0) {
                        s[m] = 0;
                    }
                    if (s[m] > 255) {
                        s[m] = 255;
                    }
                }

                p_dst[j * channel + 0] = (U8) s[0];
                p_dst[j * channel + 1] = (U8) s[1];
                p_dst[j * channel + 2] = (U8) s[2];
                p_dst[j * channel + 3] = (U8) 0xFF;

                // LOGI("ARGB = %x%02x%02x%02x",0xFF,s[2],s[1],s[0]);
            }
        }
#if ENABLE_OMP
    }
#endif
    } else{
        free(in);
        return;
    }
    free(in);
}

static void horizonFilterNeonU8(U8* src, U8* dst, int height, int width, int channel, uint16_t* kernel, int ksize)
{
    int kCenter = ksize / 2;

    if (channel == 1){
        for(int i = 0; i < height; i++){
            U8* in = (U8*) calloc((width + ksize), sizeof(U8));
            memcpy(in, src + i * width * channel, kCenter * channel);
            memcpy(in + kCenter * channel, src + i * width * channel, width * channel);
            memcpy(in + (kCenter + width) * channel, in + width * channel, kCenter * channel);

            unsigned char* p_dst = dst + i * width;
            int n = width / XA_U8_SIZE;
            int end = n * (XA_U8_SIZE * channel);
            int count = 0;

            for(int j = 0; j < end; j+=XA_U8_SIZE){
                uint16x8_t laccum_u16, haccum_u16;
                laccum_u16 = vmovq_n_u16(0);
                haccum_u16 = vmovq_n_u16(0);

                for(int k = 0; k < ksize; k++){
                    uint8x16_t data = vld1q_u8(in + j + k);
                    uint8x8_t lp_u8 = vget_low_u8( data );
                    uint8x8_t hp_u8 = vget_high_u8( data );

                    uint16x8_t lp_u16 = vmovl_u8( lp_u8 );
                    uint16x8_t hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16 = vmlaq_n_u16(laccum_u16, lp_u16, kernel[k]);
                    haccum_u16 = vmlaq_n_u16(haccum_u16, hp_u16, kernel[k]);
                }
                laccum_u16 = vshrq_n_u16(laccum_u16, NORM_SHIFT);
                haccum_u16 = vshrq_n_u16(haccum_u16, NORM_SHIFT);
                uint8x8_t laccum_u8 = vmovn_u16( laccum_u16 );
                uint8x8_t haccum_u8 = vmovn_u16( haccum_u16 );
                uint8x16_t accum_u8 = vcombine_u8( laccum_u8, haccum_u8 );
                vst1q_u8(p_dst + j, accum_u8);
                count += XA_U8_SIZE;
            }

            for(int j = count / channel; j < width; j++){
                U8 s = 0;
                for(int k = 0; k < ksize; k++){
                    s += (*(in + (j + k) * channel) * kernel[k]) >> NORM_SHIFT;
                }
                if(s < 0)
                    s = 0;
                if(s > 255)
                    s = 255;

                p_dst[j * channel] = (U8)s;
            }

            free(in);
        }
    }else if(channel == 4)
    {
#if ENABLE_OMP
    omp_set_num_threads(OMP_THREADS);
    #pragma omp parallel
        {
        #pragma omp for
#endif
        for(int i = 0; i < height; i++){
            U8* in = (U8*) calloc((width + ksize) * channel, sizeof(U8));
            memcpy(in, src + i * width * channel, kCenter * channel);
            memcpy(in + kCenter * channel, src + i * width * channel, width * channel);
            memcpy(in + (kCenter + width) * channel, in + width * channel, kCenter * channel);

            U8* p_dst = dst + i * width * channel;
            int n = width / XA_U8_SIZE;
            int end = n * (XA_U8_SIZE * channel);
            int count = 0;

            for(int j = 0; j < end; j += XA_U8_SIZE * channel){
                uint16x8_t laccum_u16_r = vmovq_n_u16(0);
                uint16x8_t haccum_u16_r = vmovq_n_u16(0);

                uint16x8_t laccum_u16_g = vmovq_n_u16(0);
                uint16x8_t haccum_u16_g = vmovq_n_u16(0);

                uint16x8_t laccum_u16_b = vmovq_n_u16(0);
                uint16x8_t haccum_u16_b = vmovq_n_u16(0);

                uint8x8_t lp_u8;
                uint8x8_t hp_u8;
                uint16x8_t lp_u16;
                uint16x8_t hp_u16;

                for(int k = 0; k < ksize; k++){
                    uint8x16x4_t data = vld4q_u8(in + j + k * channel);

                    // r
                    lp_u8 = vget_low_u8( data.val[0]);
                    hp_u8 = vget_high_u8( data.val[0]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_r = vmlaq_n_u16(laccum_u16_r, lp_u16, kernel[k]);
                    haccum_u16_r = vmlaq_n_u16(haccum_u16_r, hp_u16, kernel[k]);

                    // g
                    lp_u8 = vget_low_u8( data.val[1]);
                    hp_u8 = vget_high_u8( data.val[1]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_g = vmlaq_n_u16(laccum_u16_g, lp_u16, kernel[k]);
                    haccum_u16_g = vmlaq_n_u16(haccum_u16_g, hp_u16, kernel[k]);

                    // b
                    lp_u8 = vget_low_u8( data.val[2]);
                    hp_u8 = vget_high_u8( data.val[2]);

                    lp_u16 = vmovl_u8( lp_u8 );
                    hp_u16 = vmovl_u8( hp_u8 );

                    laccum_u16_b = vmlaq_n_u16(laccum_u16_b, lp_u16, kernel[k]);
                    haccum_u16_b = vmlaq_n_u16(haccum_u16_b, hp_u16, kernel[k]);
                }

                laccum_u16_r = vshrq_n_u16(laccum_u16_r, NORM_SHIFT);
                haccum_u16_r = vshrq_n_u16(haccum_u16_r, NORM_SHIFT);
                uint8x8_t laccum_u8_r = vmovn_u16( laccum_u16_r );
                uint8x8_t haccum_u8_r = vmovn_u16( haccum_u16_r );
                uint8x16_t accum_u8_r = vcombine_u8( laccum_u8_r, haccum_u8_r);

                laccum_u16_g = vshrq_n_u16(laccum_u16_g, NORM_SHIFT);
                haccum_u16_g = vshrq_n_u16(haccum_u16_g, NORM_SHIFT);
                uint8x8_t laccum_u8_g = vmovn_u16( laccum_u16_g );
                uint8x8_t haccum_u8_g = vmovn_u16( haccum_u16_g );
                uint8x16_t accum_u8_g = vcombine_u8( laccum_u8_g, haccum_u8_g);

                laccum_u16_b = vshrq_n_u16(laccum_u16_b, NORM_SHIFT);
                haccum_u16_b = vshrq_n_u16(haccum_u16_b, NORM_SHIFT);
                uint8x8_t laccum_u8_b = vmovn_u16( laccum_u16_b );
                uint8x8_t haccum_u8_b = vmovn_u16( haccum_u16_b );
                uint8x16_t accum_u8_b = vcombine_u8( laccum_u8_b, haccum_u8_b);

                uint8x16x4_t res;
                res.val[0] = accum_u8_r;
                res.val[1] = accum_u8_g;
                res.val[2] = accum_u8_b;

                // 设置A通道为全不透明
                uint8x16_t alpha = vdupq_n_u8(255);  // 创建一个包含全255的向量
                res.val[3] = alpha;  // 将A通道的值设置为255，表示全不透明

                vst4q_u8(p_dst + j, res);
                count += 64;
            }
            // 处理剩余部分
            for (int j = count / 4; j < width; j++) {
                int s[4] = {0};
                for (int k = 0; k < ksize; k++) {
                    s[0] += (*(in + (j + k) * channel + 0) * kernel[k]);
                    s[1] += (*(in + (j + k) * channel + 1) * kernel[k]);
                    s[2] += (*(in + (j + k) * channel + 2) * kernel[k]);
                }
                s[0] = s[0] >> NORM_SHIFT;
                s[1] = s[1] >> NORM_SHIFT;
                s[2] = s[2] >> NORM_SHIFT;
                // alpha 不需要做越界处理
                for (int m = 0; m < channel -1 ; m++) {
                    if (s[m] < 0) {
                        s[m] = 0;
                    }
                    if (s[m] > 255) {
                        s[m] = 255;
                    }
                }

                p_dst[j * channel + 0] = (U8) s[0];
                p_dst[j * channel + 1] = (U8) s[1];
                p_dst[j * channel + 2] = (U8) s[2];
                p_dst[j * channel + 3] = (U8) 0xFF;
                
            }

            free(in);
        }
#if ENABLE_OMP
        }
#endif
    } else{
        return;
    }
}

void gaussianFilter_u8_Neon(U8* src, U8* dst, int height, int width, int channel, int ksize)
{
    if(ksize > MAX_KERNEL_SIZE){
        return;
    }
#if 1

    uint16_t *weight = (uint16_t*) malloc(sizeof(uint16_t) * ksize);
    uint16_t sum = getGaussianInt(weight, ksize);
    float weightsNorm = (float)(1 << NORM_SHIFT);
    float iwsum = weightsNorm / (float)sum;
    for(int i = 0; i < ksize; i++){
        weight[i] = (uint16_t)((float)weight[i] * iwsum + 0.5);
    }

    verticalFilterNeonU8(src, dst, height, width, channel, weight, ksize);
    horizonFilterNeonU8(dst, dst, height, width, channel, weight, ksize);

    free(weight);
#endif
}
