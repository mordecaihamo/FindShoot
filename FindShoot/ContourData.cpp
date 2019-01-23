#include "pch.h"
#include "ContourData.h"


ContourData::ContourData()
{
	//mMatchThr = 0.95f;
	mAr = 0.0f;
	mShRct.x = 0;
	mShRct.y = 0;
	mRatioWh = 0;
	mCntNonZ = 0;
	mRatioAr = 0.0f;
	mRatioFromAll = 0.0f;
	mCg.x = 0;
	mCg.y = 0;
}

ContourData::ContourData(const ContourData& cdIn)
{
	mPicSize = cdIn.mPicSize;
	mContour = cdIn.mContour;
	mAr = cdIn.mAr;
	mShRct = cdIn.mShRct;
	mRatioWh = cdIn.mRatioWh;
	mCntNonZ = cdIn.mCntNonZ;
	mRatioAr = cdIn.mRatioAr;
	mRatioFromAll = cdIn.mRatioFromAll;
	mCg = cdIn.mCg;
}

ContourData::ContourData(const vector<Point>& cntr, Size sz)
{
	mPicSize = sz;
	mContour = cntr;
	mAr = contourArea(cntr);
	mShRct = boundingRect(cntr);
	mRatioWh = min(mShRct.width, mShRct.height) / (float)max(mShRct.width, mShRct.height);
	mCntNonZ = (int)cntr.size();
	mRatioAr = mCntNonZ / (float)(mShRct.width*mShRct.height);
	mRatioFromAll = mCntNonZ / (float)(sz.width*sz.height);
	mCg = Point2f(mShRct.x + mShRct.width*0.5f, mShRct.y + mShRct.height*0.5f);
}

ContourData::~ContourData()
{
}

ContourData* ContourData::operator =(const ContourData& cdIn)
{
	mPicSize = cdIn.mPicSize;
	mContour = cdIn.mContour;
	mAr = cdIn.mAr;
	mShRct = cdIn.mShRct;
	mRatioWh = cdIn.mRatioWh;
	mCntNonZ = cdIn.mCntNonZ;
	mRatioAr = cdIn.mRatioAr;
	mRatioFromAll = cdIn.mRatioFromAll;
	mCg = cdIn.mCg;

	return this;
}

bool ContourData::operator ==(const ContourData& cdIn)
{
	bool res = false;
	float dx = abs(cdIn.mCg.x - mCg.x);
	float dy = abs(cdIn.mCg.y - mCg.y);
	if (dx > 3 || dy > 3)
		return res;
	if (min(mCntNonZ, cdIn.mCntNonZ) / max(mCntNonZ, cdIn.mCntNonZ) < 0.7)
		return res;
	double matchRes = matchShapes(mContour, cdIn.mContour, ShapeMatchModes::CONTOURS_MATCH_I3,0);
	if (matchRes > 0.1)
		return res;
	else
		res = true;

	return res;
}
