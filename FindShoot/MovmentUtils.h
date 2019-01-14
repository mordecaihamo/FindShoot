#pragma once
#ifndef _MOVMENTUTILS
#define	_MOVMENTUTILS
#include "pch.h"
//#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
using namespace std;
/**  @function Erosion  */
void Erosion(Mat &src, Mat &dst, int erosion_size);
/** @function Dilation */
void Dilation(Mat &src, Mat &dst, int dilation_size, int dilationType = 0);

void FindMovment(Mat& a, Mat& b, int& x, int& y, Rect& rct, int lookDis, bool isToDisplay = false);

Rect FindInboundRect(Rect rct, const Point* rectPoints);

#endif