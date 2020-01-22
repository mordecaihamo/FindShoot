#pragma once
#ifndef _FINDSHOOT_H
#define _FINDSHOOT_H
#include <Windows.h>
#include "ShootTargetMetaData.h"

#define FINDSHOOTEXPORT __declspec(dllexport)
extern "C" FINDSHOOTEXPORT int Analyze(char* vidName, int isDebugMode);                           
extern "C" FINDSHOOTEXPORT int FindShoots(const char* vidName, int selectedCh, HBITMAP imgBuffer, int imgHeight, int imgWidth, int isDebugMode);

class FINDSHOOTEXPORT FindShoot
{
	int mW;
	int mH;
	ShootTargetMetaData mMetaData;
	vector<ShotData> mShotsData;

	String mFolderPath;
	String mFileHisto;
	String mFileTime;
	String mLastFramePath;
	String mXYmovePath;
	Mat mFirstPic;
	Mat mShotsHistogramMat;
	Mat mShotsFrameNumMat;
	Mat mLastPic;
	int mXmove;
	int mYmove;

public:
	FindShoot();
	FindShoot(String& fileNameFirst);
	~FindShoot();

	int GetWidth() { return mShotsHistogramMat.size().width; };
	int GetHeight() { return mShotsHistogramMat.size().height; };
	int Compute(String& resultFileName, int isDebugMode);
	int LoadMetaData(String& mdFileName);

	void MarkTarget();
	int CompareTwoPics(String& firstPic, String& secondPic);
};


#endif