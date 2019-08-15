#include "pch.h"
#include "ShotData.h"
#include <iostream>
#include "ContourData.h"
#include "MovmentUtils.h"

typedef  pair<Point, float> PointVal;

bool ComparePix(pair < Point, pair<float, int>> a, pair < Point, pair<float, int>> b)
{
	return a.second.first > b.second.first;
}

bool CompareSpotsBySize(ShotData& a, ShotData& b)
{
	return a.mLen > b.mLen;
}

//https://stackoverflow.com/questions/1257117/a-working-non-recursive-floodfill-algorithm-written-in-c
void FloodfillIter(Mat& vals, Point q, int SEED_COLOR,int lowTol, int highTol, int COLOR, vector<pair<Point, pair<float, int>>>& foundPix, Mat& mask, int COLOR_MASK)
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
			foundPix.push_back(make_pair(p, make_pair(v,-1)));
			
			vals.at<float>(y, x) = COLOR;
			mask.at<float>(y, x) = COLOR_MASK;
			stack.push_back(Point(x + 1, y));
			stack.push_back(Point(x - 1, y));
			stack.push_back(Point(x, y + 1));
			stack.push_back(Point(x, y - 1));
		}
	}
}
//Flood fill that compares to it's neighbers and not to the seed
void FloodfillNeigh(Mat& vals, const Mat& time, int ariveTime, Point q, int SEED_COLOR, int lowTol, int highTol, int COLOR, vector<pair<Point, pair<float, int>>>& foundPix, Mat& mask, int COLOR_MASK)
{
	int h = vals.rows;
	int w = vals.cols;

	if (q.y < 0 || q.y > h - 1 || q.x < 0 || q.x > w - 1)
		return;
	foundPix.resize(0);

	vector<pair<Point,float>> stack;
	PointVal qf;
	qf.first = q;
	qf.second = vals.at<float>(q.y, q.x);
	stack.push_back(qf);
	while ((int)stack.size() > 0)
	{
		PointVal p = stack[(int)stack.size() - 1];
		stack.pop_back();
		int x = p.first.x;
		int y = p.first.y;
		if (y < 0 || y > h - 1 || x < 0 || x > w - 1)
			continue;
		float val = vals.at<float>(y, x);
		int t = time.at<int>(y, x);
		if ((val >= p.second - lowTol && val <= p.second + highTol) || (val > 0 && t-ariveTime<25 ))
		{
			float v = mask.at<float>(y, x);
			foundPix.push_back(make_pair(p.first, make_pair(v, -1)));

			vals.at<float>(y, x) = COLOR;
			mask.at<float>(y, x) = COLOR_MASK;
			stack.push_back(make_pair(Point(x + 1, y), val));
			stack.push_back(make_pair(Point(x - 1, y), val));
			stack.push_back(make_pair(Point(x, y + 1), val));
			stack.push_back(make_pair(Point(x, y - 1), val));
		}
	}
}

//#define _DISPLAY
int LookForShots(Mat& histMat, Mat& timeMat, int thresholdInHist, vector<ShotData>& shots, int isDebugMode)
{
	int numOfShots = 0;
	Mat hist, histThr, histThrL16, histThrH16, histThrL, histThrH, mask, lowHistThr;
	double mn16, mx16;
	minMaxLoc(histMat, &mn16, &mx16);
	int thrLow = (int)(255.0*thresholdInHist / mx16);
	histMat.convertTo(hist, CV_8UC1,255.0/max(1.0, mx16));
	Size sz = histMat.size();

	Mat time, thrTime32, thrTime;
	timeMat.convertTo(time, CV_32FC1);
	//Find all pixels that were ON before frame 100
	int timeToCleanBefore = 50;
	threshold(time, thrTime32, timeToCleanBefore, 255, THRESH_BINARY);
	thrTime32.convertTo(thrTime, CV_8U);
	cv::imshow("timeTime", thrTime);

	Erosion(thrTime, thrTime, 1);
	Dilation(thrTime, thrTime, 1);
	
	cv::imshow("timeThr", thrTime);
	cv::imshow("histMat", hist);

	//Doing 2 thresholds high and low, selecting the one with more contours
	//If its too low then a lot of contours were connected to each other
	//If its too high then a lot of contours disappeared
	histMat.convertTo(histThrL16, CV_32FC1);
	histThrL16.copyTo(histThrH16);
	minMaxLoc(histMat, &mn16, &mx16);
	threshold(histThrL16, histThrL16, thresholdInHist, 255, THRESH_BINARY);
	threshold(histThrH16, histThrH16, (int)floor(1.5*thresholdInHist), 255, THRESH_BINARY);

	//threshold(histMat, histThrL16, 20, 255, THRESH_BINARY);
	//threshold(histMat, histThrH16, 120, 255, THRESH_BINARY);
	histThrL16.convertTo(histThrL, CV_8U);
	histThrH16.convertTo(histThrH, CV_8U);
	cv::imshow("histThrL", histThrL);
	cv::imshow("histThrH", histThrH);
	Mat histThrLclean, histThrHclean;
	histThrL.copyTo(histThrLclean, thrTime);
	histThrH.copyTo(histThrHclean, thrTime);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchyFirst;
	cv::findContours(histThrHclean, contours, hierarchyFirst, RETR_CCOMP, CHAIN_APPROX_NONE);
	int numOfCurContoursHigh = (int)contours.size();

	cv::findContours(histThrLclean, contours, hierarchyFirst, RETR_CCOMP, CHAIN_APPROX_NONE);
	int numOfCurContoursLow = (int)contours.size();
#define WITH_CLEAN
	if (numOfCurContoursHigh > numOfCurContoursLow)
	{
#ifdef WITH_CLEAN
		histThrHclean.copyTo(histThr);
#else
		histThrH.copyTo(histThr);
#endif
	}
	else
	{
#ifdef WITH_CLEAN
		histThrLclean.copyTo(histThr);
#else
		histThrL.copyTo(histThr);
#endif
	}
	cv::imshow("histThr1", histThr);

	histThr.convertTo(histThr, CV_32FC1);
	hist.convertTo(hist, CV_32FC1);

	Mat dispShots;
	if (isDebugMode)
	{
		dispShots = Mat(sz, CV_8U);
		dispShots.setTo(0);
		threshold(hist, lowHistThr, thresholdInHist >> 2, 255, THRESH_BINARY);
	}
	
	
	
	int cnt = 0;
	int tolH = 150; //upper and lower diff to distinguish between tight shots
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
				vector<pair<Point, pair<float, int>>> points;
				FloodfillIter(histThr, Point(c, r), thrval, tolL, tolH, cnt, points, hist, 0);
				ShotData sd(points,timeMat);
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
				//if (sd.mLen > 23)0
				int t = INT_MAX;
				for (int s = 0; s < sd.mLen; ++s)
				{
					if (sd.mPoints[s].second.second < t)
						t = sd.mPoints[s].second.second;
				}
				//Delete shots that appeared before 4 seconds
				if (t > timeToCleanBefore && sd.mLen > 1)
				{
					sd.mValueInTime = t;
					shots.push_back(sd);
				}
			}
		}
	}
	numOfShots = (int)shots.size();
	sort(shots.begin(), shots.end(), CompareSpotsBySize);
	int percentile10 = (int)round(numOfShots*0.5f);
	int shotMinSize = shots[percentile10].mLen;
	int shotDiam = sqrt(shotMinSize);
	int shotMaxSizeAllowed = (int)round(shotMinSize*1.25f);
	//Go over the first 0.5 spots and check if they are too big and needs to go to a split process
	for (int i = 0; i < numOfShots; ++i)
	{
		if (shots[i].mLen < 20)
		{
			break;//Since they are sorted, all of the following will also be small
		}
		//Split the blob to several blobs if shots touch each other
		vector<ShotData> sdsSplit;
		ShotData sd(shots[i]);
		sd.mIsFromSplit = false;
		
		//Do the split
		Mat dispMat(sz, CV_8UC1);
		dispMat.setTo(0);
		//int splitsFound = sd.Split(sdsSplit, shotMinSize, shotDiam, timeMat, thresholdInHist, &dispMat);
		int splitsFound = sd.Split(sdsSplit,timeMat , &dispMat);
		if (splitsFound == 1)
			continue;

		char val = 0;
		sd.mValueInHist = val;
//		sdsSplit.push_back(sd);
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
				if (sdsSplit[indSd].mValueInTime > 4*25)//if this blob was after 3 sec. than add it, else it was at the starting frame
				{
					if (indSd == 0)
						shots[i] = sdsSplit[indSd];
					else
					{
						shots.push_back(sdsSplit[indSd]);
						++numOfShots;
					}
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

ShotData::ShotData(vector<pair<Point, pair<float, int>>> pointsOfShot):ShotData()
{
	mPoints = pointsOfShot;
	mLen = (int)mPoints.size();
	mCgX = mCgY = 0.0f;
	if (mLen == 0)
		return;
	mValueInHist = 0;
	vector<int> xs(mLen,-1), ys(mLen,-1);
	for (int i = 0; i < mLen; ++i)
	{
		mCgX += mPoints[i].first.x;
		mCgY += mPoints[i].first.y;
		xs[i] = mPoints[i].first.x;
		ys[i] = mPoints[i].first.y;
		if (mPoints[i].second.first > mValueInHist)
			mValueInHist = mPoints[i].second.first;
	}
	mCgX /= mLen;
	mCgY /= mLen;
	sort(xs.begin(), xs.end());
	sort(ys.begin(), ys.end());
	//Take the median as the center of gravity
	int len2 = mLen >> 2;
	mCgX = xs[len2];
	mCgY = ys[len2];
}

ShotData::ShotData(vector<pair<Point, pair<float, int>>> pointsOfShot, const Mat& timeMat) :ShotData(pointsOfShot)
{
	for (int i = 0; i < mLen; ++i)
	{
		int t = timeMat.at<int>(mPoints[i].first.y, mPoints[i].first.x);		
		mPoints[i].second.second = t;
	}
}

ShotData::ShotData(vector<Point> points, const Mat& timeMat, const Mat& histMat)
{
	mPoints.resize(points.size());
	mLen = (int)mPoints.size();

	for (int i = 0; i < mLen; ++i)
	{
		mPoints[i].first = points[i];
		int t = timeMat.at<int>(mPoints[i].first.y, mPoints[i].first.x);
		float h=histMat.at<float>(mPoints[i].first.y, mPoints[i].first.x);
		mPoints[i].second.first = h;
		mPoints[i].second.second = t;
	}
}

ShotData::ShotData(const ShotData& sdIn)
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

ShotData& ShotData::operator=(const ShotData & sdIn)
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
	return *this;
}

void DrawBinPoints(vector<pair<Point, pair<float, int>>>& binPoints, Mat& displayMat, uchar color)
{
	int szBin = (int)binPoints.size();
	
	for (int i = 0; i < szBin; ++i)
	{
		displayMat.at<uchar>(binPoints[i].first.y, binPoints[i].first.x) = color;
	}
}

int ShotData::Split(vector<ShotData>& sds, const Mat& timeMat, Mat* displayMat)
{
	int numOfShots = 0;
	int len = (int)mPoints.size();
	if (len < 5)
		return numOfShots;

	Size sz = timeMat.size();
	vector<pair<Point, pair<float, int>>> allP = mPoints;
	
	sort(allP.begin(), allP.end(), ComparePix);
	float curMaxVal = allP[0].second.first;
	int binInterval = 20;
	int numOfBins = (int)(curMaxVal / binInterval);
	if (numOfBins <= 1)
		return numOfShots;
	vector <vector<pair<Point, pair<float, int>>>> histP(numOfBins);
	int curBin = 0;
	Mat spotsMat(sz.height, sz.width, CV_32F);
	spotsMat.setTo(0);

	for (int i = 0; i < len; ++i)
	{
		displayMat->at<uchar>(allP[i].first) = (int)floor(255 * allP[i].second.first / curMaxVal);
		spotsMat.at<float>(allP[i].first) = allP[i].second.first;
	}

	Mat mask;
	Mat spotsMatPrev;
	spotsMat.copyTo(mask);
	vector<pair<Point, pair<float, int>>> points;
	//compute the histogram of the values in the spot, if we will get a large bin than it is a shot
	int diffAllowed = 1;
	for (int l = 0; l < (int)allP.size(); ++l)
	{
		points.resize(0);
		spotsMat.copyTo(spotsMatPrev);
		FloodfillNeigh(spotsMat, timeMat, allP[0].second.second, allP[0].first, (int)allP[l].second.first, diffAllowed, diffAllowed, 0, points, mask, 255);
		//if (allP[0].first.x==361 && allP[0].first.y==177)
		//{
			//Mat spotsMat8;
			//spotsMat.convertTo(spotsMat8, CV_8UC1);
			//circle(spotsMat8, allP[0].first, 3, Scalar(128));
			//cv::imshow("spotsMat", spotsMat8);
			//cv::imshow("displayMat", *displayMat);
			//cv::waitKey();
		//}
		
		int pSz = (int)points.size();
		if (pSz > 6)
		{
			histP[curBin] = points;
			if (curBin < numOfBins - 1)
			{
				//find points in allP, and clear them
				for (int p = 0; p < (int)points.size(); ++p)
				{
					for (int a = (int)allP.size() - 1; a >= 0; --a)
					{
						if (points[p].first == allP[a].first)
						{
							allP.erase(allP.begin() + a);
							break;
						}
					}

				}
				++curBin;
			}
			else if(curBin == numOfBins - 1)
				break;
		}
		else if (pSz <= 6 && (int)allP.size() > 6 && diffAllowed > 1)
		{
			spotsMatPrev.copyTo(spotsMat);
			++diffAllowed;
		}
		else
			break;
	}
	/* CHECK the OUTPUT*/
	int numOfCands = curBin;
	for (int s = 0; s < numOfCands; ++s)
	{
		ShotData sd(histP[s]);
		sds.push_back(sd);
	}
	numOfShots = (int)sds.size();
	return numOfShots;
}

//int ShotData::Split(vector<ShotData>& sds, int shotMinLen, int shotminDiam, Mat* displayMat)
//{
//	int numOfShots = 0;
//	int len = (int)mPoints.size();
//	if (len < 5)
//		return numOfShots;
//	Size sz = displayMat->size();
//	vector<pair<Point, float>> allP = mPoints;
//	
//	sort(allP.begin(), allP.end(), ComparePix);
//	float curMaxVal = allP[0].second;
//	int maxNumberOfShots = min(10,max(2, (int)floor(len / (2 * shotMinLen))));
//	
//	vector <vector<pair<Point, float>>> histP(maxNumberOfShots);
//	Mat samples(len, 3, CV_32F), labels, centers;
//	//Normelize x y and hist val so they will have the same influance on the k-means
//	float mnx = sz.width, mny = sz.height, mxx = 1, mxy = 1, mnval = FLT_MAX, mxval = 1;
//	for (int i = 0; i < len; ++i)
//	{
//		if (allP[i].first.x < mnx)
//			mnx = allP[i].first.x;
//		if (allP[i].first.x > mxx)
//			mxx = allP[i].first.x;
//
//		if (allP[i].first.y < mny)
//			mny = allP[i].first.y;
//		if (allP[i].first.x > mxx)
//			mxy = allP[i].first.y;
//
//		if (allP[i].second < mnval)
//			mnval = allP[i].second;
//		if (allP[i].second > mxval)
//			mxval = allP[i].second;
//	}
//
//	for (int i = 0; i < len; ++i)
//	{
//		samples.at<float>(i, 0) = (allP[i].first.x - mnx) / mxx;
//		samples.at<float>(i, 1) = (allP[i].first.y - mny) / mxy;
//		samples.at<float>(i, 2) = 20*(allP[i].second - mnval) / mxval;
//	}
//	int attempts = 3;
//	TermCriteria tc;// = TermCriteria(TermCriteria::CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.0001);
//	tc.type = TermCriteria::Type::EPS | TermCriteria::Type::MAX_ITER;
//	tc.maxCount = 10000;
//	tc.epsilon = 0.001;
//	kmeans(samples, maxNumberOfShots, labels,tc, attempts, KMEANS_PP_CENTERS, centers);
//	vector<float> lblVec;
//	for (int i = 0; i < len; ++i)
//	{
//		int lbl = labels.at<int>(i, 0) + 1;
//		displayMat->at<uchar>(allP[i].first) = lbl;// *(int)floor(255 / maxNumberOfShots);
//		lblVec.push_back(lbl);
//	}
//
//	for (int l = 0; l < len; ++l)
//	{
//		int x = allP[l].first.x;
//		int y = allP[l].first.y;
//		int lbl = lblVec[l];
//		//p1x=plus 1 x, m1x=minus 1 x
//		uchar p1x=255, m1x=255, p1y=255, m1y=255;
//		int numOfVoidNgbrs = 0;
//		if (x - 1 >= 0)
//		{
//			m1x = displayMat->at<uchar>(y, x - 1);
//			if (m1x == lbl)
//				++numOfVoidNgbrs;
//		}
//		if (x + 1 < sz.width)
//		{
//			p1x = displayMat->at<uchar>(y, x + 1);
//			if (p1x == lbl)
//				++numOfVoidNgbrs;
//		}
//		if (y - 1 >= 0)
//		{
//			m1y = displayMat->at<uchar>(y - 1, x);
//			if (m1y == lbl)
//				++numOfVoidNgbrs;
//		}
//		if (y + 1 < sz.height)
//		{
//			p1y = displayMat->at<uchar>(y + 1, x);
//			if (p1y == lbl)
//				++numOfVoidNgbrs;
//		}
//		if (numOfVoidNgbrs > 1)
//		{
//			//displayMat->at<uchar>(allP[l].first) = (curBin + 1) * 10;
//			histP[lbl-1].push_back(allP[l]);
//		}
//	}
//	int shotMinLen2 = shotMinLen >> 1;
//	for (int i = 0; i < maxNumberOfShots; ++i)
//	{
//		if ((int)histP[i].size() > shotMinLen2)
//		{
//			sds.push_back(ShotData(histP[i]));
//			++numOfShots;
//		}
//	}
//	*displayMat = (*displayMat) *(int)floor(255 / maxNumberOfShots);
//	imshow("displayMat", *displayMat);
//	waitKey();
//	return numOfShots;
//}

int ShotData::Split(vector<ShotData>& sds, int shotMinLen, int shotminRad, const Mat& timeMat, int thresholdInHist, Mat* displayMat)
{
	int numOfShots = 0;
	int len = (int)mPoints.size();
	if (len < 5)
		return numOfShots;
	//sds.push_back(ShotData(mPoints));
	//return 1;
	Size sz = displayMat->size();
	Mat spotsMat(sz.height, sz.width, CV_32F);
	spotsMat.setTo(0);
	vector<pair<Point, pair<float, int>>> allP = mPoints;
	sort(allP.begin(), allP.end(), ComparePix);
	float curMaxVal = allP[0].second.first;
	int maxNumberOfShots = (int)ceil(len / (float)shotMinLen);
	++maxNumberOfShots;
	vector <vector<pair<Point, pair<float, int>>>> histP(maxNumberOfShots);
	for (int i = 0; i < len; ++i)
	{
		displayMat->at<uchar>(allP[i].first) = (int)floor(255 * allP[i].second.first / curMaxVal);
		spotsMat.at<float>(allP[i].first) = allP[i].second.first;
	}
	Mat blobsMat;

	vector<ContourData> cntrData;
	vector<vector<Point> > contoursThr;
	vector<Vec4i> hierarchyThr;
	//Go from thr=max until number of contours starts to decrease. At this point the shots start to join.
	//Go back one step and take the blobs to further analyse.
	Mat spotsMat8;
	Mat blobsMat8(sz.height, sz.width, CV_8UC1);
	Mat blobsPrev;
	spotsMat.convertTo(spotsMat8, CV_8UC1, 255 / curMaxVal);
	int curMaxVal8 = 255;
	int thresholdInHist8 = 255*thresholdInHist / (float)curMaxVal;
	int numOfCurContours = 1;
	int numOfContours = 0;
	int prevArea = 0;
	int curArea = 1;
	for (int thr = curMaxVal8 -1; thr > thresholdInHist8; --thr)
	{
		blobsMat8.copyTo(blobsPrev);
		threshold(spotsMat8, blobsMat8, (double)thr, 255, THRESH_BINARY);
		prevArea = curArea;
		curArea = countNonZero(blobsMat8);
		//blobsMat.convertTo(blobsMat8, CV_8UC1);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchyFirst;
		cv::findContours(blobsMat8, contours, hierarchyFirst, RETR_CCOMP, CHAIN_APPROX_NONE);
		int numOfCurContours = (int)contours.size();
		if (thr < curMaxVal8 - 1 && (numOfCurContours > 1 || curArea>1.25*prevArea))
		{
			cv::imshow("spotsMat", spotsMat8);
			cv::imshow("blobsMat", blobsMat8);
			cv::imshow("blobsMatPrev", blobsPrev);
			cv::waitKey();
		}
		if (numOfCurContours < numOfContours)
		{
			++thr;
			threshold(spotsMat8, blobsMat8, (double)thr, 255, THRESH_BINARY);
			//blobsMat.convertTo(blobsMat8, CV_8UC1);
			cv::findContours(blobsMat8, contoursThr, hierarchyThr, RETR_CCOMP, CHAIN_APPROX_NONE);
			numOfContours = (int)contours.size();
			cv::imshow("spotsMat", spotsMat8);
			cv::imshow("blobsMat", blobsMat8);
			cv::waitKey();
			break;
		}
	}
	Mat maskMat(sz.height, sz.width, CV_8UC1);
	blobsMat8.copyTo(maskMat);
	vector< ContourData> cds(numOfContours);
	for (int i = 0; i < numOfContours; ++i)
	{
		ContourData cd(contoursThr[i], sz);
		cds.push_back(cd);
		auto pt = cd.mContour[0];
		vector<pair<Point, pair<float, int>>> points;
		FloodfillIter(blobsMat8, pt, 255, 255, 255, 128, points, maskMat, 0);
		vector<pair<Point, pair<float, int>>> pointsOfShot;
		ShotData sd(points, timeMat);
		sds.push_back(sd);
		++numOfShots;
	}

	return numOfShots;
}

bool ShotData::IsItShot()
{
	bool res = false;
	int minLen = 23;
	bool isMaxValuesShapeWide = true;
	return res;
}
