#pragma once
#ifndef _ANALYZESHOTSRESULTS
#define	_ANALYZESHOTSRESULTS

#include "pch.h"
#include <iostream>
#include <fstream>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/video.hpp"
#include "ContourData.h"
#include "ShootTargetMetaData.h"

using namespace std;
using namespace cv;

class AnalyzeShotsResult
{
	string mFileHisto;
	string mFileTime;
	Mat mShotsHistogramMat;// (sz.height, sz.width, CV_32SC1);
	Mat mShotsFrameNumMat;
	vector<ContourData> mCntrData;
	ShootTargetMetaData mMetaData;

public:
	AnalyzeShotsResult();
	AnalyzeShotsResult(string& histFileName, string& timeFileName);
	AnalyzeShotsResult(string& histFileName, string& timeFileName, string& metadataFileName);
	~AnalyzeShotsResult();

	int Compute(string& csvFileName);
	int LoadMetaData(string& mdFileName);
};

#endif