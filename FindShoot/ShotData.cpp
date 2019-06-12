#include "pch.h"
#include "ShotData.h"
#include <iostream>

bool comparePix(pair<Point, float> a, pair<Point, float> b)
{
	return a.second > b.second;
}

//https://stackoverflow.com/questions/1257117/a-working-non-recursive-floodfill-algorithm-written-in-c
void FloodfillIter(Mat& vals, Point q, int SEED_COLOR,int lowTol, int highTol, int COLOR, vector<pair<Point, float>>& foundPix, Mat& mask, int COLOR_MASK)
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
			float v = mask.at<float>(y, x);
			foundPix.push_back(make_pair(p,v));
			
			vals.at<float>(y, x) = COLOR;
			mask.at<float>(y, x) = COLOR_MASK;
			stack.push_back(Point(x + 1, y));
			stack.push_back(Point(x - 1, y));
			stack.push_back(Point(x, y + 1));
			stack.push_back(Point(x, y - 1));
		}
	}
}

//#define _DISPLAY
int LookForShots(Mat& histMat, Mat& timeMat, int thresholdInHist, vector<ShotData>& shots, int isDebugMode)
{
	int numOfShots = 0;
	Mat hist, histThr, mask, lowHistThr;
	histMat.convertTo(hist, CV_32FC1);
	threshold(hist, histThr, thresholdInHist, 255, THRESH_BINARY);
	if(isDebugMode)
		threshold(hist, lowHistThr, thresholdInHist>>2, 255, THRESH_BINARY);
	//histThr.convertTo(histThr, CV_8UC1);
	Size sz = histMat.size();
	int cnt = 0;
	int tolH = 150; //upper and lower diff to distitinguish between tight shots
	int tolL = 150;
	//Go over the pixels of the threshold hist mat, every pixel that is above thr is shot cand
	for (int r = 0; r < sz.height; ++r)
	{
		for (int c = 0; c < sz.width; ++c)
		{
			int thrval = histThr.at<float>(r, c);
			if (thrval > 128)
			{
				--cnt;
				Rect rct;
				float val = hist.at<float>(r, c);
				ShotData sd;
				double mn16, mx16;
				if (isDebugMode)
				{				
					minMaxLoc(hist, &mn16, &mx16);
					Mat dispHist;
					hist.convertTo(dispHist, CV_8U, 255.0 / max(1.0, mx16));
					rectangle(dispHist, Point(c, r), Point(min(sz.width - 1, c + 10), min(sz.height - 1, r + 10)), Scalar(255.0));
					cv::imshow("histBefore", dispHist);
					//cv::waitKey();
				}
				if (val - tolL < thresholdInHist)
					tolL = 0;
				//Get the points that touching it
				FloodfillIter(histThr, Point(c, r), thrval, tolL, tolH, cnt, sd.mPoints, hist, 0);
				if (isDebugMode)
				{
					Mat dispHist;
					hist.convertTo(dispHist, CV_8U, 255.0 / max(1.0, mx16));
					cv::imshow("histAfter", dispHist);
					histThr.convertTo(dispHist, CV_8U);
					cv::imshow("histThr", dispHist);
					lowHistThr.convertTo(dispHist, CV_8U);
					cv::imshow("lowHistThr", dispHist);
					cv::waitKey();
				}
				//Split the blob to several blobs if shots touch each other
				vector<ShotData> sdsSplit;
				sd.mIsFromSplit = false;
				sd.mLen = (int)sd.mPoints.size();
				//Do the split
				//Mat dispMat(sz, CV_8UC1);
				//dispMat.setTo(0);
				//int splitsFound = sd.Split(sdsSplit,&dispMat);
				int splitsFound = sd.Split(sdsSplit);
				if (splitsFound > 0)
				{
					for (int indSd = 0; indSd < (int)sdsSplit.size(); ++indSd)
					{
						sd.mLen -= sdsSplit[indSd].mLen;
					}
				}
				sd.mValueInHist = val;
				sdsSplit.push_back(sd);
				for (int indSd = 0; indSd < (int)sdsSplit.size(); ++indSd)
				{
					//To compute the shot time we will collect all the values from the time mat and find the median in them
					vector<int> timeVals;
					int foundSz = (int)sdsSplit[indSd].mPoints.size();
					if (foundSz > 0)
					{
						for (int i = 0; i < foundSz; ++i)
						{
							int t = timeMat.at<int>(sdsSplit[indSd].mPoints[i].first.y, sdsSplit[indSd].mPoints[i].first.x);
							timeVals.push_back(t);
						}
						sort(timeVals.begin(), timeVals.end());
						int foundSz2 = foundSz >> 1;
						sdsSplit[indSd].mValueInTime = timeVals[foundSz2];
						if (sdsSplit[indSd].mValueInTime > 75)//if this blob was after 3 sec. than add it, else it was at the starting frame
						{
							shots.push_back(sdsSplit[indSd]);
							++numOfShots;
							cout << "Found " << numOfShots << " size " << foundSz << " value in hist " << val << " marked at frame # " << sd.mValueInTime << endl;
						}
						else
							cout<< "First frame marks with size"<<foundSz << " value in hist " << val << " marked at frame # " << sd.mValueInTime << endl;
					}
				}
			}
		}
	}
	return numOfShots;
}

ShotData::ShotData()
{
	mDisFromCorners.resize(4, 0.0f);
}


ShotData::~ShotData()
{
}

void DrawBinPoints(vector<pair<Point, float>>& binPoints, Mat& displayMat, uchar color)
{
	int szBin = (int)binPoints.size();
	
	for (int i = 0; i < szBin; ++i)
	{
		displayMat.at<uchar>(binPoints[i].first.y, binPoints[i].first.x) = color;
	}
}

int ShotData::Split(vector<ShotData>& sds, Mat* displayMat)
{
	int numOfShots = 0;
	int len = (int)mPoints.size();
	if (len < 5)
		return numOfShots;
	vector<pair<Point, float>> allP = mPoints;
	
	sort(allP.begin(), allP.end(), comparePix);
	float curMaxVal = allP[0].second;
	int binInterval = 100;
	int numOfBins = (int)(curMaxVal / binInterval);
	if (numOfBins <= 1)
		return numOfShots;
	vector <vector<pair<Point, float>>> histP(numOfBins);
	int curBin = 0;
	//compute the histogram of the values in the spot, if we will get a large bin than it is a shot
	for (int l = 0; l < len; ++l)
	{
		if (curMaxVal - allP[l].second < binInterval)
		{
			histP[curBin].push_back(allP[l]);
		}
		else if (curBin < numOfBins)
		{
			if(curBin < numOfBins-1)
				++curBin;
			curMaxVal = allP[l].second;
			histP[curBin].push_back(allP[l]);
		}
	}
	//The first one (0) is the shot itself, so skip it.
	float stdvMxDis = 2.0f;
	uchar clr = 255;
	for (int n = 0; n <= curBin; ++n)
	{
		if (displayMat)
		{
			DrawBinPoints(histP[n], *displayMat,clr);
			clr -= 20;
			if (clr < 0)
				clr = 255;
			imshow("disp", *displayMat);
			waitKey();
		}
		if (n == 0)
			continue;
		if ((int)histP[n].size() >= 5)
		{
			//Possible new touching shot
			//Check with the rect cover test
			Rect rct;
			int mnx = INT16_MAX, mxx = -1, mny = INT16_MAX, mxy = -1;
			float cgX = 0.0f, cgY = 0.0f, stdvX = 0.0f, stdvY = 0.0f;
			float cgXnoOutLiers = 0.0f, cgYnoOutLiers = 0.0f;
			int szBin = (int)histP[n].size();
			int szAfterRemoveOutliers = szBin;
			ShotData sd;
			for (int i = 0; i < szBin; ++i)
			{
				cgX += histP[n][i].first.x;
				cgY += histP[n][i].first.y;
				if (histP[n][i].first.x < mnx)
					mnx = histP[n][i].first.x;
				if (histP[n][i].first.x > mxx)
					mxx = histP[n][i].first.x;
				if (histP[n][i].first.y < mny)
					mny = histP[n][i].first.y;
				if (histP[n][i].first.y > mxy)
					mxy = histP[n][i].first.y;
			}
			cgX /= szBin;
			cgY /= szBin;
			for (int i = 0; i < szBin; ++i)
			{
				float dx = histP[n][i].first.x - cgX;
				float dy = histP[n][i].first.y - cgY;
				stdvX += dx * dx;
				stdvY += dy * dy;
			}
			stdvX /= szBin;
			stdvY /= szBin;
			stdvX = sqrtf(stdvX);
			stdvY = sqrtf(stdvY);

			for (int i = 0; i < szBin; ++i)
			{
				float dx = abs(histP[n][i].first.x - cgX);
				float dy = abs(histP[n][i].first.y - cgY);
				if (dx > stdvMxDis*stdvX || dy > stdvMxDis*stdvY)
				{
					--szAfterRemoveOutliers;
					continue;
				}
				cgXnoOutLiers += histP[n][i].first.x;
				cgYnoOutLiers += histP[n][i].first.y;
				sd.mPoints.push_back(histP[n][i]);
				if (histP[n][i].first.x < mnx)
					mnx = histP[n][i].first.x;
				if (histP[n][i].first.x > mxx)
					mxx = histP[n][i].first.x;
				if (histP[n][i].first.y < mny)
					mny = histP[n][i].first.y;
				if (histP[n][i].first.y > mxy)
					mxy = histP[n][i].first.y;
			}
			if (szAfterRemoveOutliers == 0)
			{
				continue;
			}
			cgXnoOutLiers /= szAfterRemoveOutliers;
			cgYnoOutLiers /= szAfterRemoveOutliers;
			int dx = mxx - mnx + 1;
			int dy = mxy - mny + 1;
			//compute the squre area
			int s = dx * dy;
			//compute the ratio between the num of pix to the area. Assuming that the shot is round the ratio should be high
			float rat = (float)histP[n].size() / s;
			if (rat > 0.3 || (rat>0.1 && histP[n].size()>20))
			{		
				if (displayMat)
				{
					displayMat->at<uchar>((int)cgYnoOutLiers, (int)cgXnoOutLiers) = 100;
					imshow("disp", *displayMat);
					waitKey();
				}
				sd.mCgX = cgXnoOutLiers;
				sd.mCgY = cgYnoOutLiers;
				sd.mLen = szAfterRemoveOutliers;
				sd.mValueInHist = curMaxVal + n * binInterval;
				sd.mIsFromSplit = true;
				sds.push_back(sd);
				++numOfShots;
			}
		}
	}

	return numOfShots;
}
