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
	ContourData(void);
	~ContourData();
	ContourData(const ContourData& cdIn);
	ContourData(const vector<Point>& cntr, Size sz);
	ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx);
	ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx, vector<Point> cornersOfLarge);
	ContourData* operator =(const ContourData& cdIn);
	bool operator ==(const ContourData& cdIn);
	vector<Point> FixSlightlyOpenContour();
	bool CompareContourAndReturnResidu(const ContourData& cdIn, vector<ContourData>& cdsResidu, Mat* thrMatP = NULL);
	ContourData operator +(const Point p);
	ContourData operator -(const Point p);
	void SetDistFromLargeCorners(const vector<Point>& cornersOfLarge);
	void SetDistFromLargeCenter(const Point& pLarge);

	const static float mMatchThr;
	Size mPicSize;
	vector<Point> mContour;
	double mAr;
	Rect mShRct;
	float mRatioWh;
	//int mCntNonZ;
	float mRatioAr;
	float mRatioFromAll;
	Point2f mCg;
	int mLen;
	int mFrameNum;
	int mIdxCntr;
	vector<Point> mCorners;
	vector<Point> mDistFromLargeCorners;
	Point mDistToCenterOfLarge;
	float mAvgBorderColor;
	int mMinBorderColor;
	int mMaxBorderColor;
	float mAvgOutRctColor;
	float mAvgInRctColor;
};

void CalcAverageBorderColor(Mat& grad8Thr, ContourData& cd);
void CalcAverageRectInOutColor(Mat& img, ContourData& cd);
#endif
