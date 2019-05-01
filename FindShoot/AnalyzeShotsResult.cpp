#include "pch.h"
#include "AnalyzeShotsResult.h"
#include "ShotData.h"


//void main()
//{
//	//string histFileName = "C:\\moti\\FindShoot\\MVI_4\\HistOfShots.xml";
//	//string timeFileName = "C:\\moti\\FindShoot\\MVI_4\\TimeOfShots.xml";
//	//string metadataFileName = "C:\\moti\\FindShoot\\MVI_4.txt";
//	//AnalyzeShotsResult analyzer(histFileName, timeFileName, metadataFileName);
//	//analyzer.Compute(histFileName);
//}

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

int AnalyzeShotsResult::Compute()
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

	return numOfShots;
}
