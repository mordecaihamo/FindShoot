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
	vector<pair<Point,float>> mPoints;
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
	ShotData(vector<pair<Point, float>> pointsOfShot);
	~ShotData();
	ShotData& operator = (const ShotData& sdIn);
	int Split(vector<ShotData>& sds, Mat* displayMat = NULL);
	int Split(vector<ShotData>& sds,int shotMinLen, int shotminDiam, Mat* displayMat);
};

int LookForShots(Mat& histMat, Mat& timeMat, int thresholdInHist, vector<ShotData>& shots, int isDebugMode);
#endif
