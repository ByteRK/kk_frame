
#if defined(_MSC_VER) && _MSC_VER >= 1600
#pragma warning(disable:4244)
#endif

#include <vector>
#include <iostream>
#include <string.h>

#include <glm/glm.hpp>

#include "blur.h"

using namespace std;

double PI = 3.141592653589793;
vector<int> BoxesForGauss(int sigma, int n)  // standard deviation, number of boxes
{
	float wIdeal = sqrt((12 * sigma*sigma / n) + 2);  // Ideal averaging filter w
	int wl = floor(wIdeal);
	if (wl % 2 == 0) wl--;
	int wu = wl + 2;

	float mIdeal = (12 * sigma*sigma - n*wl*wl - 4 * n*wl - 3 * n) / (-4 * wl - 4);
	int m = round(mIdeal);
	// var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

	vector<int> sizes;
	for (int i = 0; i < n; i++)
		sizes.push_back(i < m ? wl : wu);
	return sizes;
}

// algorithm4
void BoxBlurH_4(unsigned char *scl, unsigned char *tcl, int w, int h, int ch, int r)
{
	float iarr = 1.0f / (r + r + 1.0f);
	for (int i = 0; i < h; i++) {
		int ti = i*w*ch;// middle index
		int li = ti;// left index
		int ri = ti + r*ch;// right index
		glm::vec3 fv = glm::vec3(scl[ti], scl[ti + 1], scl[ti + 2]);// first value
		glm::vec3 lv = glm::vec3(scl[ti + (w - 1)*ch], scl[ti + (w - 1)*ch + 1], scl[ti + (w - 1)*ch + 2]);// last value
		glm::vec3 val = glm::vec3(fv.r*(r + 1), fv.g*(r + 1), fv.b*(r + 1));// (r+1)/(2r+1)
		for (int j = 0; j < r*ch; j += ch)
		{
			val.r += scl[ti + j];
			val.g += scl[ti + j + 1];
			val.b += scl[ti + j + 2];
		}
		for (int j = 0; j <= r*ch; j += ch)
		{
			val.r += scl[ri] - fv.r;
			val.g += scl[ri + 1] - fv.g;
			val.b += scl[ri + 2] - fv.b;

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			ri += ch;
			ti += ch;
		}
		for (int j = (r + 1)*ch; j < (w - r)*ch; j += ch)
		{
			val.r += scl[ri] - scl[li];
			val.g += scl[ri + 1] - scl[li + 1];
			val.b += scl[ri + 2] - scl[li + 2];

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			ri += ch;
			li += ch;
			ti += ch;
		}
		for (int j = (w - r)*ch; j < w*ch; j += ch)
		{
			val.r += lv.r - scl[li];
			val.g += lv.g - scl[li + 1];
			val.b += lv.b - scl[li + 2];

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			li += ch;
			ti += ch;
		}
	}
}

void BoxBlurT_4(unsigned char *scl, unsigned char *tcl, int w, int h, int ch, int r)
{
	float iarr = 1.0f / (r + r + 1.0f);
	for (int i = 0; i < w*ch; i += ch) {
		int ti = i;
		int li = ti;
		int ri = ti + r*w*ch;
		glm::vec3 fv = glm::vec3(scl[ti], scl[ti + 1], scl[ti + 2]);
		glm::vec3 lv = glm::vec3(scl[ti + w*(h - 1)*ch], scl[ti + w*(h - 1)*ch + 1], scl[ti + w*(h - 1)*ch + 2]);
		glm::vec3 val = glm::vec3((r + 1)*fv.r, (r + 1)*fv.g, (r + 1)*fv.b);
		for (int j = 0; j < r; j++)
		{
			val.r += scl[ti + j*w*ch];
			val.g += scl[ti + j*w*ch + 1];
			val.b += scl[ti + j*w*ch + 2];
		}
		for (int j = 0; j <= r; j++)
		{
			val.r += scl[ri] - fv.r;
			val.g += scl[ri + 1] - fv.g;
			val.b += scl[ri + 2] - fv.b;

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			ri += w*ch;
			ti += w*ch;
		}
		for (int j = r + 1; j < h - r; j++)
		{
			val.r += scl[ri] - scl[li];
			val.g += scl[ri + 1] - scl[li + 1];
			val.b += scl[ri + 2] - scl[li + 2];

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			li += w*ch;
			ri += w*ch;
			ti += w*ch;
		}
		for (int j = h - r; j < h; j++)
		{
			val.r += lv.r - scl[li];
			val.g += lv.g - scl[li + 1];
			val.b += lv.b - scl[li + 2];

			tcl[ti] = val.r*iarr;
			tcl[ti + 1] = val.g*iarr;
			tcl[ti + 2] = val.b*iarr;

			li += w*ch;
			ti += w*ch;
		}
	}
}

void BoxBlur_4(unsigned char *scl, unsigned char *tcl, int w, int h, int ch, int r)
{
	memcpy(tcl, scl, w*h*ch);
	BoxBlurH_4(tcl, scl, w, h, ch, r);
	BoxBlurT_4(scl, tcl, w, h, ch, r);
}

void GaussianBlur4(unsigned char *scl, unsigned char *tcl, int w, int h, int ch, int r)
{
	vector<int> bxs = BoxesForGauss(r, 3);
	BoxBlur_4(scl, tcl, w, h, ch, (bxs[0] - 1) / 2);
	BoxBlur_4(tcl, scl, w, h, ch, (bxs[1] - 1) / 2);
	BoxBlur_4(scl, tcl, w, h, ch, (bxs[2] - 1) / 2);
}

