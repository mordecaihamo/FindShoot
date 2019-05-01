#include "pch.h"
#include "ShotData.h"
#include <iostream>

//https://stackoverflow.com/questions/1257117/a-working-non-recursive-floodfill-algorithm-written-in-c
void FloodfillIter(Mat& vals, Point q, int SEED_COLOR,int lowTol, int highTol, int COLOR, vector<Point>& foundPix, Mat& mask, int COLOR_MASK)
{
	int h = vals.rows;
	int w = vals.cols;

	if (q.y < 0 || q.y > h - 1 || q.x < 0 || q.x > w - 1)
		return;
	foundPix.resize(0);

	vector<Point> stack;
	stack.push_back(q);
	while ((int)stack.size() > 0)
	{
		Point p = stack[(int)stack.size() - 1];
		stack.pop_back();
		int x = p.x;
		int y = p.y;
		if (y < 0 || y > h - 1 || x < 0 || x > w - 1)
			continue;
		float val = vals.at<float>(y, x);
		if (val >= SEED_COLOR - lowTol && val <= SEED_COLOR + highTol)
		{
			foundPix.push_back(p);
			vals.at<float>(y, x) = COLOR;
			stack.push_back(Point(x + 1, y));
			stack.push_back(Point(x - 1, y));
			stack.push_back(Point(x, y + 1));
			stack.push_back(Point(x, y - 1));
		}
		mask.at<uchar>(y, x) = COLOR_MASK;
	}
}

#define _DISPLAY
int LookForShots(Mat& histMat, Mat& timeMat, int thresholdInHist, vector<ShotData>& shots)
{
	int numOfShots = 0;
	Mat hist, histThr, mask;
	histMat.convertTo(hist, CV_32FC1);
	threshold(hist, histThr, thresholdInHist, 255, THRESH_BINARY);
	histThr.convertTo(histThr, CV_8UC1);
	Size sz = histMat.size();
	int cnt = 0;
	int tolH = 150; //upper and lower diff to distitinguish between tight shots
	int tolL = 150;
	for (int r = 0; r < sz.height; ++r)
	{
		for (int c = 0; c < sz.width; ++c)
		{
			int thrval = histThr.at<uchar>(r, c);
			if (thrval > 0)
			{
				--cnt;
				Rect rct;
				float val = hist.at<float>(r, c);
				ShotData sd;
#ifdef _DISPLAY
				double mn16,  mx16;
				minMaxLoc(hist, &mn16, &mx16);
				Mat dispHist;
				hist.convertTo(dispHist, CV_8U, 255.0 / max(1.0, mx16));
				cv::imshow("histBefore", dispHist);
#endif
				if (val - tolL < thresholdInHist)
					tolL = 0;
				FloodfillIter(hist, Point(c, r), val, tolL, tolH, cnt, sd.mPoints, histThr, 0);
#ifdef _DISPLAY
				hist.convertTo(dispHist, CV_8U, 255.0 / max(1.0, mx16));
				cv::imshow("histAfter", dispHist);
				cv::imshow("histThr", histThr);
#endif
				sd.mValueInHist = val;

				//To compute the shot time we will collect all the values from the time mat and find the median in them
				vector<int> timeVals;
				int foundSz = (int)sd.mPoints.size();
				if (foundSz > 0)
				{
					for (int i = 0; i < foundSz; ++i)
					{
						int t = timeMat.at<int>(sd.mPoints[i].y, sd.mPoints[i].x);
						timeVals.push_back(t);
					}
					sort(timeVals.begin(), timeVals.end());
					int foundSz2 = foundSz >> 1;
					sd.mValueInTime = timeVals[foundSz2];
					shots.push_back(sd);
					++numOfShots;
					cout << "Found " << numOfShots << " size " << foundSz << " value in hist " << val << " marked at frame # " << sd.mValueInTime << endl;
				}
				cv::waitKey();
			}
		}
	}
	return numOfShots;
}

ShotData::ShotData()
{
}


ShotData::~ShotData()
{
}
