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
	vector<Point> mPoints;
	int mLen;
	float mValueInHist;
	float mValueInTime;

	ShotData();
	~ShotData();
};

int LookForShots(Mat& histMat, Mat& timeMat, int thresholdInHist, vector<ShotData>& shots);
#endif
