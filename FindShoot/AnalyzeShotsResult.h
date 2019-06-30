#pragma once
#ifndef _ANALYZESHOTSRESULTS
#define	_ANALYZESHOTSRESULTS

#include "pch.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/video.hpp"
#include "ContourData.h"
#include "ShootTargetMetaData.h"
#include "ShotData.h"

using namespace std;
using namespace cv;

class AnalyzeShotsResult
{
	String mFileHisto;
	String mFileTime;
	String mLastFramePath;
	Mat mShotsHistogramMat;
	Mat mShotsFrameNumMat;
	Mat mLastFrame;
	vector<ContourData> mCntrData;
	ShootTargetMetaData mMetaData;
	vector<ShotData> mShotsData;

public:
	AnalyzeShotsResult();
	AnalyzeShotsResult(String& histFileName, String& timeFileName);
	AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName);
	AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName, String& lastFramePath);
	~AnalyzeShotsResult();

	int GetWidth()	{return mShotsHistogramMat.size().width; };
	int GetHeight() { return mShotsHistogramMat.size().height; };
	int Compute(String& resultFileName, int isDebugMode);
	int LoadMetaData(String& mdFileName);
};

#endif