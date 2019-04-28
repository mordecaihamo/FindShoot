#pragma once
#ifndef _SHOTDATA
#define	_SHOTDATA

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"

using namespace std;
using namespace cv;

//int LookForShots()

class ShotData
{
	vector<Point> mPoints;
	int mLen;
	float mValueInHist;
	float mValueInTime;
public:
	ShotData();
	~ShotData();
};

#endif
