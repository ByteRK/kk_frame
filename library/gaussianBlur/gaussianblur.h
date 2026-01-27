#ifndef __GAUSSIANBLUR_H
#define __GAUSSIANBLUR_H

#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif
void GaussianBlurFilter(uint8_t *input , int Width, int Height, float GaussianSigma);
#ifdef __cplusplus
}
#endif

#endif //__GAUSSIANBLUR_H