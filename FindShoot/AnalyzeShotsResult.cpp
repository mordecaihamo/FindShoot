#include "pch.h"
#include "AnalyzeShotsResult.h"
#include "ShotData.h"
#include "MovmentUtils.h"
#include <experimental/filesystem>


bool CompareShotData(ShotData a, ShotData b)
{
	return a.mValueInTime < b.mValueInTime;
}

AnalyzeShotsResult::AnalyzeShotsResult()
{
}

AnalyzeShotsResult::AnalyzeShotsResult(String& histFileName, String& timeFileName)
{
	mFileHisto = histFileName;
	mFileTime = timeFileName;
	if (std::experimental::filesystem::exists(mFileHisto) && std::experimental::filesystem::exists(mFileTime))
	{
		FileStorage fs1(mFileHisto, FileStorage::READ);
		fs1["shotsHistogramMat"] >> mShotsHistogramMat;
		fs1.release();
		FileStorage fs2(mFileTime, FileStorage::READ);
		fs2["shotsFrameNumMat"] >> mShotsFrameNumMat;
		fs2.release();
	}
}

AnalyzeShotsResult::AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName)
	:AnalyzeShotsResult(histFileName, timeFileName)
{
	LoadMetaData(metadataFileName);
}

AnalyzeShotsResult::AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName, String& lastFramePath)
	: AnalyzeShotsResult(histFileName, timeFileName, metadataFileName)
{
	mLastFramePath = lastFramePath;
	if (std::experimental::filesystem::exists(mLastFramePath))
	{
		mLastFrame = imread(mLastFramePath);
	}
}

AnalyzeShotsResult::AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName, String& lastFramePath, String& xvMovPath)
	: AnalyzeShotsResult(histFileName, timeFileName, metadataFileName, lastFramePath)
{
	mXYmovePath = xvMovPath;
	if (std::experimental::filesystem::exists(mXYmovePath))
	{
		ifstream inStream(mXYmovePath.c_str(), std::ifstream::in);
		inStream >> mXmove >> mYmove;
		inStream.close();
	}
}

AnalyzeShotsResult::~AnalyzeShotsResult()
{
}

int AnalyzeShotsResult::LoadMetaData(String& mdFileName)
{
	mMetaData.FromFile(mdFileName);
	return 1;
}

int AnalyzeShotsResult::Compute(String& resultFileName, int isDebugMode)
{
	int numOfShots = 0;
	Size sz = mShotsHistogramMat.size();
	if (mShotsHistogramMat.empty() || mShotsFrameNumMat.empty())
		return -1;
	int minHistThr = 10;
	cv::destroyAllWindows();
	double mn16, mx16;
	Mat shot(sz, CV_8UC1);
	Mat shotsFound(sz, CV_32FC1);
	minMaxLoc(mShotsHistogramMat, &mn16, &mx16);

	mShotsHistogramMat.convertTo(shotsFound, shotsFound.type());
	double mn16sc, mx16sc;
	minMaxLoc(shotsFound, &mn16sc, &mx16sc);
	threshold(shotsFound, shotsFound, minHistThr, 255, THRESH_BINARY);
	
	mShotsHistogramMat.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
	cv::imshow("shotsHistogramMat", shot);
	minMaxLoc(mShotsFrameNumMat, &mn16, &mx16);
	mShotsFrameNumMat.convertTo(shot, shot.type(), 255.0 / mx16);
	cv::imshow("shotsFrameNumMat", shot);
	shotsFound.convertTo(shot, shot.type());
	cv::imshow("shotsFound", shot);
	Mat frameWithMarks, canMat;
	if (!mLastFrame.empty())
	{
		cv::imshow("LastFrame", mLastFrame);
		mLastFrame.copyTo(frameWithMarks);
		int thrOfGrad = 60;
		Canny(mLastFrame, canMat, thrOfGrad, 2.75 * thrOfGrad);		
		cv::imshow("canMat", canMat);
	}
	//cv::waitKey();
	vector<ShotData> sds;
	
	numOfShots = LookForShots(mShotsHistogramMat, mShotsFrameNumMat, canMat, minHistThr, sds, mXmove, mYmove, isDebugMode);
	sort(sds.begin(), sds.end(), CompareShotData);
	/*Compute the distance from the metadata and cg*/
	uchar c1 = 255, c2 = 128, c3 = 0;
	for (int i = 0; i < numOfShots; ++i)
	{
		if (sds[i].mLen > 0)
		{
			for (int l = 0; l < 4; ++l)
			{
				sds[i].mDisFromCorners[l] = AucDis(sds[i].mCgX, sds[i].mCgY, (float)mMetaData.mPoints[l].x, (float)mMetaData.mPoints[l].y);
			}
			sds[i].mDisFromCenter = AucDis(sds[i].mCgX, sds[i].mCgY, (float)mMetaData.mCenter.x, (float)mMetaData.mCenter.y);
			if (!mLastFrame.empty())
			{
				circle(frameWithMarks, Point((int)sds[i].mCgX+mXmove, (int)sds[i].mCgY+mYmove), 3, Scalar(c1, c2, c3),-1);
				c1 += 20;
				c2 -= 20;
				c3 += 40;
			}
		}
	}

	/*Write the results to csv a file*/
	ofstream of(resultFileName, ios::out | ios::trunc );
	if (of.is_open())
	{
		of << "Frame#, Time(sec), Size, x, y, TL dis, TR dis, BR dis, BL dis, Center Dis";
		of << " , TL(x y), " << mMetaData.mPoints[0].x << "," << mMetaData.mPoints[0].y;
		of << " , TR(x y), " << mMetaData.mPoints[1].x << "," << mMetaData.mPoints[1].y;
		of << " , BR(x y), " << mMetaData.mPoints[2].x << "," << mMetaData.mPoints[2].y;
		of << " , BL(x y), " << mMetaData.mPoints[3].x << "," << mMetaData.mPoints[3].y;
		of << " , Center(x y), " << mMetaData.mCenter.x << "," << mMetaData.mCenter.y << endl;
		for (int i = 0; i < numOfShots; ++i)
		{
			of << sds[i].mValueInTime << "," << sds[i].mValueInTime * 0.04 << ",";
			of << sds[i].mLen << "," << sds[i].mCgX << "," << sds[i].mCgY << ",";
			for (int d = 0; d < 4; ++d)
			{
				of << sds[i].mDisFromCorners[d] << ",";
			}
			of << sds[i].mDisFromCenter << endl;
		}
		of.close();
	}

	if (!frameWithMarks.empty())
	{
		cv::imshow("MarksLastFrame", frameWithMarks);
	}
	cv::waitKey();
	cv::destroyAllWindows();

	return numOfShots;
}
