// FindShoot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <fstream>
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
	//String fName = "MVI_3";	String extName = ".MOV";
	String fName = "MVI_4"; String extName = ".MOV";
	//String fName = "MVI_1"; String extName = ".MOV";
	//String fName = "MVI_2"; String extName = ".MOV";
	//String fName = "VID-20181125-WA0005"; String extName = ".mp4"; very bad video

	ofstream fout;
	fout.open(dirName + fName + ".csv");
	
	String fullFileName = dirName + fName + extName;
	
	struct stat buffer;
	if (stat(fullFileName.c_str(), &buffer) != 0)
	{
		cout << fullFileName << " not found!" << endl;
		fout << fullFileName << " not found!" << endl;
	}
	VideoCapture cap(fullFileName);
	// Check if camera opened successfully
	if(!cap.isOpened())
	{
		cout << "Error opening video stream or file" << endl;
		fout << "Error opening video stream or file" << endl;
		fout.close();
		return -1;
	}
	int rot = (int)cap.get(cv::CAP_PROP_MODE);
 
	Mat frame, prevFrame, frameDiff, firstFrame;
	Mat sumFrame, tempSumFrame;
	Mat smallFrame, firstFrameThrs;
	Mat Transform;
	Mat Transform_avg = Mat::eye(2, 3, CV_64FC1);
	Mat warped;
	int cntFrameNum = 0;
	int frameNumAcc = 0;
	int STARTFRAME = 60;
	Size sz(-1, -1);
	for (; cntFrameNum < STARTFRAME; ++cntFrameNum)
	{
		cout << cntFrameNum << endl;
		cap >> frame;
		if (sz.width == -1)
		{
			sz = frame.size();
			if (sz.width != 740)
			{
				sz.height = cvRound(sz.height*(740.0f / sz.width));
				sz.width = 740;
			}
			sumFrame = Mat(sz, CV_16UC3);
			smallFrame = Mat(sz.height, sz.width, frame.type());
			firstFrameThrs = Mat(sz.height, sz.width, CV_8UC1);
			firstFrameThrs.setTo(0);
			tempSumFrame = Mat(sz.height, sz.width, CV_8UC1);
			tempSumFrame.setTo(0);
		}
		else if(cntFrameNum > STARTFRAME-10)
		{
			frameNumAcc++;
			resize(frame, smallFrame, sz, 0, 0);
			cvtColor(smallFrame, firstFrame, COLOR_BGR2GRAY);
			adaptiveThreshold(firstFrame, tempSumFrame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, (((int)floor(1.1*sz.width)) | 1), 0);
			firstFrameThrs = firstFrameThrs | tempSumFrame;
		}
	}
	bitwise_not(firstFrameThrs, firstFrameThrs);
	bitwise_not(tempSumFrame, tempSumFrame);	
	
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
	int fltrSz = 3;
	cv::equalizeHist(firstFrame, firstFrame);
	blur(firstFrame, firstFrame, Size(fltrSz, fltrSz));
	firstFrame.copyTo(smallFrame);
	Rect rctMargin;
	int look = 20;
	rctMargin.x = look;
	rctMargin.y = look;
	rctMargin.width = sz.width - 2 * look;
	rctMargin.height = sz.height - 2 * look;
	Mat target(sz.height, sz.width, CV_8UC1);
	Rect rectInBound = FindInboundRect(rctMargin, metaData.mPoints);
	cout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
	fout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
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
	
	//Mat shiftMap = map(Rect(1, 1, sz.width - 1, sz.height - 1));
	
	rectangle(target, rectInBound, Scalar(100));
	
	Mat shot = map.clone();
	
	int thrOfGrad = 77;
	double thr = 128 + 7;
	int maxGrayLevelAllowed = 150;
	
//#undef min	cv::min(firstFrame, maxGrayLevelAllowed, firstFrame);
	Canny(firstFrameThrs, firstGrad, thrOfGrad, 2.75 * thrOfGrad);
	firstGrad.setTo(0, map);
	Mat shotsHistogramMat(sz.height, sz.width, CV_16UC1);
	shotsHistogramMat.setTo(1);
	shotsHistogramMat.setTo(0, map);
	//cv::imshow("firstFrame", firstFrame);
	//cv::imshow("firstGrad", firstGrad);
	//cv::waitKey();
	//cv::threshold(firstGrad, firstGrad, thrOfGrad, 255, THRESH_BINARY);
	//Map all the contours of the first frame
	vector<vector<Point> > contoursFirst;
	vector<Vec4i> hierarchyFirst;
	cv::findContours(firstGrad, contoursFirst,hierarchyFirst, RETR_CCOMP, CHAIN_APPROX_NONE);
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
		//for (; idx >= 0; idx = hierarchyFirst[idx][0])
		while(idx >= 0)
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
			if (idx < 0)
				break;
			idx = hierarchyFirst[idx][0];
		}
	}
	int numOfInitCntr = (int)cntrDataFirst.size();
	for (int i = 0; i < numOfInitCntr; ++i)
	{
		cntrDataFirst[i].SetDistFromLargeCorners(cntrDataFirst[idxOfLargeInTheFirstArray].mCorners);
		cntrDataFirst[i].SetDistFromLargeCenter(cntrDataFirst[idxOfLargeInTheFirstArray].mCg);
	}

	Mat grad8Thr, matAdpt(sz.height, sz.width, CV_8UC1);
	//Mat grag8ThrMar;
	Mat frameRgb;
	Mat frameRgbDisplayed;
	cv::imshow("gradFirst", firstGrad);
	shot.setTo(0);
	double measureSharpnessThr = 400.0f;// 350.0f;
	vector<Rect> foundShotRects;
	vector<Scalar> colors;
	vector<ContourData> shotsCand;
	bool isToBreak = false;
	bool isToSave = false;
	bool isInspectNms = false;
	bool isFromFile = false && !isToSave;
	//isInspectNms = true;
	//isFromFile = true && !isToSave;

	int sumX = 0, sumY = 0;
	while(1)
	{		
		if (!isFromFile)
		{
			shotsCand.resize(0);
			// Capture frame-by-frame
			cap >> frame;
			cntFrameNum++;
			cout << cntFrameNum << endl;

			if (frame.size().height == 0)
				break;
			//if (cntFrameNum < 1475) continue;

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
			cv::equalizeHist(smallFrame, smallFrame);
			blur(smallFrame, smallFrame, Size(fltrSz, fltrSz));
			if (isToSave)
			{
				std::stringstream buf;
				buf << dirName << fName << "/" << cntFrameNum << ".bmp";
				imwrite(buf.str(), smallFrame);
			}
		}
		else
		{
			cntFrameNum = 1908; //940;//742;// //788;// 841;//  
			std::stringstream buf;
			buf << dirName << fName << "/" << cntFrameNum << ".bmp";
			smallFrame = imread(buf.str());
			cvtColor(smallFrame, smallFrame, COLOR_BGR2GRAY);
		}
		int x = 0, y = 0;
		FindMovment(firstFrame, smallFrame, x, y, rectInBound, look,false,false);
		Point pntMov(x, y);
		
		adaptiveThreshold(smallFrame, matAdpt, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, (((int)floor(1.1*sz.width)) | 1), 0);
		cv::imshow("matAdptBeforeClean", matAdpt);
		Canny(matAdpt, grad8Thr, 0, 255,5,true);
		Mat canMat;
		Canny(smallFrame, canMat, thrOfGrad, 1.75 * thrOfGrad, 3, true);
		//cv::imshow("gradThrBeforeClean", grad8Thr);
		//threshold(smallFrame, matAdpt, thr, 255, THRESH_BINARY);
		Mat matDx, matDy;
		Sobel(smallFrame, matDx, CV_16S, 1, 0);
		Sobel(smallFrame, matDy, CV_16S, 0, 1);
		Mat mapMove(sz.height, sz.width, CV_8UC1);
		mapMove.setTo(255);
		Point pointsRect[4];

		for (int p = 0; p < 4; ++p)
		{
			pointsRect[p] = metaData.mPoints[p] + pntMov;
		}
		drawPolyRect(mapMove, pointsRect, Scalar(0), -1);

		matDx.setTo(0, mapMove);
		matDy.setTo(0, mapMove);
		double mnx, mxx, mny, mxy;
		minMaxLoc(matDx, &mnx, &mxx);
		minMaxLoc(matDy, &mny, &mxy);
		double sharpMeasure = max(max(max(abs(mnx), abs(mxx)), abs(mny)), abs(mxy));
		if (sharpMeasure < measureSharpnessThr)
		{
			cout << "Skipping " << cntFrameNum << " Blured frame " << sharpMeasure << endl;
			//continue;//Blured image
		}
			
		
		matAdpt.setTo(0, mapMove);
		grad8Thr.setTo(0, mapMove);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		cv::findContours(grad8Thr, contours, hierarchy,RETR_CCOMP, CHAIN_APPROX_NONE);

		int numOfContours = (int)contours.size();
		char buf[256] = { '\0' };
		sprintf_s(buf, "FindShot: F=%d move x=%d y=%d. Found cntr=%d\n", cntFrameNum,x,y, numOfContours);
		OutputDebugStringA(buf);
/*********/
		int idxMax = -1;
		int rectAreaMax = 0;
		Point2f pntCgMax(0, 0);
		
		if (numOfContours > 0)
		{
			int idx = 0;
			double arMax = 0;
			vector<ContourData> cdsFrame;
			int idxOfLargeInTheArray = -1;
			while(idx>=0)
			{
				if (contours[idx].size() > 3)
				{
					ContourData cd(contours[idx], sz);
					if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
						continue;
					//shot.setTo(0);
					//polylines(shot, cd.mContour, true, 255, 1, 8);
					//cv::setMouseCallback("shot", mouse_callback, &metaData);
					//cv::imshow("cntr", shot);
					//cv::waitKey();
					//char buf[256] = { '\0' };
					//sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
					//	cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
					//OutputDebugStringA(buf);
					CalcAverageBorderColor(smallFrame, cd);
					if (cd.mLen > rectAreaMax)
					{
						rectAreaMax = cd.mLen;
						idxMax = idx;
						pntCgMax = cd.mCg;
						idxOfLargeInTheArray = (int)cdsFrame.size();
					}
					//if (cd.mLen < 10 /*|| (cd.mShRct.width > 25 && cd.mShRct.height > 25)*/)//too small delete it
					//{
					//	contours.erase(contours.begin() + idx);
					//	--idx;
					//	numOfContours--;
					//}
					//if (cd.mLen >= 7)
					cdsFrame.push_back(cd);
				}
				if(idx < 0)
					break;
				idx = hierarchy[idx][0];
			}
			/*If a n out of focus frame than the contours will break, skip these frames*/
			if (rectAreaMax / (float)rectAreaMaxFirst < 0.65)
			{
				//cout <<"FindShot:*****Skipping "<< cntFrameNum<<" area "<< rectAreaMax / (float)rectAreaMaxFirst << endl;
				//cv::imshow("gradThr", grad8Thr);
				//cv::imshow("skipping", frameRgb);
				//cv::waitKey();
				//continue;
			}
			//If the contour is repeating itself from the inside, return the inside contour
			vector<Point> rpt = cdsFrame[idxOfLargeInTheArray].FixSlightlyOpenContour();
			if ((int)rpt.size() > 0)
			{
				cdsFrame.push_back(ContourData(	rpt, 
												cdsFrame[idxOfLargeInTheArray].mPicSize, 
												cdsFrame[idxOfLargeInTheArray].mFrameNum, 
												(int)cdsFrame.size()));
			}
			/*This is the movment between the frames*/
			//pntCgMax = cdsFrame[idxOfLargeInTheArray].mCg;
			//x = cdsFrame[idxOfLargeInTheArray].mShRct.x - cntrDataFirst[idxOfLargeInTheFirstArray].mShRct.x;
			//y = cdsFrame[idxOfLargeInTheArray].mShRct.y - cntrDataFirst[idxOfLargeInTheFirstArray].mShRct.y;
			
			//x = (int)round(cdsFrame[idxOfLargeInTheArray].mCorners[0].x - cntrDataFirst[idxOfLargeInTheFirstArray].mCorners[0].x);
			//y = (int)round(cdsFrame[idxOfLargeInTheArray].mCorners[0].y - cntrDataFirst[idxOfLargeInTheFirstArray].mCorners[0].y);
			int cgXmov = abs(cdsFrame[idxOfLargeInTheArray].mCg.x - cntrDataFirst[idxOfLargeInTheFirstArray].mCg.x);
			int cgYmov = abs(cdsFrame[idxOfLargeInTheArray].mCg.y - cntrDataFirst[idxOfLargeInTheFirstArray].mCg.y);
			if (abs(x) > 10 || abs(y) > 10 || cgXmov > 10 || cgYmov > 10)
			{
				cout << "Skipping" << endl;
				//continue;
			}
			//cdsFrame[idxOfLargeInTheArray].mShRct.x -= x;
			//cdsFrame[idxOfLargeInTheArray].mShRct.y -= y;
			//cdsFrame[idxOfLargeInTheArray] = cdsFrame[idxOfLargeInTheArray] - pntMov;
			for (idx=0; idx < (int)cdsFrame.size(); idx++)
			{
				cdsFrame[idx].mFrameNum = cntFrameNum;
				cdsFrame[idx].mIdxCntr = idx;
				cdsFrame[idx].SetDistFromLargeCenter(pntCgMax);
				ContourData cd = cdsFrame[idx];
				CalcAverageRectInOutColor(smallFrame, cd);
				if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
					continue;

				char buf[256] = { '\0' };
				sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
					cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
				OutputDebugStringA(buf);
				if (0|| cntFrameNum == -108)// && idx == 2)// && idxFirst == 7)
				{
					shot.setTo(0);
					cv::rectangle(shot, cdsFrame[idxOfLargeInTheArray].mShRct, 255);
					cv::rectangle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mShRct, 128);
					polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 200, 1, 8);
					polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 80, 1, 8);
					polylines(shot, cd.mContour, true, 255, 1, 8);
					circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
					circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
					cv::imshow("matAdpt", matAdpt);
					cv::imshow("cntrIn", shot);
					cv::imshow("Frame", smallFrame); 
					cv::imshow("grad8Thr", grad8Thr);
					cv::waitKey();
				}
				if (IsItShot(cd))
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
						if (cntFrameNum == -256 && idx == 10 && idxFirst == 2)
						{
							shot.setTo(0);
							//polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
							//polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
							cout << "Check " << idx << " with " << idxFirst << endl;
							polylines(shot, cd.mContour, true, 255, 1, 8);
							polylines(shot, cdf.mContour, true, 128, 1, 8);
							//circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
							//circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
							cv::imshow("gradThr", grad8Thr);
							cv::imshow("cntrIn", shot);
							cv::waitKey();
						}

						vector<ContourData> cdsResidu;
						if (cd.CompareContourAndReturnResidu(cdf, cdsResidu, &pntMov)==true)//returns true if equal
						{
							if(cd.mLen>cdf.mLen && cdf.mLen>1 && cd.mLen/(float)cdf.mLen<1.25)
							{
								for (int cdResIdx = 0; cdResIdx < (int)cdsResidu.size(); ++cdResIdx)
								{
									if (cdsResidu[cdResIdx].mLen > 10 && cdsResidu[cdResIdx].mLen<cdf.mLen && cdsResidu[cdResIdx].mLen<cd.mLen)
									{
										cdsResidu[cdResIdx].SetDistFromLargeCenter(pntCgMax);
										CalcAverageRectInOutColor(smallFrame, cdsResidu[cdResIdx]);
										if (IsItShot(cdsResidu[cdResIdx]))
										{
											//shot.setTo(0);
											//Mat s1, s2, s3;
											//shot.copyTo(s1); shot.copyTo(s2); shot.copyTo(s3);
											//ContourData cdTemp(cd - pntMov);
											////polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
											////polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
											//polylines(shot, cdTemp.mContour, true, 255, 1, 8);
											//polylines(shot, cdf.mContour, true, 128, 1, 8);
											//polylines(shot, cdsResidu[cdResIdx].mContour, true, 200, 1, 8);
											//polylines(s1, cdTemp.mContour, true, 255, 1, 8);
											//polylines(s2, cdf.mContour, true, 128, 1, 8);
											//polylines(s3, cdsResidu[cdResIdx].mContour, true, 200, 1, 8);

											////circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
											////circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
											//cv::imshow("gradThr", grad8Thr);
											//cv::imshow("cntrAndCntrIn", shot);
											//cv::imshow("s1", s1);
											//cv::imshow("s2", s2);
											//cv::imshow("s3", s3);
											//cv::waitKey();
											cdsFrame.push_back(cdsResidu[cdResIdx]);
										}
									}
								}
							}
							isFound = true;
							//cntrDataFirst.push_back(cd + pntMov);
							break;
						}
					}
					if (!isFound && cd.mLen<200)
					{						
						//cd = cd + pntMov;
						shotsCand.push_back(cd);
						//cntrDataFirst.push_back(cd);
						shot.setTo(0);
						//for (int i=0; i < shotsCand.size(); i++)
						//{
						//	polylines(shot, shotsCand[i].mContour, true, 255, 1, 8);
						//}
						polylines(shot, cd.mContour, true, 255, 1, 8);
						char buf[256] = { '\0' };
						sprintf_s(buf, "FindShot: New F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
							cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
						OutputDebugStringA(buf);						
						//cv::imshow("gradThr", grad8Thr);
						//cv::imshow("Frame", smallFrame);
						//cv::imshow("cntr", shot);
						//cv::imshow("gradMat", matAdpt);
						//cv::waitKey();
						//cout << "Num of shots in frame " << cntFrameNum << " is " << shotsCand.size() << endl;
						//cntrDataFirst.push_back(cd);
					}
				}
			}
		}
		//if (isFromFile && isInspectNms)
		//{
		//	Mat matToDraw(sz.height, sz.width, CV_8UC1);		NMS(shotsCand, smallFrame, pntMov,&matToDraw);
		//}
		//else
		//	NMS(shotsCand,smallFrame, pntMov);
		int numOfShotsFound = (int)shotsCand.size();
		cout << "Num of shots after NMS in frame " << cntFrameNum << " is ," << numOfShotsFound << endl;
		fout << "Num of shots after NMS in frame " << cntFrameNum << " is ," << numOfShotsFound << endl;
		shot.setTo(0);
		for (int rc = 0; rc < numOfShotsFound; ++rc)
		{
			Rect rct(shotsCand[rc].mShRct);
			//polylines(shot, shotsCand[rc].mContour, true, 1, -1, 8);

			//rct.x -= x;
			//rct.y -= y;
			//rectangle(frameRgb, rct, colors[rc], 2);
			if (!isFromFile)
			{
				if (rc == numOfShotsFound - 1)
					rectangle(frameRgb, rct, Scalar(0, 255, 0), 1);
				else
					rectangle(frameRgb, rct, Scalar(255, 0, 0), 1);
			}
			else
			{
				rectangle(smallFrame, rct, Scalar(10, 0, 0), 1);
				//rectangle(matAdpt, rct, Scalar(128, 0, 0), 1);
				//cv::imshow("Frame", smallFrame);
				//cv::waitKey();
			}
			//Copy the shot to the shots map, this map will be added to the totla map
			rct = rct - pntMov;
			Mat pMatThr = matAdpt(shotsCand[rc].mShRct);
			Mat pMatShot = shot(rct);
			pMatThr.copyTo(pMatShot);
			bitwise_not(pMatShot, pMatShot);
		}
		Mat shot16;
		shot.convertTo(shot16, shotsHistogramMat.type(),1.0/255.0);
		shotsHistogramMat += shot16;
		if (!isFromFile)
		{
			frameRgb.copyTo(frameRgbDisplayed);
			cv::imshow("SHOTS", frameRgbDisplayed);
			//cv::imshow("PrevFrame", prevFrame);
		}
		double mn16, mx16;
		minMaxLoc(shotsHistogramMat, &mn16, &mx16);
		shotsHistogramMat.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
		cv::imshow("shotsHistogramMat", shot);
		cv::imshow("firstFrame", firstFrame);
		//cv::imshow("canMat", canMat);
		cv::imshow("matAdpt", matAdpt);
		cv::imshow("Frame", smallFrame);
		cv::imshow("gradThr", grad8Thr);
		cv::setMouseCallback("gradThr", mouse_callback, (void*)&grad8Thr);
		// Press  ESC on keyboard to exit
		if (0 && !isFromFile)//cntFrameNum > 680)//isToBreak)//
		{
			isToBreak = false;
			char c = (char)cv::waitKey();
			if (c == 27)
				break;
		}
		else if (!isFromFile)
			waitKey(25);
		else
			break;
	}
	//destroyAllWindows();
	if (!isFromFile)
		cv::imshow("SHOTS", frameRgbDisplayed);
	cv::waitKey();
	cv::destroyAllWindows();
	fout.close();
  // When everything done, release the video capture object
	cap.release();
    return 0;
}
