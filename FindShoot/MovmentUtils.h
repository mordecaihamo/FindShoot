#pragma once
#ifndef _MOVMENTUTILS
#define	_MOVMENTUTILS
#include "pch.h"
//#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

//using namespace cv;
using namespace std;
/**  @function Erosion  */
void Erosion(cv::Mat &src, cv::Mat &dst, int erosion_size, int erosionType = 0);
/** @function Dilation */
void Dilation(cv::Mat &src, cv::Mat &dst, int dilation_size, int dilationType = 0);

void FindMovment(cv::Mat& a, cv::Mat& b, int& x, int& y, cv::Rect& rct, int lookDis, bool isToZero = false, bool isToDisplay = false);

cv::Rect FindInboundRect(cv::Rect rct, const cv::Point* rectPoints);

void ThresholdByLightMap(cv::Mat& inMat, cv::Mat& outMat, cv::Mat& lightMat, float percFromLight, cv::Rect& inRect, cv::Rect& lightRect, bool isToDisplay = false);

template<class AUC>
AUC AucDis(AUC ax, AUC ay, AUC bx, AUC by)
{
	AUC dx = ax - bx;
	AUC dy = ay - by;
	return sqrt(dx * dx + dy * dy);
}

#endif