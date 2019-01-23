#pragma once
#ifndef _CONTOURDATA
#define	_CONTOURDATA
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"

using namespace cv;
using namespace std;
class ContourData
{
public:
	ContourData();
	~ContourData();
	ContourData(const ContourData& cdIn);
	ContourData(const vector<Point>& cntr, Size sz);
	ContourData* operator =(const ContourData& cdIn);
	bool operator ==(const ContourData& cdIn);

	static const float mMatchThr=0.95f;
	Size mPicSize;
	vector<Point> mContour;
	double mAr;
	Rect mShRct;
	float mRatioWh;
	int mCntNonZ;
	float mRatioAr;
	float mRatioFromAll;
	Point2f mCg;
};
#endif
