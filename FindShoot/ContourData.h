#pragma once
#ifndef _CONTOURDATA
#define	_CONTOURDATA
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"

using namespace cv;

class ContourData
{
public:
	ContourData();
	~ContourData();

	double mAr;
	Rect mShRct;
	float mRatioWh;
	int mCntNonZ;
	float mRatioAr;
	float mRatioFromAll;
	Point2f mCg;
};
#endif
