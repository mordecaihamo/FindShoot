#include "pch.h"
#include "ShotData.h"
#include <iostream>

bool ComparePix(pair<Point, float> a, pair<Point, float> b)
{
	return a.second > b.second;
}

bool CompareSpotsBySize(ShotData& a, ShotData& b)
{
	return a.mLen > b.mLen;
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
	Size sz = histMat.size();
	threshold(hist, histThr, thresholdInHist, 255, THRESH_BINARY);
	Mat dispShots;
	if (isDebugMode)
	{
		dispShots = Mat(sz, CV_8U);
		dispShots.setTo(0);
		threshold(hist, lowHistThr, thresholdInHist >> 2, 255, THRESH_BINARY);
	}
	//histThr.convertTo(histThr, CV_8UC1);
	
	
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
					rectangle(dispHist, Point(c - 5, r - 5), Point(min(sz.width - 1, c + 5), min(sz.height - 1, r + 5)), Scalar(255.0));
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
				shots.push_back(sd);
			}
		}
	}
	numOfShots = (int)shots.size();
	sort(shots.begin(), shots.end(), CompareSpotsBySize);
	int percentile10 = (int)round(numOfShots*0.9f);
	int shotMinSize = shots[percentile10].mLen;
	int shotRad = sqrt(shotMinSize);
	int shotMaxSizeAllowed = (int)round(shotMinSize*1.5f);
	//Go over the first 0.5 spots and check if they are too big and needs to go to a split process
	for (int i = 0; i < numOfShots*0.5; ++i)
	{
		if (shots[i].mLen < shotMaxSizeAllowed)
		{
			break;//Since they are sorted, all of the following will also be small
		}
		//Split the blob to several blobs if shots touch each other
		vector<ShotData> sdsSplit;
		ShotData sd;// (shots[i]);
		sd.mIsFromSplit = false;
		sd.mLen = (int)sd.mPoints.size();
		//Do the split
		Mat dispMat(sz, CV_8UC1);
		dispMat.setTo(0);
		//int splitsFound = sd.Split(sdsSplit,&dispMat);
		int splitsFound = sd.Split(sdsSplit, shotMinSize, shotRad, &dispMat);
		//int splitsFound = sd.Split(sdsSplit);
		if (splitsFound > 0)
		{
			for (int indSd = 0; indSd < (int)sdsSplit.size(); ++indSd)
			{
				sd.mLen -= sdsSplit[indSd].mLen;
			}
		}
		char val = 0;
		sd.mValueInHist = val;
		sdsSplit.push_back(sd);
		uchar color = 255;
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
					if (isDebugMode)
					{
						for (int i = 0; i < sdsSplit[indSd].mLen; ++i)
						{
							dispShots.at<uchar>(sdsSplit[indSd].mPoints[i].first) = color;
						}
						cv::imshow("spots", dispShots);
								
					}
				}
				else
					cout<< "First frame marks with size"<<foundSz << " value in hist " << val << " marked at frame # " << sd.mValueInTime << endl;
			}
			if (isDebugMode)
			{
				color -= 40;
				cv::waitKey();
			}
		}		
	}
	return numOfShots;
}

ShotData::ShotData()
{
	mDisFromCorners.resize(4, 0.0f);
}

ShotData::ShotData(vector<pair<Point, float>> pointsOfShot):ShotData()
{
	mPoints = pointsOfShot;
	mLen = (int)mPoints.size();
}

ShotData::ShotData(ShotData& sdIn)
{
	mPoints = sdIn.mPoints;
	mLen = sdIn.mLen;
	mValueInHist = sdIn.mValueInHist;
	mValueInTime = sdIn.mValueInTime;
	mIsFromSplit = sdIn.mIsFromSplit;
	mCgX = sdIn.mCgX;
	mCgY = sdIn.mCgY;
	mDisFromCorners = sdIn.mDisFromCorners;
	mDisFromCenter = sdIn.mDisFromCenter;
}

ShotData::~ShotData()
{
}

ShotData * ShotData::operator=(ShotData & sdIn)
{
	mPoints = sdIn.mPoints;
	mLen = sdIn.mLen;
	mValueInHist = sdIn.mValueInHist;
	mValueInTime = sdIn.mValueInTime;
	mIsFromSplit = sdIn.mIsFromSplit;
	mCgX = sdIn.mCgX;
	mCgY = sdIn.mCgY;
	mDisFromCorners = sdIn.mDisFromCorners;
	mDisFromCenter = sdIn.mDisFromCenter;
	return this;
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
	
	sort(allP.begin(), allP.end(), ComparePix);
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

int ShotData::Split(vector<ShotData>& sds, int shotMinLen, int shotminRad, Mat* displayMat)
{
	int numOfShots = 0;
	int len = (int)mPoints.size();
	if (len < 5)
		return numOfShots;
	Size sz = displayMat->size();
	vector<pair<Point, float>> allP = mPoints;
	sort(allP.begin(), allP.end(), ComparePix);
	float curMaxVal = allP[0].second;
	int maxNumberOfShots = len / shotMinLen;
	++maxNumberOfShots;
	vector <vector<pair<Point, float>>> histP(maxNumberOfShots);
	while(len>5)
	{
		vector<int> isMarked(len, 0);
		int markedCnt = 0;
		int curBin = 0;

		histP[curBin].push_back(allP[0]);
		isMarked[0] = 1;
		markedCnt++;
		int xOfMax = histP[curBin][0].first.x;
		int yOfMax = histP[curBin][0].first.y;

		for (int l = 1; l < len; ++l)
		{
			int x = allP[l].first.x;
			int y = allP[l].first.y;
			displayMat->at<uchar>(allP[l].first) = 255;
			if (abs(x - xOfMax) <= shotminRad && abs(y - yOfMax) <= shotminRad && isMarked[l] == 0)
			{
				displayMat->at<uchar>(allP[l].first) = markedCnt*10;
				histP[curBin].push_back(allP[l]);
				isMarked[l] = 1;
				markedCnt++;
			}
		}
		auto iter = allP.end();
		auto iterB = allP.begin();
		for (int m = len; m >=0; --m, --iter)
		{
			if (isMarked[m] == 1 && iter >= iterB)
			{
				allP.erase(iter);
			}
		}
		//Add the pixels that have no neighbors UDLR
		len = (int)allP.size();
		isMarked.resize(len, 0);
		for (int l = len-1; l >=0; --l)
		{
			int x = allP[l].first.x;
			int y = allP[l].first.y;
			if (x - 1 >= 0)
			{
				uchar v = displayMat->at<uchar>(y, x - 1);
				if (v == 255)
					continue;
			}
			if (x + 1 < sz.width)
			{
				uchar v = displayMat->at<uchar>(y, x + 1);
				if (v == 255)
					continue;
			}
			if (y - 1 >= 0)
			{
				uchar v = displayMat->at<uchar>(y - 1, x);
				if (v == 255)
					continue;
			}
			if (y + 1 < sz.height)
			{
				uchar v = displayMat->at<uchar>(y + 1, x);
				if (v == 255)
					continue;
			}
			histP[curBin].push_back(allP[l]);
			allP.erase(allP.begin() + l);
		}
		len = (int)allP.size();
		if (histP[curBin].size() < 5)//If we did not find new shot cand, stop the search
			break;
		sds.push_back(ShotData(histP[curBin]));
		++numOfShots;
		++curBin;
	}


	return numOfShots;
}
