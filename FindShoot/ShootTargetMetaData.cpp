#include "pch.h"
#include "ShootTargetMetaData.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <fstream>
#include <iostream>

bool IsItShot(ContourData& cd)
{
	bool res = false;
	if (cd.mAvgOutRctColor - cd.mAvgInRctColor > 24 && cd.mAvgInRctColor < 175 && cd.mRatioWh > 0.25 && cd.mShRct.width < 25 && cd.mShRct.height < 25)// && cd.mAr>0.01)
	{
		//if(	(cd.mAr > 10 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 2 && cd.mShRct.height > 2 && cd.mRatioWh > 0.54) /*||
		//	(cd.mAr > 25 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height > 4 && cd.mRatioWh > 0.45) ||
		//	(cd.mAr >= 3 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height >= 4 && cd.mRatioWh > 0.79)*/)
			res = true;
	}
	return res;
}

bool IsItShot(ContourData cd, Mat thrMap)
{
	bool res = false;
	double mn, mx;
	Point mnLoc, mxLoc;
	Mat shArea = thrMap(cd.mShRct);
	minMaxLoc(shArea, &mn, &mx, &mnLoc, &mxLoc);
	if (cd.mAvgOutRctColor - cd.mAvgInRctColor > 15 && cd.mAvgInRctColor < 200 && cd.mRatioWh > 0.25 && cd.mShRct.width < 25 && cd.mShRct.height < 25 && mn < 1.0f)
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
void NMS(vector<ContourData>& cntrs, Mat& frameMat, Point& pntMov, Mat* matToDraw)
{
	//pntMov is the movement between this frame and the first frame, if you measure the pixels in the frame, make sure to reduce it from the point
	int sz = (int)cntrs.size();
	for (int i = 0; i < (int)cntrs.size(); ++i)
	{
		if (0&&matToDraw)
		{
			cout << "cntr " << i << endl;
			matToDraw->setTo(0);
			ContourData cdi(cntrs[i]);
			cdi = cdi - pntMov;
			polylines(*matToDraw, cdi.mContour, true, 255, 1, 8);
			imshow("NMScntr", *matToDraw);
			waitKey();
		}
		if (cntrs[i].mLen > 45)//too large, find only the small ones
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
			if (0&&matToDraw)
			{
				cout << "cntr " << i << " and " << j << endl;
				matToDraw->setTo(0);
				ContourData cdi(cntrs[i]);
				cdi = cdi - pntMov;
				ContourData cdj(cntrs[j]);
				cdj = cdj - pntMov;
				polylines(*matToDraw, cdi.mContour, true, 255, 1, 8);
				polylines(*matToDraw, cdj.mContour, true, 128, 1, 8);
				imshow("cntr", *matToDraw);
				//if(i==2 /*&& j==2*/)
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
			if (minDisPoint <= 20)//unite if the distance is small
			{
				Rect uniteRect = cntrs[i].mShRct;
				uniteRect.x = min(uniteRect.x, cntrs[j].mShRct.x);
				uniteRect.y = min(uniteRect.y, cntrs[j].mShRct.y);
				if (uniteRect.x + uniteRect.width < cntrs[j].mShRct.x + cntrs[j].mShRct.width)
				{
					uniteRect.width = cntrs[j].mShRct.x + cntrs[j].mShRct.width - uniteRect.x + 1;
				}
				if (uniteRect.y + uniteRect.height < cntrs[j].mShRct.y + cntrs[j].mShRct.height)
				{
					uniteRect.height = cntrs[j].mShRct.y + cntrs[j].mShRct.height - uniteRect.y + 1;
				}
				//Add border to avoid entering the holes
				int border = 5;
				uniteRect.x = max(uniteRect.x - border - pntMov.x, 0);
				uniteRect.y = max(uniteRect.y - border - pntMov.y, 0);
				if (uniteRect.x + uniteRect.width + 2 * border < frameMat.cols)
				{
					uniteRect.width += 2 * border;
				}
				else
				{
					uniteRect.width = frameMat.cols - uniteRect.x;
				}

				if (uniteRect.y + uniteRect.height + 2 * border < frameMat.rows)
				{
					uniteRect.height += 2 * border;
				}
				else
				{
					uniteRect.height = frameMat.rows - uniteRect.y;
				}
				Mat uniteMat = frameMat(uniteRect);
				int valAtCorners[4];
				valAtCorners[0] = uniteMat.at<uchar>(0, 0);
				valAtCorners[1] = uniteMat.at<uchar>(0, uniteRect.width-1);
				valAtCorners[2] = uniteMat.at<uchar>(uniteRect.height - 1, uniteRect.width - 1);
				valAtCorners[3] = uniteMat.at<uchar>(uniteRect.height - 1, 0);
				int thr = valAtCorners[0];
				for (int i = 1; i < 4; ++i)
				{
					if (thr < valAtCorners[i])
					{
						thr = valAtCorners[i];
					}
				}

				Point pi, pj;
				Rect rctI = cntrs[i].mShRct;
				Rect rctJ = cntrs[j].mShRct;
				rctI.x -= pntMov.x;
				rctI.y -= pntMov.y;
				rctJ.x -= pntMov.x;
				rctJ.y -= pntMov.y;
				Mat matI = frameMat(rctI);
				Mat matJ = frameMat(rctJ);
				double mni, mnj, mxi, mxj;
				minMaxLoc(matI, &mni, &mxi, &pi);
				minMaxLoc(matJ, &mnj, &mxj, &pj);
				pi.x += rctI.x - uniteRect.x;
				pi.y += rctI.y - uniteRect.y;
				pj.x += rctJ.x - uniteRect.x;
				pj.y += rctJ.y - uniteRect.y;

				//pi.x = max(0,(int)round(cntrs[i].mCg.x - uniteRect.x - pntMov.x));
				//pi.y = max(0, (int)round(cntrs[i].mCg.y - uniteRect.y - pntMov.y));
				//pj.x = max(0, (int)round(cntrs[j].mCg.x - uniteRect.x - pntMov.x));
				//pj.y = max(0, (int)round(cntrs[j].mCg.y - uniteRect.y - pntMov.y));

				LineIterator lit(uniteMat, pi,pj , 8);
				vector<uchar> lineVal(lit.count);
				int maxValOnLine = INT_MIN;
				int maxLocation = -1;
				int litCount2 = lit.count >> 1;
				for (int t = 0; t < lit.count; ++t,++lit)
				{
					lineVal[t] = **lit;
					if ((lineVal[t] >= maxValOnLine && t <= litCount2) || (lineVal[t] > maxValOnLine && t > litCount2))
					{
						maxValOnLine = lineVal[t];
						maxLocation = t;
					}
					if (matToDraw)
					{
						**lit = 255;
					}
				}
				//thr = floor(0.5*(mn + thr));
				//Mat thrMat;
				//adaptiveThreshold(uniteMat, thrMat, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, (max(uniteMat.rows, uniteMat.cols) | 1) - 2, 0);
				//threshold(uniteMat, thrMat, thr, 255, THRESH_BINARY);
				Rect localRct = uniteRect;
				localRct.x = 0;
				localRct.y = 0;
				//rectangle(thrMat, localRct, 255, border >> 1);
				
				//vector<vector<Point> > contours;
				//vector<Vec4i> hierarchy;
				//cv::findContours(thrMat, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE);
				//int cntrSz = (int)contours.size();
				if (matToDraw)
				{
					cout << "cntr " << i << " and " << j << endl;
					matToDraw->setTo(0);
					ContourData cdi(cntrs[i]);
					cdi = cdi-pntMov;
					ContourData cdj(cntrs[j]);
					cdj = cdj - pntMov;
					polylines(*matToDraw, cdi.mContour, true, 255, 1, 8);
					polylines(*matToDraw, cdj.mContour, true, 128, 1, 8);
					uniteRect.x -= pntMov.x;
					uniteRect.y -= pntMov.y;
					rectangle(*matToDraw, uniteRect, Scalar(200), 1);
					imshow("cntr", *matToDraw);
					imshow("uniteMat", uniteMat);
					imshow("frameMat", frameMat);
					waitKey();
				}
				
				if(	lit.count <= 6 ||
					(lit.count > 6 && lit.count < 8 && maxValOnLine < 0.75*thr) ||//if they are too close
					(((lineVal[0] > thr || lineVal[0] > 200)&&(lineVal[lit.count - 1] < thr || lineVal[lit.count - 1] < 200))^//If end is low and start is high
					((lineVal[0] < thr || lineVal[0] < 200) && (lineVal[lit.count - 1] > thr || lineVal[lit.count - 1] > 200))) ||//If start is low and end is high
					(lit.count >= 8 && (maxLocation < 0.3*lit.count || maxLocation >= 0.7*lit.count || maxValOnLine < 0.75*thr)))//If the max is at the end or at the start than it is a single shot
				{
					vector<Point> u = cntrs[j].mContour;
					u.insert(u.end(), cntrs[i].mContour.begin(), cntrs[i].mContour.end());
					ContourData cd(u, cntrs[i].mPicSize, cntrs[i].mFrameNum, cntrs[i].mIdxCntr);
					cntrs[i] = cd;//replace with the united contour
					cntrs.erase(cntrs.begin() + j);
					if (j < i)
						--i;
					--j;
				}
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
