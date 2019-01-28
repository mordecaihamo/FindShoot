#include "pch.h"
#include "ContourData.h"
#include <iostream>
#include "opencv2/highgui.hpp"

const float  ContourData::mMatchThr = 0.6f;

void CopyContourData(ContourData& dest, const ContourData& src)
{
	dest.mPicSize = src.mPicSize;
	dest.mContour = src.mContour;
	dest.mAr = src.mAr;
	dest.mShRct = src.mShRct;
	dest.mRatioWh = src.mRatioWh;
	dest.mRatioAr = src.mRatioAr;
	dest.mRatioFromAll = src.mRatioFromAll;
	dest.mCg = src.mCg;
	dest.mLen = src.mLen;
	dest.mFrameNum = src.mFrameNum;
	dest.mIdxCntr = src.mIdxCntr;
	dest.mCorners = src.mCorners;
	dest.mDistFromLargeCorners = src.mDistFromLargeCorners;
}

ContourData::ContourData()
{
	mAr = 0.0f;
	mShRct.x = 0;
	mShRct.y = 0;
	mRatioWh = 0;
	mRatioAr = 0.0f;
	mRatioFromAll = 0.0f;
	mCg.x = 0;
	mCg.y = 0;
	mLen = 0;
	mCorners.resize(4, Point(0, 0));
	mDistFromLargeCorners.resize(4, Point(-1, -1));
}

ContourData::ContourData(const ContourData& cdIn)
{
	CopyContourData(*this, cdIn);
}

ContourData::ContourData(const vector<Point>& cntr, Size sz):ContourData()
{
	mPicSize = sz;
	mContour = cntr;
	mAr = contourArea(cntr);
	mShRct = boundingRect(cntr);
	mRatioWh = min(mShRct.width, mShRct.height) / (float)max(mShRct.width, mShRct.height);
	mRatioAr = mLen / (float)(mShRct.width*mShRct.height);
	mRatioFromAll = mLen / (float)(sz.width*sz.height);
	mCg = Point2f(mShRct.x + mShRct.width*0.5f, mShRct.y + mShRct.height*0.5f);
	mLen = (int)cntr.size();
	mFrameNum = -1;
	mIdxCntr = -1;
	Point l(sz.width,0), r(0,0), t(0,sz.height), b(0,0);
	for (Point p : cntr)
	{
		if (p.x < l.x)
			l = p;
		if (p.x > r.x)
			r = p;
		if (p.y < t.y)
			t = p;
		if (p.y > b.y)
			b = p;
	}
	if (l.x <= t.x)
	{
		mCorners[0] = l;//tl
		mCorners[1] = t;//tr
		mCorners[2] = r;//br
		mCorners[3] = b;//bl
	}
	else
	{
		mCorners[0] = t;
		mCorners[1] = r;
		mCorners[2] = b;
		mCorners[3] = l;
	}

}

ContourData::ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx):ContourData(cntr, sz)
{
	mFrameNum = frameNum;
	mIdxCntr = idx;
}

ContourData::ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx, vector<Point> cornersOfLarge)
{
	ContourData(cntr, sz,frameNum,idx);
	SetDistFromLargeCorners(cornersOfLarge);
}

ContourData::~ContourData()
{
}

ContourData* ContourData::operator =(const ContourData& cdIn)
{
	CopyContourData(*this, cdIn);

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

ContourData ContourData::operator -(const Point p)
{
	ContourData cdOut = *this;
	cdOut.mShRct.x -= p.x;
	cdOut.mShRct.y -= p.y;
	cdOut.mCg.x -= p.x;
	cdOut.mCg.y -= p.y;
	for (int i = 0; i < cdOut.mContour.size(); ++i)
	{
		cdOut.mContour[i].x -= p.x;
		cdOut.mContour[i].y -= p.y;
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
	//if (dx > 3 || dy > 3)
	//	return res;
	int m = 0, o = 0;
	int numOfFoundPix = 0;
	int minDis = 2;
	for (; m < mLen; ++m)
	{
		for (o=0; o < cdIn.mLen; ++o)
		{
			int dxP = abs(mContour[m].x - cdIn.mContour[o].x);
			int dyP = abs(mContour[m].y - cdIn.mContour[o].y);
			if (dxP<minDis && dyP<minDis)
			{
				++numOfFoundPix;
				break;
			}
		}
	}
	float matchRatio = (float)numOfFoundPix / (float)mLen;
	//double matchRes = matchShapes(mContour, cdIn.mContour, ShapeMatchModes::CONTOURS_MATCH_I2,0);
	//if (prnt) cout << matchRes << endl;
	//if (matchRes > mMatchThr)
	//	return res;
	//else
	//	res = true;
	if (mFrameNum==70 && mIdxCntr==9 && cdIn.mIdxCntr==14)
		prnt = true;
	if (prnt)
	{
		Mat plineCmp(mPicSize, CV_8UC1);
		plineCmp.setTo(0);
		polylines(plineCmp, cdIn.mContour, true, 255, 1, 8);
		polylines(plineCmp, mContour, true, 128, 1, 8);
		cv::imshow("plineCmp", plineCmp);
		cv::waitKey();
	}
	if (matchRatio < mMatchThr)
		return res;
	else
	{
		res = true;
	}
	return res;
}

void ContourData::SetDistFromLargeCorners(const vector<Point>& cornersOfLarge)
{
	for (int i = 0; i < 4; ++i)
	{
		mDistFromLargeCorners[i].x = mCorners[i].x - cornersOfLarge[i].x;
		mDistFromLargeCorners[i].y = mCorners[i].y - cornersOfLarge[i].y;
	}
}