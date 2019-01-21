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
	cv::imshow("TargetOnFrame", smallFrame);
	cv::setMouseCallback("TargetOnFrame", mouse_callback);
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
	//Find the edges inside the map
	Mat dx, dy, grad, grad8;
	cv::Sobel(smallFrame, dx, CV_16S, 1, 0);
	dx.setTo(0, map);
	cv::Sobel(smallFrame, dy, CV_16S, 0, 1);
	dy.setTo(0, map);
	cv::pow(dx, 2, dx);
	cv::pow(dy, 2, dy);
	grad = dx + dy;
	
	int gradThr = 100;
	minMaxLoc(grad, &mn, &mx);
	grad -= mn;
	if (mx > 0)
	{
		grad.convertTo(grad8, CV_8UC1, 255 / mx);
		cv::threshold(grad8, grad8, gradThr, 255,THRESH_BINARY);
	}
	else
		grad8.setTo(0);

	Mat firstGrad;
	grad8.copyTo(firstGrad);

	//Dilation(target, target, 1);
	cv::imshow("grad", grad8);
	//map.setTo(255);
	//rectangle(map, targetRect, 0, -1);
	Mat mapNot;
	bitwise_not(map, mapNot);

	double ffMin=0, ffMax=0;
	int ffMinIdx=0, ffMaxIdx=0;
	smallFrame.copyTo(firstFrame);
	int fltrSz = 3;
	//blur(firstFrame, firstFrame,Size(fltrSz, fltrSz));
	cv::imshow("firstFrame", firstFrame);
	minMaxIdx(firstFrame, &ffMin, &ffMax,&ffMinIdx,&ffMaxIdx,mapNot);
	int bg = cvFloor((ffMin+ffMax)*0.5);
	cv::threshold(target, target, 1, 255, THRESH_BINARY);
	
	Mat shiftMap = map(Rect(1, 1, sz.width - 1, sz.height - 1));
	Mat shiftGrad = grad8(Rect(0, 0, sz.width - 1, sz.height - 1));

	rectangle(target, rectInBound, Scalar(100));
	cv::imshow("mapZ", map);
	cv::imshow("mapZ1", target);
	cv::waitKey();
	srand(1);
	Mat shot = map.clone();
	Mat mapMar(map, rctMargin);
	Mat targetMar(target, rctMargin);
	int thrOfGrad = 100;
	cv::threshold(firstGrad, firstGrad, thrOfGrad, 255, THRESH_BINARY);
	//Map all the contours of the first frame
	vector<vector<Point> > contoursFirst;
	vector<Vec4i> hierarchyFirst;
	cv::findContours(firstGrad, contoursFirst, RETR_LIST, CHAIN_APPROX_NONE);
	//Leave only large contours
	int numOfFirstContours = (int)contoursFirst.size();
	int idxMaxFirst = -1;
	int rectAreaMaxFirst = 0;
	Point2f pntCgMaxFirst(0, 0);

	if (numOfFirstContours > 0)
	{
		int idx = 0;
		double arMax = 0;
		for (; idx < contoursFirst.size(); idx++)
		{
			double ar = contourArea(contoursFirst[idx]);
			Rect shRct = boundingRect(contoursFirst[idx]);
			if (shRct.width == 0 || shRct.height == 0)
				continue;
			float ratioWh = min(shRct.width, shRct.height) / (float)max(shRct.width, shRct.height);
			int cntNonZ = (int)contoursFirst[idx].size();// countNonZero(onlyCntr);
			float ratioAr = cntNonZ / (float)(shRct.width*shRct.height);
			float ratioFromAll = cntNonZ / (float)(sz.width*sz.height);
			char buf[256] = { '\0' };
			sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
				cntFrameNum, idx, numOfFirstContours, ar, shRct.width, shRct.height, ratioWh, shRct.x + (shRct.width >> 1), shRct.y + (shRct.height >> 1), cntNonZ, ratioFromAll);
			OutputDebugStringA(buf);
			if (cntNonZ > rectAreaMaxFirst)
			{
				rectAreaMaxFirst = cntNonZ;
				idxMaxFirst = idx;
				pntCgMaxFirst.x = shRct.x + shRct.width*0.5f;
				pntCgMaxFirst.y = shRct.y + shRct.height*0.5f;
			}
			//if (cntFrameNum == 160)
			//{
			//	shot.setTo(0);
			//	drawContours(shot, contoursFirst, idx, 255, 0, 8, hierarchyFirst);
			//	cv::setMouseCallback("shot", mouse_callback, &metaData);
			//	cv::imshow("cntr", shot);
			//	cv::waitKey();
			//}

			if (ratioFromAll < 0.0001)//too small delete it
			{
				contoursFirst.erase(contoursFirst.begin() + idx);
				--idx;
			}
		}
	}
	int numOfInitCntr = (int)contoursFirst.size();

	Mat grad8Thr;
	Mat firstGradMar, grag8ThrMar;
	firstGradMar = firstGrad(rctMargin);
	Mat frameRgb;
	Mat frameRgbDisplayed;
	
	shot.setTo(0);
	vector<Rect> foundShotRects;
	vector<Scalar> colors;
	bool isToBreak = false;
	int sumX = 0, sumY = 0;
	while(1)
	{		
		// Capture frame-by-frame
		cap >> frame;		
		cntFrameNum++;
		//auto res=cap.retrieve(frame, cntFrameNum);
		//cntFrameNum += 10;

		if (frame.size().height == 0)
			break;
		//if (cntFrameNum % 10 != 0)
		//	continue;
		
		resize(frame, frameRgb, sz);
		//Mat Rgbs[3];
		//split(frameRgb, Rgbs);
		cvtColor(frame, frame, COLOR_BGR2GRAY);
		smallFrame.copyTo(prevFrame);
		//Rgbs[1].copyTo(smallFrame);
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
		//smallFrame.setTo(bg, map);
		cv::equalizeHist(smallFrame, smallFrame);

		cv::Sobel(smallFrame, dx, CV_16S, 1, 0);
		dx.setTo(0, map);
		cv::Sobel(smallFrame, dy, CV_16S, 0, 1);
		dy.setTo(0, map);
		cv::pow(dx, 2, dx);
		cv::pow(dy, 2, dy);
		grad = dx + dy;
		//double mn, mx;
		//minMaxLoc(grad, &mn, &mx);
		grad -= mn;
		if (mx > 0)
		{
			grad.convertTo(grad8, CV_8UC1, 255 / mx);
			cv::threshold(grad8, grad8Thr, thrOfGrad, 255, THRESH_BINARY);
		}
		else
			grad8.setTo(0);

		int x = 0, y = 0;
		bool isToDisplay = false;
		//if (cntFrameNum == 738)	isToDisplay = true;
		//cout << cntFrameNum<<" Movment search" << endl;
		//FindMovment(grad8Thr, firstGrad, x, y, rectInBound, look, false, isToDisplay);
		Rect movRct(rctMargin);
		movRct.x += x;
		movRct.y += y;

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
			for (; idx < contours.size(); idx++)
			{
				double ar = contourArea(contours[idx]);
				Rect shRct = boundingRect(contours[idx]);
				if (shRct.width == 0 || shRct.height == 0)
					continue;
				float ratioWh = min(shRct.width, shRct.height) / (float)max(shRct.width, shRct.height);
				int cntNonZ = (int)contours[idx].size();// countNonZero(onlyCntr);
				float ratioAr = cntNonZ / (float)(shRct.width*shRct.height);
				float ratioFromAll = cntNonZ / (float)(sz.width*sz.height);
				char buf[256] = { '\0' };
				sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
					cntFrameNum, idx, numOfContours, ar, shRct.width, shRct.height, ratioWh, shRct.x + (shRct.width >> 1), shRct.y + (shRct.height >> 1), cntNonZ, ratioFromAll);
				OutputDebugStringA(buf);
				if (cntNonZ > rectAreaMax)
				{
					rectAreaMax = cntNonZ;
					idxMax = idx;
					pntCgMax.x = shRct.x + shRct.width*0.5f;
					pntCgMax.y = shRct.y + shRct.height*0.5f;
				}
				if(cntNonZ < 10 || (shRct.width>25 && shRct.height>25))//too small delete it
				{
					contours.erase(contours.begin() + idx);
					--idx;
					numOfContours--;
				}
			}
			/*This is the movment between the frames*/
			x = pntCgMax.x - pntCgMaxFirst.x;
			y = pntCgMax.y - pntCgMaxFirst.y;

			for (idx=0; idx < contours.size(); idx++)
			{
				double ar = contourArea(contours[idx]);
				Rect shRct = boundingRect(contours[idx]);
				if (shRct.width == 0 || shRct.height == 0)
					continue;
				float ratioWh = min(shRct.width, shRct.height) / (float)max(shRct.width, shRct.height);
				int cntNonZ = (int)contours[idx].size();// countNonZero(onlyCntr);
				float ratioAr = cntNonZ / (float)(shRct.width*shRct.height);
				float ratioFromAll = cntNonZ / (float)(sz.width*sz.height);
				char buf[256] = { '\0' };
				sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
					cntFrameNum, idx, numOfContours, ar, shRct.width, shRct.height, ratioWh, shRct.x + (shRct.width >> 1), shRct.y + (shRct.height >> 1), cntNonZ, ratioFromAll);
				OutputDebugStringA(buf);

				if ((ar > arMax) &&
					(ar > 10 && shRct.width < 20 && shRct.height < 20 && shRct.width > 2 && shRct.height > 2 && ratioWh > 0.54) ||
					(ar > 25 && shRct.width < 20 && shRct.height < 20 && shRct.width > 4 && shRct.height > 4 && ratioWh > 0.45) ||
					(ar >= 3 && shRct.width < 20 && shRct.height < 20 && shRct.width > 4 && shRct.height >= 4 && ratioWh > 0.79))
				{
					/*Need to go over the first contours compare its cg and MatchShape and see if this one is new, if yes add it to the list*/
					{
						shot.setTo(0);
						drawContours(shot, contours, idx, 255, 0, 8, hierarchy);
						cv::setMouseCallback("shot", mouse_callback, &metaData);
						cv::imshow("cntr", shot);
						cv::waitKey();
					}
				}
			}

		}

/********/
		int numOfShotsFound = (int)foundShotRects.size();
		for (int rc = 0; rc < numOfShotsFound-1; ++rc)
		{
			Rect rct = foundShotRects[rc];
			rct.x += x;
			rct.y += y;
			//rectangle(frameRgb, rct, colors[rc], 2);
			rectangle(frameRgb, rct, Scalar(0,255,0), 2);
		}
		if(numOfShotsFound>0)
			rectangle(frameRgb, foundShotRects[numOfShotsFound - 1], Scalar(255, 0, 0), 2);
		frameRgb.copyTo(frameRgbDisplayed);
		cv::imshow("SHOTS", frameRgb);
		cv::imshow("firstFrame", firstFrame);
		cv::imshow("mapMar", mapMar);
		cv::imshow("Frame", smallFrame);
		cv::imshow("PrevFrame", prevFrame);
		cv::imshow("grad", grad8);
		cv::setMouseCallback("grad", mouse_callback,(void*)&grad8);
		cv::imshow("gradThr", grad8Thr);
		cv::imshow("gradFirst", firstGrad);
 
		// Press  ESC on keyboard to exit
		if (1)//cntFrameNum > 680)//isToBreak)//
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
