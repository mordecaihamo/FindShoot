#include "pch.h"
#include "AnalyzeShotsResult.h"
#include "ShotData.h"
#include "MovmentUtils.h"


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

	FileStorage fs1(mFileHisto, FileStorage::READ);
	fs1["shotsHistogramMat"] >> mShotsHistogramMat;
	fs1.release();
	FileStorage fs2(mFileTime, FileStorage::READ);
	fs2["shotsFrameNumMat"] >> mShotsFrameNumMat;
	fs2.release();
}

AnalyzeShotsResult::AnalyzeShotsResult(String& histFileName, String& timeFileName, String& metadataFileName)
	:AnalyzeShotsResult(histFileName, timeFileName)
{
	LoadMetaData(metadataFileName);
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

	double mn16, mx16;
	Mat shot(sz, CV_8UC1);
	Mat shotsFound(sz, CV_32FC1);
	mShotsHistogramMat.convertTo(shotsFound, shotsFound.type());
	threshold(shotsFound, shotsFound, 50, 255, THRESH_BINARY);
	minMaxLoc(mShotsHistogramMat, &mn16, &mx16);
	mShotsHistogramMat.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
	cv::imshow("shotsHistogramMat", shot);
	minMaxLoc(mShotsFrameNumMat, &mn16, &mx16);
	mShotsFrameNumMat.convertTo(shot, shot.type(), 255.0 / mx16);
	cv::imshow("shotsFrameNumMat", shot);
	shotsFound.convertTo(shot, shot.type());
	cv::imshow("shotsFound", shot);
	cv::waitKey();
	vector<ShotData> sds;
	numOfShots = LookForShots(mShotsHistogramMat, mShotsFrameNumMat, 50, sds);
	sort(sds.begin(), sds.end(), CompareShotData);
	/*Compute the distance from the metadata and cg*/
	for (int i = 0; i < numOfShots; ++i)
	{
		if (sds[i].mLen > 0)
		{
			for (int l = 0; l < sds[i].mLen; ++l)
			{
				sds[i].mCgX += sds[i].mPoints[l].first.x;
				sds[i].mCgY += sds[i].mPoints[l].first.y;
			}
			sds[i].mCgX /= sds[i].mLen;
			sds[i].mCgY /= sds[i].mLen;
			for (int l = 0; l < 4; ++l)
			{
				sds[i].mDisFromCorners[l] = AucDis(sds[i].mCgX, sds[i].mCgY, (float)mMetaData.mPoints[l].x, (float)mMetaData.mPoints[l].y);
			}
			sds[i].mDisFromCenter = AucDis(sds[i].mCgX, sds[i].mCgY, (float)mMetaData.mCenter.x, (float)mMetaData.mCenter.y);
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


	return numOfShots;
}
