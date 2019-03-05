#pragma once
#ifndef _SHOOTTARGETMETADATA
#define	_SHOOTTARGETMETADATA
#include "opencv2/core.hpp"
#include "ContourData.h"

using namespace cv;
using namespace std;

void drawPolyRect(cv::Mat& img, const Point* p,Scalar color, int lineWd);
bool IsItShot(ContourData cd);
void NMS(vector<ContourData>& cntrs, Mat* matToDraw = NULL);

class ShootTargetMetaData
{
public:
	ShootTargetMetaData();
	~ShootTargetMetaData();
	void DisplayTarget();
	int ToFile(string filename);
	int FromFile(string filename);

	Point mPoints[4];//Rect
	Point mCenter;
	Mat mOrgMat;
	Mat mDrawMat;
	string mWindowName;
	Scalar mRectColor;
	Scalar mCenterColor;
};

istream& operator>>(istream& is, ShootTargetMetaData& md);
ostream& operator<<(ostream& os, ShootTargetMetaData& md);

#endif
