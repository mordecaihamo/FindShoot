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
		imshow(md->mWindowName, md->mDrawMat);
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
		imshow(md->mWindowName, md->mDrawMat);
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
			imshow(md->mWindowName, md->mDrawMat);
		}
	}
}

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN) 
	{
		cout << "(" << x << ", " << y << ")" << endl;
	}
}

int main()
{
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
		setMouseCallback("EnterPositions", MarkRectDrag_callback, &metaData);
		int k = waitKey();
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
	
	cvtColor(smallFrame, firstFrame, COLOR_BGR2GRAY);
	drawPolyRect(smallFrame, metaData.mPoints, Scalar(255, 0, 17), 1);
	imshow("TargetOnFrame", smallFrame);
	setMouseCallback("TargetOnFrame", mouse_callback);
	equalizeHist(firstFrame, firstFrame);
	firstFrame.copyTo(smallFrame);
	
	Mat frameDiffF(sz.height, sz.width, CV_64FC1);

	Rect rctMargin;
	int look = 15;
	rctMargin.x = look;
	rctMargin.y = look;
	rctMargin.width = sz.width - 2 * look;
	rctMargin.height = sz.height - 2 * look;
	Mat smallFrameMar(smallFrame, rctMargin);
	Mat frameDiffFMar(frameDiffF, rctMargin);
	Mat firstFrameMar(firstFrame, rctMargin);
	Mat target(sz.height, sz.width, CV_8UC1);
	Mat map(sz.height, sz.width, CV_8UC1);
	Rect rectInBound = FindInboundRect(rctMargin, metaData.mPoints);
	
	drawPolyRect(target, metaData.mPoints, Scalar(0), -1);
	map.setTo(255);
	drawPolyRect(map, metaData.mPoints, Scalar(0), -1);
	//Find the edges inside the map
	Mat dx, dy, grad, grad8;
	Sobel(smallFrame, dx, CV_16S, 1, 0);
	dx.setTo(0, map);
	Sobel(smallFrame, dy, CV_16S, 0, 1);
	dy.setTo(0, map);
	pow(dx, 2, dx);
	pow(dy, 2, dy);
	grad = dx + dy;
	double mn, mx;
	minMaxLoc(grad, &mn, &mx);
	grad -= mn;
	if (mx > 0)
	{
		grad.convertTo(grad8, CV_8UC1, 255 / mx);
		threshold(grad8, grad8, 200, 255,THRESH_BINARY);
	}
	else
		grad8.setTo(0);


	//Dilation(target, target, 1);
	imshow("grad", grad8);
	//map.setTo(255);
	//rectangle(map, targetRect, 0, -1);
	Mat mapNot;
	bitwise_not(map, mapNot);

	double ffMin=0, ffMax=0;
	int ffMinIdx=0, ffMaxIdx=0;
	smallFrame.copyTo(firstFrame);
	int fltrSz = 3;
	//blur(firstFrame, firstFrame,Size(fltrSz, fltrSz));
	imshow("firstFrame", firstFrame);
	minMaxIdx(firstFrame, &ffMin, &ffMax,&ffMinIdx,&ffMaxIdx,mapNot);
	int bg = cvFloor((ffMin+ffMax)*0.5);
	//accu.setTo(0);
	frameDiffF.setTo(0);
	threshold(target, target, 1, 255, THRESH_BINARY);
	bitwise_or(map, grad8, map);
	Mat shiftMap = map(Rect(1, 1, sz.width - 1, sz.height - 1));
	Mat shiftGrad = grad8(Rect(0, 0, sz.width - 1, sz.height - 1));
	bitwise_or(shiftMap, shiftGrad, map);
	rectangle(target, rectInBound, Scalar(100));
	imshow("mapZ", map);
	imshow("mapZ1", target);
	waitKey();
	srand(1);
	Mat shot = map.clone();
	Mat mapMar(map, rctMargin);
	Mat targetMar(target, rctMargin);
	//Mat matDrawOn = map.clone();
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
		equalizeHist(smallFrame, smallFrame);
		int x = 0, y = 0;
		bool isToDisplay = false;
		//if (cntFrameNum == 738)	isToDisplay = true;
		//cout << cntFrameNum<<" Movment search" << endl;
		FindMovment(firstFrame, smallFrame, x, y, rectInBound, look, isToDisplay);
		//if (abs(x) > 8 || abs(y) > 8)
		//	continue;
		Rect movRct(rctMargin);
		movRct.x += x;
		movRct.y += y;
		smallFrameMar = smallFrame(movRct);
		//blur(smallFrame, smallFrame, Size(fltrSz, fltrSz));
		if (0&&!prevFrame.empty())
		{
#pragma warning(disable : 4996)
			Transform = estimateRigidTransform(smallFrame, firstFrame, 1);
			Transform(Range(0, 2), Range(0, 2)) = Mat::eye(2, 2, CV_64FC1);
			Transform_avg += (Transform - Transform_avg) / 2.0;
			warpAffine(smallFrame, warped, Transform_avg, Size(smallFrame.cols, smallFrame.rows));
			warped.copyTo(smallFrame);
			//imshow("Camw", warped);
		}

		frameDiff = abs(firstFrameMar - smallFrameMar);
		//Mat frameDiffShotInShot = frameDiff.clone();
		imshow("diffBefore", frameDiff);
		//imshow("mapMarBefore", mapMar);
#undef max

		frameDiff.setTo(0, mapMar);
		//imshow("diffAfterMap", frameDiff);
		//frameDiff = max(frameDiff, 30);
		//imshow("diffAfterMax", frameDiff);
		//frameDiff = frameDiff - 100;
		//equalizeHist(frameDiff, frameDiff);

		//Erosion(frameDiff, shot, 1);
		threshold(frameDiff, shot, 35, 255, THRESH_BINARY);
		//Dilation(shot, shot, 1);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(shot, contours, RETR_LIST, CHAIN_APPROX_NONE);
		int cntrSz = (int)contours.size();
		char buf[256] = { '\0' };
		sprintf(buf, "FindShot: F=%d move x=%d y=%d. Found cntr=%d\n", cntFrameNum,x,y,cntrSz);
		OutputDebugStringA(buf);

		if (cntrSz > 0	&& cntrSz < 170)
		{
			int idx = 0;
			int idxMax = -1;
			int rectAreaMax = 0;
			double arMax = 0;
			for (; idx < contours.size(); idx++)
			{
				double ar = contourArea(contours[idx]);
				Rect shRct = boundingRect(contours[idx]);
				if (shRct.width == 0 || shRct.height == 0)
					continue;
				float ratioWh = min(shRct.width, shRct.height) / (float)max(shRct.width, shRct.height);
				char buf[256] = { '\0' };
				sprintf(buf, "FindShot: F=%d, Cntr=%d:%d, Shots#=%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d)\n", 
					cntFrameNum, idx, cntrSz, (int)foundShotRects.size(), ar, shRct.width, shRct.height, ratioWh, shRct.x+ (shRct.width>>1), shRct.y+ (shRct.height>>1));
				OutputDebugStringA(buf);
				//if (cntFrameNum == 160)
				//{
				//	shot.setTo(0);
				//	drawContours(shot, contours, idx, 255, FILLED, 8, hierarchy);
				//	imshow("shot", shot);
				//	imshow("FrameD", frameDiff);
				//	imshow("Frame", smallFrame);
				//	imshow("PrevFrame", prevFrame);
				//	setMouseCallback("Frame", mouse_callback, &metaData);
				//	waitKey();
				//}

				if ((ar>arMax)&&
					(ar > 10 && shRct.width<20 && shRct.height<20 && shRct.width > 2 && shRct.height > 2 && ratioWh>0.54)||
					(ar > 25 && shRct.width < 20 && shRct.height < 20 && shRct.width > 4 && shRct.height > 4 && ratioWh > 0.45)||
					(ar >= 3 && shRct.width < 20 && shRct.height < 20 && shRct.width > 4 && shRct.height >= 4 && ratioWh > 0.79))
				{
					Mat onlyCntr = shot(shRct);
					int cntNonZ = countNonZero(onlyCntr);
					float ratioAr = cntNonZ / (float)(shRct.width*shRct.height);
					char buf[256] = { '\0' };
					sprintf(buf, "FindShot cand: %f\n", ratioAr);
					OutputDebugStringA(buf);
					if (ratioAr > 0.5)
					{
						arMax = ar;
						rectAreaMax = shRct.width*shRct.height;
						idxMax = idx;
					}
				}
			}
			if (idxMax > -1)
			{
				isToBreak = true;
				//cout << cntFrameNum << " Found good cand" << endl;
				Rect shRct = boundingRect(contours[idxMax]);
				shRct.x += look;
				shRct.y += look;
				foundShotRects.push_back(shRct);
				//Update all rects to the replace of the first frame.
				for (int rc = 0; rc < (int)foundShotRects.size(); ++rc)
				{
					foundShotRects[rc].x += x;
					foundShotRects[rc].y += y;
				}
				//found shot
				//drawContours(map, contours, idx, 255, FILLED, 8, hierarchy);
				shot.setTo(0);
				drawContours(shot, contours, idxMax, 255, FILLED, 8, hierarchy);
				Dilation(shot, shot, 2);
				bitwise_or(shot, mapMar, mapMar);
				//rectangle(mapMar, shRct, Scalar(255), -1);
				smallFrame.copyTo(firstFrame);
				//Move the grad map according to all of the movments
				sumX += x+1;
				sumY += y+1;
				if (sumX >= look)
					sumX = look-1;
				if (sumX <= -look)
					sumX = -(look-1);
				if (sumY >= look)
					sumY = look-1;
				if (sumY <= -look)
					sumY = -(look-1);

				Rect rctTemp(rctMargin);
				int xMovGrad = x, yMovGrad = y;
				bool isMoveDisplay = false;
				//if (cntFrameNum == 373)					isMoveDisplay = true;
//				if (abs(x) > 1 && abs(y) > 1)
				{
					int lookGradMove = 5;// max(abs(x + 1), abs(y + 1));
					FindMovment(map, grad8, xMovGrad, yMovGrad, rectInBound, lookGradMove, isMoveDisplay);
				}
				rctTemp.x += xMovGrad;
				rctTemp.y += yMovGrad;
				//imshow("mapMar1", mapMar);
				mapMar = map(rctTemp);
				//imshow("mapMar2", mapMar);
				imshow("shot", shot);
				colors.push_back(Scalar(rand() % 256, rand() % 256, rand() % 256));
				char buf[256] = { '\0' };
				sprintf(buf, "FindShot: ***F=%d, cntr=%d, Shots#=%d, sx=%d, sy=%d\n", cntFrameNum,idxMax, (int)foundShotRects.size(),sumX,sumY);
				OutputDebugStringA(buf);
				
				x = 0;
				y = 0;
			}
		}

		//frameDiff.convertTo(frameDiffF, CV_64FC1, 255.0 / NUM_OF_FRAMES_IN_ACC);
		//accu = accu + frameDiffF;
		//minMaxIdx(accu, &ffMin, &ffMax, &ffMinIdx, &ffMaxIdx, mapNot);
		//accu.convertTo(frameDiff, CV_8UC1, 255 / ffMax);
		// Display the resulting frame
		//rectangle(frameDiff, targetRect, 128, 2);
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
		imshow("SHOTS", frameRgb);
		imshow("firstFrame", firstFrame);
		imshow("mapMar", mapMar);
		imshow( "FrameD", frameDiff);
		setMouseCallback("FrameD", mouse_callback);
		imshow("Frame", smallFrame);
		imshow("PrevFrame", prevFrame);
 
		// Press  ESC on keyboard to exit
		if (0)//cntFrameNum > 680)//isToBreak)//
		{
			isToBreak = false;
			char c = (char)waitKey();
			if (c == 27)
				break;
		}
		else
			waitKey(5);
	}
	destroyAllWindows();
	imshow("SHOTS", frameRgbDisplayed);
	waitKey();
	destroyAllWindows();
  // When everything done, release the video capture object
	cap.release();
    return 0;
}
