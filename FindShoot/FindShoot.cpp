// FindShoot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/video.hpp"
#include <Windows.h>
#include <sys/stat.h>
#include "ShootTargetMetaData.h"
#include "MovmentUtils.h"
#include "ContourData.h"

using namespace std;
using namespace cv;

void MarkRect_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN) //EVENT_MOUSEMOVE
	{
		cout << "(" << x << ", " << y << ")" << endl;

		int dis[5];
		int minDis = INT16_MAX;
		int minDisInd = 0;
		ShootTargetMetaData* md = (ShootTargetMetaData*)param;
		int dx, dy;
		for (int i = 0; i < 4; ++i)
		{
			dx = x - md->mPoints[i].x;
			dy = y - md->mPoints[i].y;
			dis[i] = dx * dx + dy * dy;
			if (minDis > dis[i])
			{
				minDis = dis[i];
				minDisInd = i;
			}
		}
		dx = x - md->mCenter.x;
		dy = y - md->mCenter.y;
		dis[4] = dx * dx + dy * dy;
		if (minDis > dis[4])
		{
			minDis = dis[4];
			minDisInd = 4;
		}
		if (minDisInd < 4)
		{
			md->mPoints[minDisInd].x = x;
			md->mPoints[minDisInd].y = y;
		}
		else
		{
			md->mCenter.x = x;
			md->mCenter.y = y;
		}
		md->DisplayTarget();
		cv::imshow(md->mWindowName, md->mDrawMat);
	}
}

void MarkRectDrag_callback(int  event, int  x, int  y, int  flag, void *param)
{
	static bool isToDraw = false;
	static int closePoint = -1;
	ShootTargetMetaData* md = (ShootTargetMetaData*)param;
	if (event == EVENT_LBUTTONDOWN)
	{
		md->DisplayTarget();
		cv::imshow(md->mWindowName, md->mDrawMat);
		isToDraw = true;
		cout << "(" << x << ", " << y << ")" << endl;
	}
	else if (event == EVENT_LBUTTONUP)
	{
		isToDraw = false;
		closePoint = -1;
	}
	else if (event == EVENT_MOUSEMOVE && isToDraw)
	{
		cout << "(" << x << ", " << y << ")" << endl;
		int dis[5];
		int minDis = INT16_MAX;
		
		if (closePoint == -1)
		{
			int dx, dy;
			for (int i = 0; i < 4; ++i)
			{
				dx = x - md->mPoints[i].x;
				dy = y - md->mPoints[i].y;
				dis[i] = dx * dx + dy * dy;
				if (minDis > dis[i] && dis[i] < 10)
				{
					minDis = dis[i];
					closePoint = i;
				}
			}
			dx = x - md->mCenter.x;
			dy = y - md->mCenter.y;
			dis[4] = dx * dx + dy * dy;
			if (minDis > dis[4] && dis[4] < 10)
			{
				minDis = dis[4];
				closePoint = 4;
			}
		}
		else if (closePoint > -1)
		{
			if (closePoint < 4)
			{
				md->mPoints[closePoint].x = x;
				md->mPoints[closePoint].y = y;
			}
			else
			{
				md->mCenter.x = x;
				md->mCenter.y = y;
			}
			md->DisplayTarget();
			cv::imshow(md->mWindowName, md->mDrawMat);
		}
	}
}

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN) 
	{
		cout << "(" << x << ", " << y << ")";
		if (param)
		{
			Mat* m = (Mat*)param;
			int chNum = m->channels();
			if (chNum == 3)
			{
				Vec3b bgrPixel = ((Mat*)param)->at<Vec3b>(y, x);
				cout << ": (" << bgrPixel[0] << "," << bgrPixel[1] << "," << bgrPixel[2] << ")" << endl;
			}
			else if (chNum == 1)
			{
				uchar pixel = ((Mat*)param)->at<uchar>(y, x);
				cout << ": " << (int)pixel << endl;
			}
		}
	}
}

int main()
{
	using namespace cv;
	bool toDisplay = false;
	String dirName = "C:/moti/FindShoot/";
	String fName = "MVI_3";
	String extName = ".MOV";
	//String fName = "VID-20181125-WA0005";
	//String extName = ".mp4";
	String fullFileName = dirName + fName + extName;
	
	struct stat buffer;
	if (stat(fullFileName.c_str(), &buffer) != 0)
	{
		cout << fullFileName << " not found!" << endl;
	}
	VideoCapture cap(fullFileName);
	// Check if camera opened successfully
	if(!cap.isOpened())
	{
		cout << "Error opening video stream or file" << endl;
		return -1;
	}
	int rot = (int)cap.get(cv::CAP_PROP_MODE);
 
	Mat frame, prevFrame, frameDiff, firstFrame;
	Mat Transform;
	Mat Transform_avg = Mat::eye(2, 3, CV_64FC1);
	Mat warped;
	int cntFrameNum = 0;
	int STARTFRAME = 60;
	for (; cntFrameNum < STARTFRAME; ++cntFrameNum)
	{
		cap >> frame;
	}
	
	Size sz = frame.size();
	if (sz.width > 800)
	{
		sz.height = cvRound(sz.height*(740.0f / sz.width));
		sz.width = 740;		
	}
	Mat smallFrame(sz.height, sz.width, frame.type());
	resize(frame, smallFrame, sz, 0, 0);
	if (rot != 0)
	{
		//cvtColor(firstFrame, firstFrame, CV_BG);
		transpose(smallFrame, smallFrame);
	}
	ShootTargetMetaData metaData;
	
	String mdFileName = dirName + fName + ".txt";
	if (stat(mdFileName.c_str(), &buffer) != 0)
	{
		int margins = 10;
		metaData.mPoints[0].x = margins;
		metaData.mPoints[0].y = margins;
		metaData.mPoints[1].x = sz.width - margins;
		metaData.mPoints[1].y = margins;
		metaData.mPoints[2].x = sz.width - margins;
		metaData.mPoints[2].y = sz.height - margins;
		metaData.mPoints[3].x = margins;
		metaData.mPoints[3].y = sz.height - margins;

		metaData.mCenter.x = sz.width >> 1;
		metaData.mCenter.y = sz.height >> 1;

		metaData.mOrgMat = smallFrame;
		metaData.mWindowName = "EnterPositions";
		metaData.mRectColor = Scalar(25, 255, 0);
		metaData.mCenterColor = Scalar(0, 0, 255);
		
		imshow("EnterPositions", metaData.mOrgMat);
		cv::setMouseCallback("EnterPositions", MarkRectDrag_callback, &metaData);
		int k = cv::waitKey();
		metaData.ToFile(mdFileName);
	}
	else
	{
		metaData.FromFile(mdFileName);
		cout << "Target:"<<endl;
		for(Point p: metaData.mPoints)
		{ 
			cout << "(" << p.x << "," << p.y << ")" << endl;
		}
		cout << "Center: (" << metaData.mCenter.x << "," << metaData.mCenter.y << ")"<<endl;
	}

	Mat map(sz.height, sz.width, CV_8UC1);
	map.setTo(255);
	drawPolyRect(map, metaData.mPoints, Scalar(0), -1);

	Mat bgr[3];
	split(smallFrame, bgr);
	int selectedCh = 0;
	int maxDiff = 0;
	double mn, mx;

	cvtColor(smallFrame, firstFrame, COLOR_BGR2GRAY);
	drawPolyRect(smallFrame, metaData.mPoints, Scalar(255, 0, 17), 1);
	//cv::imshow("TargetOnFrame", smallFrame);
	//cv::setMouseCallback("TargetOnFrame", mouse_callback);
	cv::equalizeHist(firstFrame, firstFrame);
	firstFrame.copyTo(smallFrame);
	
	Rect rctMargin;
	int look = 15;
	rctMargin.x = look;
	rctMargin.y = look;
	rctMargin.width = sz.width - 2 * look;
	rctMargin.height = sz.height - 2 * look;
	Mat firstFrameMar(firstFrame, rctMargin);
	Mat target(sz.height, sz.width, CV_8UC1);
	
	Rect rectInBound = FindInboundRect(rctMargin, metaData.mPoints);
	
	drawPolyRect(target, metaData.mPoints, Scalar(0), -1);

	Mat firstGrad;

	Mat mapNot;
	bitwise_not(map, mapNot);

	double ffMin=0, ffMax=0;
	int ffMinIdx=0, ffMaxIdx=0;
	smallFrame.copyTo(firstFrame);

	minMaxIdx(firstFrame, &ffMin, &ffMax,&ffMinIdx,&ffMaxIdx,mapNot);
	int bg = cvFloor((ffMin+ffMax)*0.5);
	cv::threshold(target, target, 1, 255, THRESH_BINARY);
	
	Mat shiftMap = map(Rect(1, 1, sz.width - 1, sz.height - 1));

	rectangle(target, rectInBound, Scalar(100));
	Mat shot = map.clone();
	Mat mapMar(map, rctMargin);
	Mat targetMar(target, rctMargin);
	int thrOfGrad = 50;
	int maxGrayLevelAllowed = 150;
//#undef min	cv::min(firstFrame, maxGrayLevelAllowed, firstFrame);
	Canny(firstFrame, firstGrad, thrOfGrad, 2 * thrOfGrad);
	firstGrad.setTo(0, map);
	cv::imshow("firstFrame", firstFrame);
	cv::imshow("firstGrad", firstGrad);
	cv::waitKey();
	//cv::threshold(firstGrad, firstGrad, thrOfGrad, 255, THRESH_BINARY);
	//Map all the contours of the first frame
	vector<vector<Point> > contoursFirst;
	vector<Vec4i> hierarchyFirst;
	cv::findContours(firstGrad, contoursFirst, RETR_LIST, CHAIN_APPROX_NONE);
	//Leave only large contours
	int numOfFirstContours = (int)contoursFirst.size();
	int idxMaxFirst = -1;
	int rectAreaMaxFirst = 0;
	int idxOfLargeInTheFirstArray = -1;
	Point2f pntCgMaxFirst(0, 0);
	vector<ContourData> cntrDataFirst;

	if (numOfFirstContours > 0)
	{
		int idx = 0;
		double arMax = 0;
		for (; idx < contoursFirst.size(); idx++)
		{
			ContourData cd(contoursFirst[idx], sz,cntFrameNum,idx);
			if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
				continue;
			char buf[256] = { '\0' };
			sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
				cntFrameNum, idx, numOfFirstContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
			OutputDebugStringA(buf);
			if (cd.mLen > rectAreaMaxFirst)
			{
				rectAreaMaxFirst = cd.mLen;
				idxMaxFirst = idx;
				pntCgMaxFirst = cd.mCg;
				idxOfLargeInTheFirstArray = (int)cntrDataFirst.size();
			}
			//if (cntFrameNum == 160)
			//{
			//	shot.setTo(0);
			//	drawContours(shot, contoursFirst, idx, 255, 0, 8, hierarchyFirst);
			//	cv::setMouseCallback("shot", mouse_callback, &metaData);
			//	cv::imshow("cntr", shot);
			//	cv::waitKey();
			//}

			//if (cd.mRatioFromAll < 0.0001)//too small delete it
			//{
			//	contoursFirst.erase(contoursFirst.begin() + idx);
			//	--idx;
			//}
			//else
			{
				cntrDataFirst.push_back(cd);
			}
		}
	}
	int numOfInitCntr = (int)contoursFirst.size();
	for (int i = 0; i < numOfInitCntr; ++i)
	{
		cntrDataFirst[i].SetDistFromLargeCorners(cntrDataFirst[idxOfLargeInTheFirstArray].mCorners);
		cntrDataFirst[i].SetDistFromLargeCenter(cntrDataFirst[idxOfLargeInTheFirstArray].mCg);
	}

	Mat grad8Thr;
	Mat firstGradMar, grag8ThrMar;
	firstGradMar = firstGrad(rctMargin);
	Mat frameRgb;
	Mat frameRgbDisplayed;
	cv::imshow("gradFirst", firstGrad);
	shot.setTo(0);
	vector<Rect> foundShotRects;
	vector<Scalar> colors;
	vector<ContourData> shotsCand;
	bool isToBreak = false;
	int sumX = 0, sumY = 0;
	while(1)
	{		
		// Capture frame-by-frame
		cap >> frame;		
		cntFrameNum++;
		cout << cntFrameNum;
		if (cntFrameNum % 10 != 0)
			cout << endl;
		//auto res=cap.retrieve(frame, cntFrameNum);
		//cntFrameNum += 10;

		if (frame.size().height == 0)
			break;
		//if (cntFrameNum % 10 != 0)
		//	continue;
		
		resize(frame, frameRgb, sz);
		cvtColor(frame, frame, COLOR_BGR2GRAY);
		smallFrame.copyTo(prevFrame);
		resize(frame, smallFrame, sz);
		if (rot != 0)
		{
			transpose(smallFrame, smallFrame);
		}
			
		if (cntFrameNum == 0)
			continue;
		// If the frame is empty, break immediately
		if (smallFrame.empty())
			break;
		int fltrSz = 3;
		cv::equalizeHist(smallFrame, smallFrame);
		blur(smallFrame, smallFrame, Size(fltrSz, fltrSz));
		Canny(smallFrame, grad8Thr, thrOfGrad, 2 * thrOfGrad);
		grad8Thr.setTo(0, map);
		int x = 0, y = 0;
		bool isToDisplay = false;
		//if (cntFrameNum == 738)	isToDisplay = true;
		Rect movRct(rctMargin);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		cv::findContours(grad8Thr, contours, RETR_LIST, CHAIN_APPROX_NONE);
		int cntrSz = (int)contours.size();
		char buf[256] = { '\0' };
		sprintf_s(buf, "FindShot: F=%d move x=%d y=%d. Found cntr=%d\n", cntFrameNum,x,y,cntrSz);
		OutputDebugStringA(buf);
/*********/
		int numOfContours = (int)contours.size();
		int idxMax = -1;
		int rectAreaMax = 0;
		Point2f pntCgMax(0, 0);
		if (numOfContours > 0)
		{
			int idx = 0;
			double arMax = 0;
			vector<ContourData> cdsFrame;
			int idxOfLargeInTheArray = -1;
			for (; idx < contours.size(); idx++)
			{
				ContourData cd(contours[idx], sz);
				if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
					continue;
				//shot.setTo(0);
				//polylines(shot, cd.mContour, true, 255, 1, 8);
				//cv::setMouseCallback("shot", mouse_callback, &metaData);
				//cv::imshow("cntr", shot);
				//cv::imshow("grad", grad8Thr);
				//cv::waitKey();
				//char buf[256] = { '\0' };
				//sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
				//	cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
				//OutputDebugStringA(buf);
				if (cd.mLen > rectAreaMax)
				{
					rectAreaMax = cd.mLen;
					idxMax = idx;
					pntCgMax = cd.mCg;
					idxOfLargeInTheArray = (int)cdsFrame.size();
				}
				if (cd.mLen < 10 /*|| (cd.mShRct.width > 25 && cd.mShRct.height > 25)*/)//too small delete it
				{
					contours.erase(contours.begin() + idx);
					--idx;
					numOfContours--;
				}
				else
					cdsFrame.push_back(cd);
			}
			/*If a n out of focus frame than the contours will break, skip these frames*/
			if (rectAreaMax / (float)rectAreaMaxFirst < 0.65)
			{
				//cout <<"FindShot:*****Skipping "<< cntFrameNum<<" area "<< rectAreaMax / (float)rectAreaMaxFirst << endl;
				//cv::imshow("gradThr", grad8Thr);
				//cv::imshow("skipping", frameRgb);
				//cv::waitKey();
				continue;
			}
			/*This is the movment between the frames*/
			//pntCgMax = cdsFrame[idxOfLargeInTheArray].mCg;
			x = cdsFrame[idxOfLargeInTheArray].mShRct.x - cntrDataFirst[idxOfLargeInTheFirstArray].mShRct.x;
			y = cdsFrame[idxOfLargeInTheArray].mShRct.y - cntrDataFirst[idxOfLargeInTheFirstArray].mShRct.y;
			if (abs(x) > 10 || abs(y) > 10)
			{
				continue;
			}
			//x = pntCgMax.x - pntCgMaxFirst.x;
			//y = pntCgMax.y - pntCgMaxFirst.y;
			Point pntMov(x, y);
			for (idx=0; idx < (int)cdsFrame.size(); idx++)
			{
				cdsFrame[idx].mFrameNum = cntFrameNum;
				cdsFrame[idx].mIdxCntr = idx;
				cdsFrame[idx].SetDistFromLargeCenter(pntCgMax);
				ContourData cd = cdsFrame[idx];
				if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
					continue;

				char buf[256] = { '\0' };
				sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
					cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
				OutputDebugStringA(buf);
				if (cntFrameNum == 134)// && idx == 2)// && idxFirst == 7)
				{
					shot.setTo(0);
					polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
					polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
					polylines(shot, cd.mContour, true, 255, 1, 8);
					circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
					circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
					cv::imshow("cntrIn", shot);
					cv::waitKey();
				}
				if ((cd.mAr > arMax) &&
					(cd.mAr > 10 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 2 && cd.mShRct.height > 2 && cd.mRatioWh > 0.54) ||
					(cd.mAr > 25 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height > 4 && cd.mRatioWh > 0.45) ||
					(cd.mAr >= 3 && cd.mShRct.width < 20 && cd.mShRct.height < 20 && cd.mShRct.width > 4 && cd.mShRct.height >= 4 && cd.mRatioWh > 0.79))
				{
					/*Need to go over the first contours compare its cg and MatchShape and see if this one is new, if yes add it to the list*/
					
/*************************/
					bool isFound = false;
					int idxFirst = 0;
					for (; idxFirst < (int)cntrDataFirst.size(); idxFirst++)
					{	
						ContourData cdf = cntrDataFirst[idxFirst];
						//shot.setTo(0);
						//polylines(shot, cd.mContour, true, 255, 1, 8);
						//polylines(shot, cdf.mContour, true, 128, 1, 8);
						//cv::setMouseCallback("shot", mouse_callback, &metaData);
						//cv::imshow("cntr", shot);
						//cv::imshow("grad", grad8Thr);
						//cv::waitKey();
						//cout << idxFirst << endl;
						if (cntFrameNum == -134)// && idx == 2)// && idxFirst == 7)
						{
							shot.setTo(0);
							polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
							polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
							polylines(shot, cd.mContour, true, 255, 1, 8);
							polylines(shot, cdf.mContour, true, 128, 1, 8);
							circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
							circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
							cv::imshow("cntrIn", shot);
							cv::waitKey();
						}
					
						if (cd==cdf)
						{
							isFound = true;
							cntrDataFirst.push_back(cd + pntMov);
							break;
						}
					}
					if (!isFound)
					{						
						cd = cd + pntMov;
						shotsCand.push_back(cd);
						cntrDataFirst.push_back(cd);
						shot.setTo(0);
						for (int i=0; i < shotsCand.size(); i++)
						{
							polylines(shot, shotsCand[i].mContour, true, 255, 1, 8);
						}
						char buf[256] = { '\0' };
						sprintf_s(buf, "FindShot: New F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
							cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
						OutputDebugStringA(buf);						
						cv::imshow("gradThr", grad8Thr);
						cv::imshow("Frame", smallFrame);
						cv::imshow("cntr", shot);
						cv::waitKey();
						cntrDataFirst.push_back(cd);
					}
/*************************/
					
				}
			}
			/*
			Find shotsCands in the current frame, if they does not exists, remove them.
			*/
			for (int idxFirst = 0; idxFirst < (int)shotsCand.size(); idxFirst++)
			{
				ContourData cdf = shotsCand[idxFirst];
				bool isFoundShot = false;

				{
					char buf[256] = { '\0' };
					sprintf_s(buf, "FindShot: %d\n",
						idxFirst);
					OutputDebugStringA(buf);
				}
				for (idx = 0; idx < (int)cdsFrame.size(); idx++)
				{
					ContourData cdToFindShot = cdsFrame[idx];
					if (cdToFindShot.mShRct.width == 0 || cdToFindShot.mShRct.height == 0)
						continue;

					if ((cdToFindShot.mAr > arMax) &&
						(cdToFindShot.mAr > 10 && cdToFindShot.mShRct.width < 20 && cdToFindShot.mShRct.height < 20 && cdToFindShot.mShRct.width > 2 && cdToFindShot.mShRct.height > 2 && cdToFindShot.mRatioWh > 0.54) ||
						(cdToFindShot.mAr > 25 && cdToFindShot.mShRct.width < 20 && cdToFindShot.mShRct.height < 20 && cdToFindShot.mShRct.width > 4 && cdToFindShot.mShRct.height > 4 && cdToFindShot.mRatioWh > 0.45) ||
						(cdToFindShot.mAr >= 3 && cdToFindShot.mShRct.width < 20 && cdToFindShot.mShRct.height < 20 && cdToFindShot.mShRct.width > 4 && cdToFindShot.mShRct.height >= 4 && cdToFindShot.mRatioWh > 0.7))
					{

						if (cntFrameNum == -128 )//&& /*idx == 2 &&*/ idxFirst == 0)
						{
							cout << idxFirst << endl;
							shot.setTo(0);
							polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
							polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
							polylines(shot, cdToFindShot.mContour, true, 255, 1, 8);
							polylines(shot, cdf.mContour, true, 128, 1, 8);
							circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
							circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
							cv::imshow("cntrShots", shot);
							cv::waitKey();
						}

						if (cdToFindShot == cdf)
						{
							isFoundShot = true;
							break;
						}
					}
				}
				if (!isFoundShot)
				{
					shotsCand.erase(shotsCand.begin() + idxFirst);
					if (idxFirst > 0)
					{
						--idxFirst;
					}
				}
			}
		}

/********/
		int numOfShotsFound = (int)shotsCand.size();
		for (int rc = 0; rc < numOfShotsFound-1; ++rc)
		{
			Rect rct = shotsCand[rc].mShRct;
			rct.x += x;
			rct.y += y;
			//rectangle(frameRgb, rct, colors[rc], 2);
			rectangle(frameRgb, rct, Scalar(0,255,0), 2);
		}

		if (numOfShotsFound > 0)
			rectangle(frameRgb, shotsCand[numOfShotsFound-1].mShRct, Scalar(255, 0, 0), 2);
		frameRgb.copyTo(frameRgbDisplayed);
		cv::imshow("SHOTS", frameRgbDisplayed);
		cv::imshow("firstFrame", firstFrame);
		cv::imshow("mapMar", mapMar);
		cv::imshow("Frame", smallFrame);
		cv::imshow("PrevFrame", prevFrame);
		cv::imshow("gradThr", grad8Thr);
		cv::imshow("gradFirst", firstGrad);
		cv::setMouseCallback("gradThr", mouse_callback, (void*)&grad8Thr);
		// Press  ESC on keyboard to exit
		if (0)//cntFrameNum > 680)//isToBreak)//
		{
			isToBreak = false;
			char c = (char)cv::waitKey();
			if (c == 27)
				break;
		}
		else
			waitKey(25);
	}
	//destroyAllWindows();
	cv::imshow("SHOTS", frameRgbDisplayed);
	cv::waitKey();
	cv::destroyAllWindows();
  // When everything done, release the video capture object
	cap.release();
    return 0;
}
