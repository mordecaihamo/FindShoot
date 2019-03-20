#include "pch.h"
#include "ContourData.h"
#include <iostream>
#include "opencv2/highgui.hpp"

using namespace cv;
void CalcAverageBorderColor(Mat& img, ContourData& cd)
{
	if (cd.mLen == 0)
	{
		return;
	}
	cd.mAvgBorderColor = 0;
	for (Point p : cd.mContour)
	{
		cd.mAvgBorderColor += img.at<uchar>(p.y, p.x);
	}
	
	cd.mAvgBorderColor /= cd.mLen;
}

void CalcAverageRectInOutColor(Mat& img, ContourData& cd)
{
	if (cd.mLen == 0 || cd.mShRct.height<2 || cd.mShRct.width<2)
	{
		return;
	}
	cd.mAvgOutRctColor = 0;
	cd.mAvgInRctColor = 0;
	
	Rect outRct(cd.mShRct);
	int shift = 0;
	outRct.x = max(0, outRct.x - shift);
	outRct.y = max(0, outRct.y - shift);
	Rect inRct(cd.mShRct);
	outRct.x = max(outRct.x - 1, 0);
	outRct.y = max(outRct.y - 1, 0);
	if (outRct.x + outRct.width + 1 < img.cols)
	{
		++outRct.width;
	}
	if (outRct.y + outRct.height + 1 < img.rows)
	{
		++outRct.height;
	}
	cd.mAvgOutRctColor += img.at<uchar>(outRct.y, outRct.x);
	cd.mAvgOutRctColor += img.at<uchar>(outRct.y, outRct.x + outRct.width - 1);
	cd.mAvgOutRctColor += img.at<uchar>(outRct.y + outRct.height - 1, outRct.x + outRct.width - 1);
	cd.mAvgOutRctColor += img.at<uchar>(outRct.y + outRct.height - 1, outRct.x);
	cd.mAvgOutRctColor *= 0.25;
	//cd.mAvgOutRctColor = (float)img.at<uchar>(outRct.y, outRct.x);
	//cd.mAvgOutRctColor = min(cd.mAvgOutRctColor, (float)img.at<uchar>(outRct.y, outRct.x + outRct.width - 1));
	//cd.mAvgOutRctColor = min(cd.mAvgOutRctColor, (float)img.at<uchar>(outRct.y + outRct.height - 1, outRct.x + outRct.width - 1));
	//cd.mAvgOutRctColor = min(cd.mAvgOutRctColor, (float)img.at<uchar>(outRct.y + outRct.height - 1, outRct.x));
	

	double mn, mx;
	Point mnLoc, mxLoc;
	if (outRct.width <= 4)
	{
		inRct.x = min(outRct.x + 1, img.cols - 1);
		inRct.width = max(1, inRct.width - 1);
	}
	else
	{
		inRct.x = outRct.x + 2;
		inRct.width = max(1, inRct.width - 2);
	}
	if (outRct.height <= 4)
	{
		inRct.y = min(outRct.y + 1, img.rows - 1);
		inRct.height = max(1, inRct.height - 1);
	}
	else
	{
		inRct.y = outRct.y + 2;
		inRct.height = max(1, inRct.height - 2);
	}
	if (0)
	{
		Mat shot;
		img.copyTo(shot);
		cv::rectangle(shot, outRct, 255);
		cv::rectangle(shot, inRct, 128);
		cv::imshow("cntrIn", shot);
		cv::waitKey();
	}
	Mat imgInRct = img(inRct);
	//cd.mAvgInRctColor = mean(imgInRct).val[0];
	minMaxLoc(imgInRct, &mn, &mx, &mnLoc, &mxLoc);
	cd.mAvgInRctColor = mn;
	//inRct.x += mnLoc.x;
	//inRct.y += mnLoc.y;
	//inRct.width = 2;
	//inRct.height = 2;

	//cd.mAvgInRctColor += img.at<uchar>(inRct.y, inRct.x);
	//cd.mAvgInRctColor += img.at<uchar>(inRct.y, inRct.x + 1);
	//cd.mAvgInRctColor += img.at<uchar>(inRct.y + 1, inRct.x + 1);
	//cd.mAvgInRctColor += img.at<uchar>(inRct.y + 1, inRct.x);
	//cd.mAvgInRctColor *= 0.25;
}


const float  ContourData::mMatchThr = 0.49f;

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
	dest.mDistToCenterOfLarge = src.mDistToCenterOfLarge;
	dest.mAvgBorderColor = src.mAvgBorderColor;
	dest.mAvgOutRctColor = src.mAvgOutRctColor;
	dest.mAvgInRctColor = src.mAvgInRctColor;
}

double AucDisSqr(const cv::Point& p1, const cv::Point& p2)
{
	double dx, dy;
	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	return dx * dx + dy * dy;
}

double AucDis(const cv::Point& p1, const cv::Point& p2)
{
	double dis = AucDisSqr(p1, p2);
	if (dis >= 0)
		dis = sqrt(dis);
	return dis;
}

ContourData::ContourData(void)
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
	mDistToCenterOfLarge = Point(-1, -1);
	mAvgBorderColor = -1;
	mAvgOutRctColor = -1;
	mAvgInRctColor = -1;

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
	//for (Point p : mCorners)
	//{
	//	mCg.x += p.x;
	//	mCg.y += p.y;
	//}
	//mCg.x = round(0.25*mCg.x);
	//mCg.y = round(0.25*mCg.y);
}

ContourData::ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx):ContourData(cntr, sz)
{
	mFrameNum = frameNum;
	mIdxCntr = idx;
}

ContourData::ContourData(const vector<Point>& cntr, Size sz, int frameNum, int idx, vector<Point> cornersOfLarge) :ContourData(cntr, sz, frameNum, idx)
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
	cdOut.mShRct.x = min(mPicSize.width - 1, cdOut.mShRct.x + p.x);
	cdOut.mShRct.y = min(mPicSize.height - 1, cdOut.mShRct.y + p.y);
	cdOut.mCg.x = min(mPicSize.width - 1.0f, cdOut.mCg.x + p.x);
	cdOut.mCg.y = min(mPicSize.height - 1.0f, cdOut.mCg.y + p.y);
	for (int i = 0; i < cdOut.mContour.size(); ++i)
	{
		cdOut.mContour[i].x = min(mPicSize.width - 1, cdOut.mContour[i].x + p.x);
		cdOut.mContour[i].y = min(mPicSize.height - 1, cdOut.mContour[i].y + p.y);
	}

	return cdOut;
}

ContourData ContourData::operator -(const Point p)
{
	ContourData cdOut = *this;
	cdOut.mShRct.x = max(0, cdOut.mShRct.x - p.x);
	cdOut.mShRct.y = max(0, cdOut.mShRct.y - p.y);
	cdOut.mCg.x = max(0.0f, cdOut.mCg.x - p.x);
	cdOut.mCg.y = max(0.0f, cdOut.mCg.y - p.y);
	for (int i = 0; i < cdOut.mContour.size(); ++i)
	{
		cdOut.mContour[i].x = max(0, cdOut.mContour[i].x - p.x);
		cdOut.mContour[i].y = max(0, cdOut.mContour[i].y - p.y);
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
	int minDis = 3;
	int xMov = mDistToCenterOfLarge.x - cdIn.mDistToCenterOfLarge.x;
	int yMov = mDistToCenterOfLarge.y - cdIn.mDistToCenterOfLarge.y;
	for (; m < mLen; ++m)
	{
		for (o=0; o < cdIn.mLen; ++o)
		{
			int dxP = abs((mContour[m].x - xMov) - (cdIn.mContour[o].x));
			int dyP = abs((mContour[m].y - yMov) - (cdIn.mContour[o].y));
			if (dxP<minDis && dyP<minDis)
			{
				++numOfFoundPix;
				break;
			}
		}
	}
	float matchRatio = (float)numOfFoundPix / (float)mLen;
	float matchRatioToIn = (float)numOfFoundPix / (float)cdIn.mLen;
	matchRatio = max(matchRatio, matchRatioToIn);
	if (numOfFoundPix >= cdIn.mLen)
	{
		matchRatio = 1.0f;
	}
	//double matchRes = matchShapes(mContour, cdIn.mContour, ShapeMatchModes::CONTOURS_MATCH_I2,0);
	//if (prnt) cout << matchRes << endl;
	//if (matchRes > mMatchThr)
	//	return res;
	//else
	//	res = true;
	//if (mFrameNum==93 && mIdxCntr==6 && cdIn.mIdxCntr==7)		prnt = true;
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

vector<Point> ContourData::FixSlightlyOpenContour()
{
	vector<Point> rpt;
	
	int midLoc = mLen >> 1;
	int qrtLoc = mLen >> 2;
	int disToMidContour2 = AucDisSqr(mContour[0], mContour[midLoc]);
	int disToQrtContour2 = AucDisSqr(mContour[0], mContour[qrtLoc]); 
	if (disToMidContour2 < disToQrtContour2)
	{
		int oneThirdLoc = (int)round(mLen / 3.0);
		int twoThirdLoc = 2 * oneThirdLoc;
		int minDisLoc = -1;
		double minDisToStart = DBL_MAX;

		for (int i = oneThirdLoc; i < twoThirdLoc; ++i)
		{
			double d = AucDisSqr(mContour[0], mContour[i]);
			if (d < minDisToStart)
			{
				minDisToStart = d;
				minDisLoc = i;
			}
		}
		if (minDisLoc > 0 && minDisLoc<mLen-1)
		{
			rpt = mContour;
			mContour.erase(mContour.begin() + minDisLoc + 1, mContour.end() - 1);
			rpt.erase(rpt.begin(), rpt.begin() + minDisLoc);
			ContourData cd(mContour, mPicSize, mFrameNum, mIdxCntr);
			*this = cd;
		}
	}

	return rpt;
}

bool ContourData::CompareContourAndReturnResidu(const ContourData& cdIn, vector<ContourData>& cdsResidu, Mat* thrMatP/*=NULL*/)
{
	bool prnt = false;
	bool res = false;
	if (prnt) cout << "(" << mCg.x << "," << mCg.y << ")" << " (" << cdIn.mCg.x << "," << cdIn.mCg.y << ")" << endl;
	float dx = abs(cdIn.mCg.x - mCg.x);
	float dy = abs(cdIn.mCg.y - mCg.y);
	if (prnt) cout << "(" << dx << "," << dy << ")" << endl;

	int m = 0, o = 0;
	int numOfFoundPix = 0;
	int minDis = 3;
	int xMov = mDistToCenterOfLarge.x - cdIn.mDistToCenterOfLarge.x;
	int yMov = mDistToCenterOfLarge.y - cdIn.mDistToCenterOfLarge.y;
	if (abs(xMov) > 120 || abs(yMov) > 120)
	{
		return res;
	}
	else if (abs(xMov) > 30 || abs(yMov) > 30)
	{
		xMov = 0;
		yMov = 0;
	}
	bool isFound = false;
	vector<Point> cntr;
	int globalMinDis = INT_MAX;
	//Go over each pixel and find it's reference in the input contour
	for (; m < mLen; ++m)
	{
		isFound = false;
		for (o = 0; o < cdIn.mLen; ++o)
		{
			int dxP = abs((mContour[m].x - xMov) - (cdIn.mContour[o].x));
			int dyP = abs((mContour[m].y - yMov) - (cdIn.mContour[o].y));
			int dis = dxP + dyP;
			if (dis < globalMinDis)
			{
				globalMinDis = dis;
			}
			if (dxP < minDis && dyP < minDis)
			{
				++numOfFoundPix;
				isFound = true;
				break;
			}
		}
		if (!isFound)
		{//Collect the not found pixels, maybe they are a new touching contour
			cntr.push_back(mContour[m]);
		}
	}
	int len = (int)cntr.size();
	int numOfCntrs = 1;
	int minDisBetweenGroups = 10;

	float matchRatio = (float)numOfFoundPix / (float)mLen;
	float matchRatioToIn = (float)numOfFoundPix / (float)cdIn.mLen;
	matchRatio = max(matchRatio, matchRatioToIn);
	if (numOfFoundPix >= cdIn.mLen)
	{
		matchRatio = 1.0f;
	}
	//double matchRes = matchShapes(mContour, cdIn.mContour, ShapeMatchModes::CONTOURS_MATCH_I2,0);
	//if (prnt) cout << matchRes << endl;
	//if (matchRes > mMatchThr)
	//	return res;
	//else
	//	res = true;
	//if (mFrameNum==93 && mIdxCntr==6 && cdIn.mIdxCntr==7)		prnt = true;
	if (prnt)
	{
		Mat plineCmp(mPicSize, CV_8UC1);
		plineCmp.setTo(0);
		polylines(plineCmp, cdIn.mContour, true, 255, 1, 8);
		polylines(plineCmp, mContour, true, 128, 1, 8);
		cv::imshow("plineCmp", plineCmp);
		cv::waitKey();
	}
	//If most of a lot of the pixels did not found in the input this is not the same contour
	if (matchRatio < mMatchThr)
		return res;
	else
	{
		//seperate the points in cntr (not found pixels) to groups of contours, s.t. we wont get contours with very far points in them
		//if (len > 10)
		//{
		//	vector<vector<Point>> cntrTmp(len);
		//	cntrTmp[0].push_back(cntr[0]);
		//	for (int p = 1; p < len; ++p)
		//	{
		//		bool isFound = false;
		//		for (int c = 0; c < numOfCntrs; ++c)
		//		{
		//			int curLen = (int)cntrTmp[c].size();
		//			int dx = abs(cntr[p].x - cntrTmp[c][curLen - 1].x);
		//			int dy = abs(cntr[p].y - cntrTmp[c][curLen - 1].y);
		//			if (dx < minDisBetweenGroups && dy < minDisBetweenGroups)
		//			{
		//				isFound = true;
		//				cntrTmp[c].push_back(cntr[p]);
		//				break;
		//			}
		//		}
		//		if (!isFound)//Not found close point, establish new contour
		//		{
		//			cntrTmp[numOfCntrs].push_back(cntr[p]);
		//			++numOfCntrs;
		//		}
		//	}
		//	//Go over the groups of the contours
		//	for (int c = 0; c < numOfCntrs; ++c)
		//	{//add new contours to check
		//		if (cntrTmp[c].size() > 10)
		//		{
		//			ContourData cdTmp(cntrTmp[c], mPicSize, mFrameNum, mIdxCntr);
		//			cdsResidu.push_back(cdTmp);
		//		}
		//	}
		//}

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

void ContourData::SetDistFromLargeCenter(const Point& pLarge)
{
	mDistToCenterOfLarge.x = pLarge.x;
	mDistToCenterOfLarge.y = pLarge.y;
}

