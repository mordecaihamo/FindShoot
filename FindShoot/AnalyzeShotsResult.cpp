#include "pch.h"
#include "AnalyzeShotsResult.h"


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

AnalyzeShotsResult::AnalyzeShotsResult(string& histFileName, string& timeFileName)
{
	mFileHisto = histFileName;
	mFileTime = timeFileName;

	FileStorage fs1(mFileHisto, FileStorage::READ);
	fs1["myMatrix"] >> mShotsHistogramMat;
	fs1.release();
	FileStorage fs2(mFileTime, FileStorage::READ);
	fs2["myMatrix"] >> mShotsFrameNumMat;
	fs2.release();
}

AnalyzeShotsResult::AnalyzeShotsResult(string& histFileName, string& timeFileName, string& metadataFileName)
	:AnalyzeShotsResult(histFileName, timeFileName)
{
	LoadMetaData(metadataFileName);
}


AnalyzeShotsResult::~AnalyzeShotsResult()
{
}

int AnalyzeShotsResult::LoadMetaData(string& mdFileName)
{
	mMetaData.FromFile(mdFileName);
}

int AnalyzeShotsResult::Compute(string& csvResultFileName)
{
	int numOfShots = 0;
	Size sz = mShotsHistogramMat.size();

	double mn16, mx16;
	Mat shot(sz, CV_8UC1);
	minMaxLoc(mShotsHistogramMat, &mn16, &mx16);
	mShotsHistogramMat.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
	cv::imshow("shotsHistogramMat", shot);
	minMaxLoc(mShotsFrameNumMat, &mn16, &mx16);
	mShotsFrameNumMat.convertTo(shot, shot.type(), 255.0 / mx16);
	cv::imshow("shotsFrameNumMat", shot);

	return numOfShots;
}
