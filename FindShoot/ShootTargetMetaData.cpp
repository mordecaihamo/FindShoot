#include "pch.h"
#include "ShootTargetMetaData.h"
#include "opencv2/imgproc.hpp"
#include <fstream>

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
