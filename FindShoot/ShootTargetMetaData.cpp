#include "pch.h"
#include "ShootTargetMetaData.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <fstream>
#include <iostream>

bool IsItShot(ContourData cd)
{
	bool res = false;
	if (cd.mAvgOutRctColor - cd.mAvgInRctColor > 15 && cd.mAvgInRctColor < 200 && cd.mRatioWh > 0.25 && cd.mShRct.width < 25 && cd.mShRct.height < 25)
	{
		//if(	(cd.mAr > 10 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 2 && cd.mShRct.height > 2 && cd.mRatioWh > 0.54) /*||
		//	(cd.mAr > 25 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height > 4 && cd.mRatioWh > 0.45) ||
		//	(cd.mAr >= 3 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height >= 4 && cd.mRatioWh > 0.79)*/)
			res = true;
	}
	return res;
}

void drawPolyRect(cv::Mat& img, const Point* p, Scalar color, int lineWd)
{
	if (lineWd > 0)
	{
		for (int i = 0; i < 4; ++i)
		{
			line(img, p[i], p[(i + 1) % 4], color, lineWd);
		}
	}
	else if(lineWd < 0)
	{
		fillConvexPoly(img, p, 4, color);
	}
}

//Unite small contour near large contour
void NMS(vector<ContourData>& cntrs, Mat* matToDraw)
{
	int sz = (int)cntrs.size();
	if (matToDraw)
	{
	}
	for (int i = 0; i < (int)cntrs.size(); ++i)
	{
		if (matToDraw)
		{
			cout << "cntr " << i << endl;
			matToDraw->setTo(0);
			polylines(*matToDraw, cntrs[i].mContour, true, 255, 1, 8);
			imshow("cntr", *matToDraw);
			waitKey();
		}
		if (cntrs[i].mLen > 29)//too large, find only the small ones
		{
			continue;
		}
		int minDisCntr = INT_MAX;
		int minDisCntrIdx = -1;
		//Find the contours that is the closest
		for (int j = 0; j < (int)cntrs.size(); ++j)
		{
			if (i == j)
				continue;
			if (matToDraw)
			{
				cout << "cntr " << i << " and " << j << endl;
				matToDraw->setTo(0);
				polylines(*matToDraw, cntrs[i].mContour, true, 255, 1, 8);
				polylines(*matToDraw, cntrs[j].mContour, true, 128, 1, 8);
				imshow("cntr", *matToDraw);
				//if(i==2 && j==3)
					waitKey();
			}
			int dx = abs(cntrs[i].mCg.x - cntrs[j].mCg.x);
			int dy = abs(cntrs[i].mCg.y - cntrs[j].mCg.y);
			//Check if the center of gravity is too far
			if (dx > 15 && dy > 15)
				continue;

			int minDisPoint = INT_MAX;
			for (int c1 = 0; c1 < cntrs[i].mLen; ++c1)
			{
				for (int c2 = 0; c2 < cntrs[j].mLen; ++c2)
				{
					int dx = cntrs[i].mContour[c1].x - cntrs[j].mContour[c2].x;
					int dy = cntrs[i].mContour[c1].y - cntrs[j].mContour[c2].y;
					int dis = dx * dx + dy * dy;
					if (0 && matToDraw && i == 2 && j == 3)
					{
						matToDraw->at<uchar>(cntrs[i].mContour[c1].y, cntrs[i].mContour[c1].x) = 128;
						matToDraw->at<uchar>(cntrs[j].mContour[c2].y, cntrs[j].mContour[c2].x) = 255;
						cout << dis << endl;
						imshow("cntr", *matToDraw);
						waitKey();
						matToDraw->at<uchar>(cntrs[i].mContour[c1].y, cntrs[i].mContour[c1].x) = 255;
						matToDraw->at<uchar>(cntrs[j].mContour[c2].y, cntrs[j].mContour[c2].x) = 128;
					}
					if (dis < minDisPoint)
						minDisPoint = dis;
				}
			}
			if (minDisPoint < minDisCntr)
			{
				minDisCntr = minDisPoint;
				minDisCntrIdx = j;
			}
			if (minDisPoint <= 10)//unite if the distance is small
			{
				vector<Point> u = cntrs[j].mContour;
				u.insert(u.end(), cntrs[i].mContour.begin(), cntrs[i].mContour.end());
				ContourData cd(u, cntrs[i].mPicSize, cntrs[i].mFrameNum, cntrs[i].mIdxCntr);
				cntrs[i] = cd;//replace with the united contour
				cntrs.erase(cntrs.begin() + j);
				--j;				
			}
		}
	}
}

ShootTargetMetaData::ShootTargetMetaData()
{
	for (int i = 0; i < 4; i++)
	{
		mPoints[i].x = 0;
		mPoints[i].y = 0;
	}

	mCenter.x = 0;
	mCenter.y = 0;
}


ShootTargetMetaData::~ShootTargetMetaData()
{
}

void ShootTargetMetaData::DisplayTarget()
{
	mOrgMat.copyTo(mDrawMat);
	drawPolyRect(mDrawMat, mPoints, mRectColor, 1);
	circle(mDrawMat, mCenter, 10, mCenterColor);
}

istream& operator>>(istream& is, ShootTargetMetaData& md)
{
	for (int i=0;i<4;++i)
	{
		is >> md.mPoints[i].x;
		is >> md.mPoints[i].y;
	}

	is >> md.mCenter.x;
	is >> md.mCenter.y;
	for (int i = 0; i < 3; ++i)
	{
		is >> md.mRectColor.val[i];
	}
	for (int i = 0; i < 3; ++i)
	{
		is >> md.mCenterColor.val[i];
	}

	return is;
}

ostream& operator<<(ostream& os, ShootTargetMetaData& md)
{
	for (Point p : md.mPoints)
	{
		os << p.x<<" ";
		os << p.y << " ";
	}

	os << md.mCenter.x << " ";
	os << md.mCenter.y << " ";
	for (int i = 0; i < 3; ++i)
	{
		os << md.mRectColor.val[i] << " ";
	}
	for (int i = 0; i < 3; ++i)
	{
		os << md.mCenterColor.val[i] << " ";
	}

	return os;
}

int ShootTargetMetaData::ToFile(string filename)
{
	ofstream outStream(filename.c_str(), std::ofstream::out);
	outStream << *this;
	return 0;
}

int ShootTargetMetaData::FromFile(string filename)
{
	ifstream inStream(filename.c_str());
	inStream >> *this;
	return 0;
}
