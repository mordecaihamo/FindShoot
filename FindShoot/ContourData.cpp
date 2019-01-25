#include "pch.h"
#include "ContourData.h"
#include <iostream>


const float  ContourData::mMatchThr = 50.0f;

ContourData::ContourData()
{
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

ContourData ContourData::operator +(const Point p)
{
	ContourData cdOut = *this;
	cdOut.mShRct.x += p.x;
	cdOut.mShRct.y += p.y;
	cdOut.mCg.x += p.x;
	cdOut.mCg.y += p.y;
	for (int i = 0; i < cdOut.mContour.size(); ++i)
	{
		cdOut.mContour[i].x += p.x;
		cdOut.mContour[i].y += p.y;
	}
	return cdOut;
}

bool ContourData::operator ==(const ContourData& cdIn)
{
	bool prnt = false;
	bool res = false;
	if (prnt) cout << "(" << mCg.x << "," << mCg.y << ")" << " (" << cdIn.mCg.x << "," << cdIn.mCg.y << ")" << endl;
	float dx = abs(cdIn.mCg.x - mCg.x);
	float dy = abs(cdIn.mCg.y - mCg.y);
	if (prnt) cout << "(" << dx << "," << dy << ")" << endl;
	if (dx > 3 || dy > 3)
		return res;
	float szRatio = mCntNonZ / (float)cdIn.mCntNonZ;
	if (prnt) cout << szRatio << endl;
	if (szRatio < 0.5f)
		return res;
	double matchRes = matchShapes(mContour, cdIn.mContour, ShapeMatchModes::CONTOURS_MATCH_I2,0);
	if (prnt) cout << matchRes << endl;
	if (matchRes > mMatchThr)
		return res;
	else
		res = true;

	return res;
}
