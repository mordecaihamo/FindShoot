#pragma once
#ifndef _SHOTDATA
#define	_SHOTDATA

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"

using namespace std;
using namespace cv;

class ShotData
{
public:	
	vector<pair<Point, pair<float, int>>> mPoints;//((x,y), (val_in_hist, appeared_in_frame))
	int mLen;
	float mValueInHist;
	float mValueInTime;
	bool mIsFromSplit;
	float mCgX;
	float mCgY;
	vector<float> mDisFromCorners;
	float mDisFromCenter;

	ShotData();
	ShotData(const ShotData& sdIn);
	ShotData(vector<pair<Point, pair<float, int>>> pointsOfShot);
	ShotData(vector<pair<Point, pair<float, int>>> pointsOfShot, const Mat& timeMat);
	ShotData(vector<Point> points, const Mat& timeMat);
	ShotData(vector<Point> points, const Mat& timeMat, const Mat& histMat);
	~ShotData();
	ShotData& operator = (const ShotData& sdIn);
	int Split(vector<ShotData>& sds, const Mat& timeMat, Mat* displayMat = NULL);
	int Split(vector<ShotData>& sds,int shotMinLen, int shotminDiam,const Mat& timeMat, int thresholdInHist, Mat* displayMat);
	bool IsItShot();
};

int LookForShots(Mat& histMat, Mat& timeMat, Mat& canMat, int thresholdInHist, vector<ShotData>& shots, int mXmove, int mYmove, int isDebugMode);
#endif
