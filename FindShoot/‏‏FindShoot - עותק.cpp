// FindShoot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "FindShoot.h"
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
#include "AnalyzeShotsResult.h"
#include <experimental/filesystem>
#include <chrono>
#include <windows.h>
#include <strsafe.h>

using namespace std;
using namespace cv;


void Trace(PCSTR pszFormat, ...)
{
	CHAR szTrace[1024];

	va_list args;
	va_start(args, pszFormat);
	(void)StringCchVPrintfA(szTrace, ARRAYSIZE(szTrace), pszFormat, args);
	va_end(args);

	OutputDebugStringA(szTrace);
}

void MarkRect_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN) //EVENT_MOUSEMOVE
	{
		std::cout << "(" << x << ", " << y << ")" << endl;

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
		std::cout << "(" << x << ", " << y << ")" << endl;
	}
	else if (event == EVENT_LBUTTONUP)
	{
		isToDraw = false;
		closePoint = -1;
	}
	else if (event == EVENT_MOUSEMOVE && isToDraw)
	{
		std::cout << "(" << x << ", " << y << ")" << endl;
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
		std::cout << "(" << x << ", " << y << ")";
		if (param)
		{
			Mat* m = (Mat*)param;
			int chNum = m->channels();
			if (chNum == 3)
			{
				Vec3b bgrPixel = ((Mat*)param)->at<Vec3b>(y, x);
				std::cout << ": (" << bgrPixel[0] << "," << bgrPixel[1] << "," << bgrPixel[2] << ")" << endl;
			}
			else if (chNum == 1)
			{
				uchar pixel = ((Mat*)param)->at<uchar>(y, x);
				std::cout << ": " << (int)pixel << endl;
			}
		}
	}
}

using namespace cv;
using namespace std;

int Analyze(char* vidName, int isDebugMode)
{
	String fullFileName(vidName);
	size_t posOfLastSlash = fullFileName.find_last_of("/\\");
	String dirName = "";
	String fName = "";
	String extName = "";
	if (posOfLastSlash != string::npos)
	{
		dirName = fullFileName.substr(0, posOfLastSlash + 1);//"C:/moti/FindShoot/";
	}
	fName = fullFileName.substr(posOfLastSlash + 1);//"MVI_3";	
	size_t posOfLastDot = fName.find_last_of(".");
	if (posOfLastDot != string::npos)
	{
		extName = fName.substr(posOfLastDot); //".MOV";
	}
	fName = fName.substr(0, posOfLastDot);

	ofstream fout;
	fout.open(dirName + fName + ".csv");
	String lastFramePath = dirName + fName + "/lastFrame.bmp";
	String mdFileName = dirName + fName + ".txt";

	
	String s1 = dirName + fName + "/HistOfShots.xml";
	String s2 = dirName + fName + "/TimeOfShots.xml";
	String s3 = dirName + fName + "/ShotsResults.csv";
	AnalyzeShotsResult ana(s1, s2, mdFileName,lastFramePath);
	int res = ana.Compute(s3, isDebugMode);
	return res;
}

int FindShoots(const char* vidName, int selectedCh, HBITMAP imgBuffer,int imgHeight,int imgWidth, int isDebugMode)
{
	
	String fullFileName(vidName);
	size_t posOfLastSlash = fullFileName.find_last_of("/\\");
	String dirName = "";
	String fName = "";
	String extName = "";
	if (posOfLastSlash != string::npos)
	{
		dirName = fullFileName.substr(0, posOfLastSlash + 1);//"C:/moti/FindShoot/";
	}
	fName = fullFileName.substr(posOfLastSlash + 1);//"MVI_3";	
	size_t posOfLastDot = fName.find_last_of(".");
	if (posOfLastDot != string::npos)
	{
		extName = fName.substr(posOfLastDot); //".MOV";
	}
	fName = fName.substr(0, posOfLastDot);

	ofstream fout;
	fout.open(dirName + fName + ".csv");
	//String fullFileName = dirName + fName + extName;
	String mdFileName = dirName + fName + ".txt";

	struct stat buffer;
	if (stat(fullFileName.c_str(), &buffer) != 0)
	{
		std::cout << fullFileName << " not found!" << endl;
		fout << fullFileName << " not found!" << endl;
	}
	VideoCapture cap(fullFileName);
	// Check if camera opened successfully
	if (!cap.isOpened())
	{
		std::cout << "Error opening video stream or file" << endl;
		fout << "Error opening video stream or file" << endl;
		fout.close();
		return -1;
	}
	int rot = (int)cap.get(cv::CAP_PROP_MODE);

	Mat frame, prevFrame, firstFrame;
	//Mat sumFrame, tempSumFrame;
	Mat smallFrame/*, firstFrameThrs*/;
//	Mat Transform;
	//Mat Transform_avg = Mat::eye(2, 3, CV_64FC1);
	Mat warped;
	int cntFrameNum = 0;
	int frameNumAcc = 0;
	int STARTFRAME = 60;
	Size sz(-1, -1);
	for (; cntFrameNum < STARTFRAME; ++cntFrameNum)
	{
		std::cout << cntFrameNum << endl;
		cap >> frame;
		if (sz.width == -1)
		{
			sz = frame.size();
			if (sz.width != 740)
			{
				sz.height = cvRound(sz.height*(740.0f / sz.width));
				sz.width = 740;
			}

//			sumFrame = Mat(sz, CV_16UC3);
			smallFrame = Mat(sz.height, sz.width, frame.type());
			//firstFrameThrs = Mat(sz.height, sz.width, CV_8UC1);
			//firstFrameThrs.setTo(0);
			//tempSumFrame = Mat(sz.height, sz.width, CV_8UC1);
			//tempSumFrame.setTo(0);
		}
		//else if (cntFrameNum > STARTFRAME - 10)
		//{
		//	frameNumAcc++;
		//	resize(frame, smallFrame, sz, 0, 0);
		//	cvtColor(smallFrame, firstFrame, COLOR_BGR2GRAY);
		//	adaptiveThreshold(firstFrame, tempSumFrame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, (((int)floor(1.1*sz.width)) | 1), 0);
		//	firstFrameThrs = firstFrameThrs | tempSumFrame;
		//}
	}
	//bitwise_not(firstFrameThrs, firstFrameThrs);
	//bitwise_not(tempSumFrame, tempSumFrame);

	Size largeSz = frame.size();
	float ratSmall2Large = (float)largeSz.width / (float)sz.width;
	float rat = 1.0f;

	if (rot != 0)
	{
		//cvtColor(firstFrame, firstFrame, CV_BG);
		transpose(smallFrame, smallFrame);
	}
	ShootTargetMetaData metaData;
	int desiredTargetWidth = 300;
	int marginsTarget = 220;
	int margins = 10;
	if (stat(mdFileName.c_str(), &buffer) != 0)
	{
		
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

		cv::destroyAllWindows();
		float targetWidth = AucDis((float)metaData.mPoints[0].x, (float)metaData.mPoints[0].y, (float)metaData.mPoints[1].x, (float)metaData.mPoints[1].y);
		//Make sure that the width of the target is 300 pix
		rat = desiredTargetWidth / targetWidth;

		sz.width = round(sz.width*rat);
		sz.height = round(sz.height*rat);
		for (int i = 0; i < 4; ++i)
		{
			metaData.mPoints[i].x = round(metaData.mPoints[i].x*ratSmall2Large);
			metaData.mPoints[i].y = round(metaData.mPoints[i].y*ratSmall2Large);
		}
		metaData.mCenter.x = round(metaData.mCenter.x*ratSmall2Large);
		metaData.mCenter.y = round(metaData.mCenter.y*ratSmall2Large);
		//drawPolyRect(frame, metaData.mPoints, Scalar(255, 0, 17), 1);
		//cv::imshow("TargetOnFrame", frame);
		//cv::waitKey();
		metaData.ToFile(mdFileName);

		targetWidth = AucDis((float)metaData.mPoints[0].x, (float)metaData.mPoints[0].y, (float)metaData.mPoints[1].x, (float)metaData.mPoints[1].y);
		//Make sure that the width of the target is 300 pix
		rat = desiredTargetWidth / targetWidth;

		sz.width = round(largeSz.width*rat);
		sz.height = round(largeSz.height*rat);
		//Return to work after saving
		for (int i = 0; i < 4; ++i)
		{
			metaData.mPoints[i].x = round(metaData.mPoints[i].x*rat);
			metaData.mPoints[i].y = round(metaData.mPoints[i].y*rat);
		}
		metaData.mCenter.x = round(metaData.mCenter.x*rat);
		metaData.mCenter.y = round(metaData.mCenter.y*rat);
	}
	else
	{
		metaData.FromFile(mdFileName);
		std::cout << "Target:" << endl;
		for (Point p : metaData.mPoints)
		{
			std::cout << "(" << p.x << "," << p.y << ")" << endl;
		}
		std::cout << "Center: (" << metaData.mCenter.x << "," << metaData.mCenter.y << ")" << endl;

		float targetWidth = AucDis((float)metaData.mPoints[0].x, (float)metaData.mPoints[0].y, (float)metaData.mPoints[1].x, (float)metaData.mPoints[1].y);
		//Make sure that the width of the target is 300 pix
		rat = desiredTargetWidth / targetWidth;

		sz.width = round(largeSz.width*rat);
		sz.height = round(largeSz.height*rat);
		//
		//resize(frame, smallFrame, sz, 0, 0);
		//drawPolyRect(frame, metaData.mPoints, Scalar(255, 0, 17), 1);
		//cv::imshow("TargetOnFrame", frame);
		//
		for (int i = 0; i < 4; ++i)
		{
			metaData.mPoints[i].x = round(metaData.mPoints[i].x*rat);
			metaData.mPoints[i].y = round(metaData.mPoints[i].y*rat);
		}
		metaData.mCenter.x = round(metaData.mCenter.x*rat);
		metaData.mCenter.y = round(metaData.mCenter.y*rat);

		//
		//drawPolyRect(smallFrame, metaData.mPoints, Scalar(255, 0, 17), 1);
		//cv::imshow("TargetOnSmallFrame", smallFrame);
		//cv::waitKey();
		//
	}

	smallFrame = Mat(sz.height, sz.width, frame.type());
	resize(frame, smallFrame, sz, 0, 0);

	bool isToCrop = false;
	Rect cropRct;
	cropRct.x = 0;
	cropRct.y = 0;
	cropRct.width = sz.width;
	cropRct.height = sz.height;
	if (sz.width > 800)
	{
		isToCrop = true;
		cropRct.x = max(0, metaData.mPoints[0].x - margins);
		cropRct.width = metaData.mPoints[1].x - cropRct.x + margins;
		cropRct.y = 0;
		cropRct.height = sz.height;
		if (sz.height > 1000)
		{
			cropRct.y = max(0, metaData.mPoints[0].y - margins);
			cropRct.height = metaData.mPoints[3].y - cropRct.y + margins;
		}
		//Update the target positions
		for (int i = 0; i < 4; ++i)
		{
			metaData.mPoints[i].x -= cropRct.x;
			metaData.mPoints[i].y -= cropRct.y;
		}
		metaData.mCenter.x -= cropRct.x;
		metaData.mCenter.y -= cropRct.y;
	}
	smallFrame = smallFrame(cropRct);
	Size cropSz;
	cropSz.width = cropRct.width;
	cropSz.height = cropRct.height;
	Mat map(cropSz.height, cropSz.width, CV_8UC1);
	map.setTo(255);
	drawPolyRect(map, metaData.mPoints, Scalar(0), -1);
	
	int maxDiff = 0;
	double mn, mx;
	
	if (selectedCh >= 0)
	{
		Mat bgr[3];
		split(smallFrame, bgr);
		bgr[selectedCh].copyTo(firstFrame);
	}
	else
		cvtColor(smallFrame, firstFrame, COLOR_BGR2GRAY);

	//BITMAP bmp;
	//GetObject(imgBuffer, sizeof(BITMAP), &bmp);
	//int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8;
	//int depth = bmp.bmBitsPixel == 8 ? CV_8U : CV_8UC3;
	//Size bmpSz;
	//bmpSz.width = bmp.bmWidth;
	//bmpSz.height = bmp.bmHeight;
	//cv::Mat imgT(bmpSz, CV_MAKETYPE(depth, nChannels), bmp.bmBits);
	//resize(firstFrame, imgT, bmpSz);
	//memcpy(imgT.data, (char*)(bmp.bmBits), bmp.bmHeight*bmp.bmWidth*nChannels);


	//drawPolyRect(smallFrame, metaData.mPoints, Scalar(255, 0, 17), 1);
	//cv::imshow("TargetOnFrame", smallFrame);
	//cv::setMouseCallback("TargetOnFrame", mouse_callback);
	int fltrSz = 3;
	//cv::equalizeHist(firstFrame, firstFrame);
	blur(firstFrame, firstFrame, Size(fltrSz, fltrSz));
	firstFrame.copyTo(smallFrame);
	Rect rctMargin;
	int look = 20;
	rctMargin.x = look;
	rctMargin.y = look;
	rctMargin.width = cropSz.width - 2 * look;
	rctMargin.height = cropSz.height - 2 * look;
	//Mat target(cropSz.height, cropSz.width, CV_8UC1);
	Rect rectInBound = FindInboundRect(rctMargin, metaData.mPoints);
	std::cout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
	fout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
	//drawPolyRect(target, metaData.mPoints, Scalar(0), -1);
	//Mat firstGrad;

	//Mat mapNot;
	//bitwise_not(map, mapNot);

	//double ffMin = 0, ffMax = 0;
	//int ffMinIdx = 0, ffMaxIdx = 0;
	smallFrame.copyTo(firstFrame);

	//minMaxIdx(firstFrame, &ffMin, &ffMax, &ffMinIdx, &ffMaxIdx, mapNot);
	//int bg = cvFloor((ffMin + ffMax)*0.5);
	//cv::threshold(target, target, 1, 255, THRESH_BINARY);

	//Mat shiftMap = map(Rect(1, 1, sz.width - 1, sz.height - 1));

	//rectangle(target, rectInBound, Scalar(100));

	Mat shot = map.clone();

	//int thrOfGrad = 77;
	//double thr = 128 + 7;
	//int maxGrayLevelAllowed = 150;

	//resize(firstFrameThrs, firstFrameThrs, sz);
	//if (isToCrop)
	//	firstFrameThrs = firstFrameThrs(cropRct);
	//#undef min	cv::min(firstFrame, maxGrayLevelAllowed, firstFrame);
//	Canny(firstFrameThrs, firstGrad, thrOfGrad, 2.75 * thrOfGrad);
//	firstGrad.setTo(0, map);
	Mat shotsHistogramMat(cropSz.height, cropSz.width, CV_32SC1);
	Mat shotsFrameNumMat(cropSz.height, cropSz.width, CV_32SC1);
	shotsHistogramMat.setTo(1);
	shotsHistogramMat.setTo(0, map);
	shotsFrameNumMat.setTo(0);
	//cv::imshow("firstFrame", firstFrame);
	//cv::imshow("firstGrad", firstGrad);
	//cv::waitKey();
	//cv::threshold(firstGrad, firstGrad, thrOfGrad, 255, THRESH_BINARY);
	//Map all the contours of the first frame
//	vector<vector<Point> > contoursFirst;
//	vector<Vec4i> hierarchyFirst;
//	cv::findContours(firstGrad, contoursFirst, hierarchyFirst, RETR_CCOMP, CHAIN_APPROX_NONE);
	//Leave only large contours
//	int numOfFirstContours = (int)contoursFirst.size();
	//int idxMaxFirst = -1;
	//int rectAreaMaxFirst = 0;
	//int idxOfLargeInTheFirstArray = -1;
	//Point2f pntCgMaxFirst(0, 0);
	//vector<ContourData> cntrDataFirst;

	//if (numOfFirstContours > 0)
	//{
	//	int idx = 0;
	//	double arMax = 0;
	//	//for (; idx >= 0; idx = hierarchyFirst[idx][0])
	//	while (idx >= 0)
	//	{
	//		ContourData cd(contoursFirst[idx], cropSz, cntFrameNum, idx);
	//		if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
	//			continue;
	//		char buf[256] = { '\0' };
	//		sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
	//			cntFrameNum, idx, numOfFirstContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
	//		OutputDebugStringA(buf);
	//		if (cd.mLen > rectAreaMaxFirst)
	//		{
	//			rectAreaMaxFirst = cd.mLen;
	//			idxMaxFirst = idx;
	//			pntCgMaxFirst = cd.mCg;
	//			idxOfLargeInTheFirstArray = (int)cntrDataFirst.size();
	//		}
	//		//if (cntFrameNum == 160)
	//		//{
	//		//	shot.setTo(0);
	//		//	drawContours(shot, contoursFirst, idx, 255, 0, 8, hierarchyFirst);
	//		//	cv::setMouseCallback("shot", mouse_callback, &metaData);
	//		//	cv::imshow("cntr", shot);
	//		//	cv::waitKey();
	//		//}

	//		//if (cd.mRatioFromAll < 0.0001)//too small delete it
	//		//{
	//		//	contoursFirst.erase(contoursFirst.begin() + idx);
	//		//	--idx;
	//		//}
	//		//else
	//		{
	//			cntrDataFirst.push_back(cd);
	//		}
	//		if (idx < 0)
	//			break;
	//		idx = hierarchyFirst[idx][0];
	//	}
	//}
	//if (idxOfLargeInTheFirstArray < 0)
	//	return -1;
	//int numOfInitCntr = (int)cntrDataFirst.size();
	//for (int i = 0; i < numOfInitCntr; ++i)
	//{
	//	cntrDataFirst[i].SetDistFromLargeCorners(cntrDataFirst[idxOfLargeInTheFirstArray].mCorners);
	//	cntrDataFirst[i].SetDistFromLargeCenter(cntrDataFirst[idxOfLargeInTheFirstArray].mCg);
	//}
	Mat firstFrameSmooth;
	int kernelSz = (int)floor(0.05 * sz.width);
	blur(firstFrame, firstFrameSmooth, Size(kernelSz, kernelSz));

	//imshow("firstFrame", firstFrame);
	//imshow("firstFrameSmooth", firstFrameSmooth);
	//waitKey();
	//Mat grad8Thr, 
	Mat matAdpt(cropSz.height, cropSz.width, CV_8UC1);
	Mat frameRgb;
	Mat frameRgbDisplayed;
	shot.setTo(0);
	//double measureSharpnessThr = 400.0f;// 350.0f;
	vector<Rect> foundShotRects;
	vector<Scalar> colors;
	//vector<ContourData> shotsCand;
	bool isToBreak = false;
	bool isToSave = false;
	bool isInspectNms = false;
	bool isFromFile = false && !isToSave;
	if (isToSave)
	{
		string folderCreateCommand = "mkdir \"" + dirName + fName + "\"";
		system(folderCreateCommand.c_str());
	}
	vector< std::chrono::steady_clock::time_point> timing;
	double toMsec = 1000000000.0;
	std::stringstream bufTime;
	int sumX = 0, sumY = 0, x = 0, y = 0;
	while (1)
	{
		Trace("****t01*****");
		if (!isFromFile)
		{
			//shotsCand.resize(0);
			// Capture frame-by-frame
			cap >> frame;
			cntFrameNum++;
			std::cout << cntFrameNum << endl;

			if (frame.size().height == 0)
				break;
			//if (cntFrameNum < 1475) continue;

			resize(frame, frameRgb, sz);
			if(abs(x)<6 && abs(y)<6)
				smallFrame.copyTo(prevFrame);

			if (isToCrop)
				frameRgb = frameRgb(cropRct);
			if (selectedCh >= 0)
			{
				Mat bgr[3];
				split(frame, bgr);
				resize(bgr[selectedCh], smallFrame, sz);
			}
			else
			{
				cvtColor(frame, frame, COLOR_BGR2GRAY);
				resize(frame, smallFrame, sz);
			}

			if (isToCrop)
				smallFrame = smallFrame(cropRct);
			if (rot != 0)
			{
				transpose(smallFrame, smallFrame);
			}

			if (cntFrameNum == 0)
				continue;
			// If the frame is empty, break immediately
			if (smallFrame.empty())
				break;
			//cv::equalizeHist(smallFrame, smallFrame);
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
			cntFrameNum = 764; 
			std::stringstream buf;
			buf << dirName << fName << "/" << cntFrameNum << ".bmp";
			smallFrame = imread(buf.str());
			cvtColor(smallFrame, smallFrame, COLOR_BGR2GRAY);
		}
		Trace("****t02*****");		
		FindMovment(firstFrame, smallFrame, x, y, rectInBound, look, false, false);
		Trace("****t03*****");
		Point pntMov(x, y);
		//Rect in the current frame after move correction, corresponds to rctInFirstFrame
		Rect rctMove;
		rctMove.x = pntMov.x + look;
		rctMove.y = pntMov.y + look;
		rctMove.width = cropSz.width - rctMove.x - look;
		rctMove.height = cropSz.height - rctMove.y - look;

		Rect rctInFirstFrame;
		rctInFirstFrame.x = look;
		rctInFirstFrame.y = look;
		rctInFirstFrame.width = rctMove.width;
		rctInFirstFrame.height = rctMove.height;

		Mat mapMove(cropSz.height, cropSz.width, CV_8UC1);
		mapMove.setTo(255);
		Point pointsRect[4];

		for (int p = 0; p < 4; ++p)
		{
			pointsRect[p] = metaData.mPoints[p] + pntMov;
		}
		drawPolyRect(mapMove, pointsRect, Scalar(0), -1);

		Mat smallFrameCalib = smallFrame(rctMove);
		Mat firstFrameCalib = firstFrame(rctInFirstFrame);
		Mat diffFrame = smallFrameCalib - firstFrameCalib;
		threshold(smallFrame, matAdpt, 100, 255, THRESH_BINARY_INV);
		//adaptiveThreshold(smallFrame, matAdpt, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, (((int)floor(2*sz.width)) | 1), 0);
		Trace("****t04*****");

		matAdpt.setTo(0, mapMove);
		char buf[256] = { '\0' };
		sprintf_s(buf, "FindShot: F=%d move x=%d y=%d.\n", cntFrameNum, x, y);
		OutputDebugStringA(buf);

		if (isDebugMode)
		{
			cv::imshow("matAdptAfterClean", matAdpt);
			cv::waitKey();
		}
//		Canny(matAdpt, grad8Thr, 0, 255, 5, true);
//
//		timing.push_back(std::chrono::steady_clock::now());
//		std::cout << "Time difference before  t4 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//

//
//		//matDx.setTo(0, mapMove);
//		//matDy.setTo(0, mapMove);
//		//double mnx, mxx, mny, mxy;
//		//minMaxLoc(matDx, &mnx, &mxx);
//		//minMaxLoc(matDy, &mny, &mxy);
//		//double sharpMeasure = max(max(max(abs(mnx), abs(mxx)), abs(mny)), abs(mxy));
//		//if (sharpMeasure < measureSharpnessThr)
//		//{
//		//	std::cout << "Skipping " << cntFrameNum << " Blured frame " << sharpMeasure << endl;
//		//	//continue;//Blured image
//		//}
//
//

//		grad8Thr.setTo(0, mapMove);
//
//		vector<vector<Point> > contours;
//		vector<Vec4i> hierarchy;
//		timing.push_back(std::chrono::steady_clock::now());
//		bufTime<< "Time difference before  t5 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//		cv::findContours(grad8Thr, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE);
//
//		timing.push_back(std::chrono::steady_clock::now());
//		bufTime<< "Time difference before  t6 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//		int numOfContours = (int)contours.size();
//		char buf[256] = { '\0' };
//		sprintf_s(buf, "FindShot: F=%d move x=%d y=%d. Found cntr=%d\n", cntFrameNum, x, y, numOfContours);
//		OutputDebugStringA(buf);
//		/*********/
//		int idxMax = -1;
//		int rectAreaMax = 0;
//		Point2f pntCgMax(0, 0);
//
//		if (numOfContours > 0)
//		{
//			int idx = 0;
//			double arMax = 0;
//			vector<ContourData> cdsFrame;
//			int idxOfLargeInTheArray = -1;
//
//			timing.push_back(std::chrono::steady_clock::now());
//			bufTime<< "Time difference before  t7 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//			while (idx >= 0)
//			{
//				if (contours[idx].size() > 3)
//				{
//					ContourData cd(contours[idx], cropSz);
//					if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
//						continue;
//					//shot.setTo(0);
//					//polylines(shot, cd.mContour, true, 255, 1, 8);
//					//cv::setMouseCallback("shot", mouse_callback, &metaData);
//					//cv::imshow("cntr", shot);
//					//cv::waitKey();
//					//char buf[256] = { '\0' };
//					//sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
//					//	cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
//					//OutputDebugStringA(buf);
//					CalcAverageBorderColor(smallFrame, cd);
//					if (cd.mLen > rectAreaMax)
//					{
//						rectAreaMax = cd.mLen;
//						idxMax = idx;
//						pntCgMax = cd.mCg;
//						idxOfLargeInTheArray = (int)cdsFrame.size();
//					}
//					//if (cd.mLen < 10 /*|| (cd.mShRct.width > 25 && cd.mShRct.height > 25)*/)//too small delete it
//					//{
//					//	contours.erase(contours.begin() + idx);
//					//	--idx;
//					//	numOfContours--;
//					//}
//					//if (cd.mLen >= 7)
//					cdsFrame.push_back(cd);
//				}
//				if (idx < 0)
//					break;
//				idx = hierarchy[idx][0];
//			}
//			timing.push_back(std::chrono::steady_clock::now());
//			bufTime<< "Time difference before  t8 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//			/*If a n out of focus frame than the contours will break, skip these frames*/
//			if (rectAreaMax / (float)rectAreaMaxFirst < 0.65)
//			{
//				//std::cout <<"FindShot:*****Skipping "<< cntFrameNum<<" area "<< rectAreaMax / (float)rectAreaMaxFirst << endl;
//				//cv::imshow("gradThr", grad8Thr);
//				//cv::imshow("skipping", frameRgb);
//				//cv::waitKey();
//				//continue;
//			}
//			//If the contour is repeating itself from the inside, return the inside contour
//			vector<Point> rpt = cdsFrame[idxOfLargeInTheArray].FixSlightlyOpenContour();
//			if ((int)rpt.size() > 0)
//			{
//				cdsFrame.push_back(ContourData(rpt,
//					cdsFrame[idxOfLargeInTheArray].mPicSize,
//					cdsFrame[idxOfLargeInTheArray].mFrameNum,
//					(int)cdsFrame.size()));
//			}
//			/*This is the movment between the frames*/
//			int cgXmov = abs(cdsFrame[idxOfLargeInTheArray].mCg.x - cntrDataFirst[idxOfLargeInTheFirstArray].mCg.x);
//			int cgYmov = abs(cdsFrame[idxOfLargeInTheArray].mCg.y - cntrDataFirst[idxOfLargeInTheFirstArray].mCg.y);
//			//if (abs(x) > 10 || abs(y) > 10 || cgXmov > 10 || cgYmov > 10)
//			//{
//			//	bufTime<< "Skipping" << endl;
//			//	//continue;
//			//}
//			timing.push_back(std::chrono::steady_clock::now());
//			bufTime<< "Time difference before  t9 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//			for (idx = 0; idx < (int)cdsFrame.size(); idx++)
//			{
//				cdsFrame[idx].mFrameNum = cntFrameNum;
//				cdsFrame[idx].mIdxCntr = idx;
//				cdsFrame[idx].SetDistFromLargeCenter(pntCgMax);
//				ContourData cd = cdsFrame[idx];
//				CalcAverageRectInOutColor(smallFrame, cd);
//				if (cd.mShRct.width == 0 || cd.mShRct.height == 0)
//					continue;
//
//				char buf[256] = { '\0' };
//				sprintf_s(buf, "FindShot: **** F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
//					cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
//				OutputDebugStringA(buf);
//				if (cntFrameNum == -764)// && idx == -10)// && idxFirst == 7)
//				{
//					shot.setTo(0);
//					cv::rectangle(shot, cdsFrame[idxOfLargeInTheArray].mShRct, 255);
//					cv::rectangle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mShRct, 128);
//					polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 200, 1, 8);
//					polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 80, 1, 8);
//					polylines(shot, cd.mContour, true, 255, 1, 8);
//					circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
//					circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
//					cv::imshow("matAdpt", matAdpt);
//					cv::imshow("cntrIn", shot);
//					cv::imshow("Frame", smallFrame);
//					cv::imshow("grad8Thr", grad8Thr);
//					cv::waitKey();
//				}
//				timing.push_back(std::chrono::steady_clock::now());
//				bufTime<< "Time difference before  t91 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//				if (IsItShot(cd))
//				{
//					/*Need to go over the first contours compare its cg and MatchShape and see if this one is new, if yes add it to the list*/
//
///*************************/
//					bool isFound = false;
//					int idxFirst = 0;
//					//for (; idxFirst < (int)cntrDataFirst.size(); idxFirst++)
//					//{
//					//	ContourData cdf = cntrDataFirst[idxFirst];
//					//	//shot.setTo(0);
//					//	//polylines(shot, cd.mContour, true, 255, 1, 8);
//					//	//polylines(shot, cdf.mContour, true, 128, 1, 8);
//					//	//cv::setMouseCallback("shot", mouse_callback, &metaData);
//					//	//cv::imshow("cntr", shot);
//					//	//cv::imshow("grad", grad8Thr);
//					//	//cv::waitKey();
//					//	//std::cout << idxFirst << endl;
//					//	//if (cntFrameNum == -256 && idx == 10 && idxFirst == 2)
//					//	//{
//					//	//	shot.setTo(0);
//					//	//	//polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
//					//	//	//polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
//					//	//	std::cout << "Check " << idx << " with " << idxFirst << endl;
//					//	//	polylines(shot, cd.mContour, true, 255, 1, 8);
//					//	//	polylines(shot, cdf.mContour, true, 128, 1, 8);
//					//	//	//circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
//					//	//	//circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
//					//	//	cv::imshow("gradThr", grad8Thr);
//					//	//	cv::imshow("cntrIn", shot);
//					//	//	cv::waitKey();
//					//	//}
//
//					//	vector<ContourData> cdsResidu;
//					//	if (cd.CompareContourAndReturnResidu(cdf, cdsResidu, &pntMov) == true)//returns true if equal
//					//	{
//					//		if (cd.mLen > cdf.mLen && cdf.mLen > 1 && cd.mLen / (float)cdf.mLen < 1.25)
//					//		{
//					//			for (int cdResIdx = 0; cdResIdx < (int)cdsResidu.size(); ++cdResIdx)
//					//			{
//					//				if (cdsResidu[cdResIdx].mLen > 10 && cdsResidu[cdResIdx].mLen < cdf.mLen && cdsResidu[cdResIdx].mLen < cd.mLen)
//					//				{
//					//					cdsResidu[cdResIdx].SetDistFromLargeCenter(pntCgMax);
//					//					CalcAverageRectInOutColor(smallFrame, cdsResidu[cdResIdx]);
//					//					if (IsItShot(cdsResidu[cdResIdx]))
//					//					{
//					//						//shot.setTo(0);
//					//						//Mat s1, s2, s3;
//					//						//shot.copyTo(s1); shot.copyTo(s2); shot.copyTo(s3);
//					//						//ContourData cdTemp(cd - pntMov);
//					//						////polylines(shot, cdsFrame[idxOfLargeInTheArray].mContour, true, 255, 1, 8);
//					//						////polylines(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mContour, true, 128, 1, 8);
//					//						//polylines(shot, cdTemp.mContour, true, 255, 1, 8);
//					//						//polylines(shot, cdf.mContour, true, 128, 1, 8);
//					//						//polylines(shot, cdsResidu[cdResIdx].mContour, true, 200, 1, 8);
//					//						//polylines(s1, cdTemp.mContour, true, 255, 1, 8);
//					//						//polylines(s2, cdf.mContour, true, 128, 1, 8);
//					//						//polylines(s3, cdsResidu[cdResIdx].mContour, true, 200, 1, 8);
//
//					//						////circle(shot, cdsFrame[idxOfLargeInTheArray].mCg, 3, 255);
//					//						////circle(shot, cntrDataFirst[idxOfLargeInTheFirstArray].mCg, 3, 128);
//					//						//cv::imshow("gradThr", grad8Thr);
//					//						//cv::imshow("cntrAndCntrIn", shot);
//					//						//cv::imshow("s1", s1);
//					//						//cv::imshow("s2", s2);
//					//						//cv::imshow("s3", s3);
//					//						//cv::waitKey();
//					//						cdsFrame.push_back(cdsResidu[cdResIdx]);
//					//					}
//					//				}
//					//			}
//					//		}
//					//		isFound = true;
//					//		//cntrDataFirst.push_back(cd + pntMov);
//					//		break;
//					//	}
//					//}
//					timing.push_back(std::chrono::steady_clock::now());
//					bufTime<< "Time difference before  92 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//					if (!isFound && cd.mLen < 200)
//					{
//						//cd = cd + pntMov;
//						shotsCand.push_back(cd);
//						//cntrDataFirst.push_back(cd);
//						shot.setTo(0);
//						//for (int i=0; i < shotsCand.size(); i++)
//						//{
//						//	polylines(shot, shotsCand[i].mContour, true, 255, 1, 8);
//						//}
//						polylines(shot, cd.mContour, true, 255, 1, 8);
//						char buf[256] = { '\0' };
//						sprintf_s(buf, "FindShot: New F=%d, Cntr=%d:%d, Area=%f,W=%d,H=%d,rat=%f, Pos(%d,%d),%d,%f\n",
//							cntFrameNum, idx, numOfContours, cd.mAr, cd.mShRct.width, cd.mShRct.height, cd.mRatioWh, cd.mCg.x, cd.mCg.y, cd.mLen, cd.mRatioFromAll);
//						OutputDebugStringA(buf);
//						//cv::imshow("gradThr", grad8Thr);
//						//cv::imshow("Frame", smallFrame);
//						//cv::imshow("cntr", shot);
//						//cv::imshow("gradMat", matAdpt);
//						//cv::waitKey();
//						//std::cout << "Num of shots in frame " << cntFrameNum << " is " << shotsCand.size() << endl;
//						//cntrDataFirst.push_back(cd);
//					}
//					timing.push_back(std::chrono::steady_clock::now());
//					bufTime<< "Time difference before  93 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//				}
//			}
//		}
//		//if (isFromFile && isInspectNms)
//		//{
//		//	Mat matToDraw(sz.height, sz.width, CV_8UC1);		NMS(shotsCand, smallFrame, pntMov,&matToDraw);
//		//}
//		//else
//		//	NMS(shotsCand,smallFrame, pntMov);
//		int numOfShotsFound = (int)shotsCand.size();
//		bufTime<< "Num of shots after NMS in frame " << cntFrameNum << " is ," << numOfShotsFound << endl;
//		fout << "Num of shots after NMS in frame " << cntFrameNum << " is ," << numOfShotsFound << endl;
//		shot.setTo(0);
//		timing.push_back(std::chrono::steady_clock::now());
//		bufTime<< "Time difference before  t10 " << (*(timing.end() - 1) - *(timing.end()-2)).count() << std::endl;
//
//		for (int rc = 0; rc < numOfShotsFound; ++rc)
//		{
//			Rect rct(shotsCand[rc].mShRct);
//			//polylines(shot, shotsCand[rc].mContour, true, 1, -1, 8);
//
//			//rct.x -= x;
//			//rct.y -= y;
//			//rectangle(frameRgb, rct, colors[rc], 2);
//			if (isDebugMode)
//			{
//				if (!isFromFile)
//				{
//					if (rc == numOfShotsFound - 1)
//						rectangle(frameRgb, rct, Scalar(0, 255, 0), 1);
//					else
//						rectangle(frameRgb, rct, Scalar(255, 0, 0), 1);
//				}
//				else
//				{
//					rectangle(smallFrame, rct, Scalar(10, 0, 0), 1);
//					//rectangle(matAdpt, rct, Scalar(128, 0, 0), 1);
//					//cv::imshow("Frame", smallFrame);
//					//cv::waitKey();
//				}
//			}
//			//std::stringstream buf;
//			//buf << dirName << fName << "\\"<<cntFrameNum<<"marks.bmp";
//			//imwrite(buf.str(), frameRgb);
//			//Copy the shot to the shots map, this map will be added to the totla map
//			rct = rct - pntMov;
//			Mat pMatThr = matAdpt(shotsCand[rc].mShRct);
//			Mat pMatShot = shot(rct);
//			pMatThr.copyTo(pMatShot);
//			bitwise_not(pMatShot, pMatShot);
//		}
		timing.push_back(std::chrono::steady_clock::now());
		bufTime<< "Time difference before  t11 " << (*(timing.end() - 1) - *(timing.end()-2)).count()/ toMsec << std::endl;
		Trace("****t05*****");
		//if (numOfShotsFound > 0)
		{
			Mat shot32;
			Mat matAdptMov = matAdpt(rctMove);
			matAdptMov.convertTo(shot32, shotsHistogramMat.type(), 1.0 / 255.0);
			//shot.convertTo(shot32, shotsHistogramMat.type(), 1.0 / 255.0);
			Mat shotsHistMove = shotsHistogramMat(rctInFirstFrame);
			shotsHistMove += shot32;

			Mat shotFrameMove = shotsFrameNumMat(rctInFirstFrame);
			/*Mark the shots pixels with the frame num*/
			for (int r = 0; r < rctInFirstFrame.height; ++r)
			{
				for (int c = 0; c < rctInFirstFrame.width; ++c)
				{
					auto val = matAdptMov.at<UCHAR>(r, c);
					auto valFrameNum = shotFrameMove.at<int>(r, c);
					if (val > 0 && valFrameNum == 0)
					{
						shotFrameMove.at<int>(r, c) = cntFrameNum;
					}
				}
			}
		}
		if (!isFromFile && isDebugMode)
		{
			frameRgb.copyTo(frameRgbDisplayed);
			cv::imshow("SHOTS", frameRgbDisplayed);
			//cv::imshow("PrevFrame", prevFrame);
		}
		Trace("****t06*****");
		double mn16, mx16;
		minMaxLoc(shotsHistogramMat, &mn16, &mx16);
		shotsHistogramMat.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
		cv::imshow("shotsHistogramMat", shot);
		//minMaxLoc(shotsFrameNumMat, &mn16, &mx16);
		shotsFrameNumMat.convertTo(shot, shot.type(), 255.0 / cntFrameNum);
		cv::imshow("shotsFrameNumMat", shot);
		cv::imshow("Frame", smallFrame);
		minMaxLoc(diffFrame, &mn16, &mx16);
		diffFrame.convertTo(shot, shot.type(), 255.0 / max(1.0, mx16));
		cv::imshow("diffFrame", diffFrame);

		
		if (isDebugMode)
		{
			cv::imshow("firstFrame", firstFrame);
			cv::imshow("matAdpt", matAdpt);
			//cv::imshow("gradThr", grad8Thr);
			//cv::setMouseCallback("gradThr", mouse_callback, (void*)&grad8Thr);
		}
		Trace("****t07*****");
		//OutputDebugStringA(str.c_str());
		// Press  ESC on keyboard to exit
		if (0 && !isFromFile)//cntFrameNum > 680)//isToBreak)//
		{
			isToBreak = false;
			char c = (char)cv::waitKey();
			if (c == 27)
				break;
		}
		else if (!isFromFile)
			waitKey(10);
		//else
		//	break;
	}

	std::stringstream bufH, bufT;
	bufH << dirName << fName;
	if (!std::experimental::filesystem::exists(bufH.str()))
	{
		std::experimental::filesystem::create_directories(bufH.str());
	}
	bufH << "\\HistOfShots.xml";
	bufT << dirName << fName << "\\TimeOfShots.xml";
	cv::FileStorage fileHisto(bufH.str(), cv::FileStorage::WRITE);
	cv::FileStorage fileTime(bufT.str(), cv::FileStorage::WRITE);
	// Write to file!
	fileHisto << "shotsHistogramMat" << shotsHistogramMat;
	fileTime << "shotsFrameNumMat" << shotsFrameNumMat;
	fileHisto.release();
	fileTime.release();
	std::stringstream buf;
	buf << dirName << fName << "\\lastFrame.bmp";
	imwrite(buf.str(), prevFrame);

	//destroyAllWindows();
	if (!isFromFile && isDebugMode)
		cv::imshow("SHOTS", frameRgbDisplayed);
	//cv::waitKey();
	//cv::destroyAllWindows();
	fout.close();
	// When everything done, release the video capture object
	cap.release();


	return 0;
}


int main()
{
	using namespace cv;
	bool toDisplay = false;
	String dirName = "C:/moti/FindShoot/videos/";
	//String fName = "MVI_3";	String extName = ".MOV";
	//String fName = "MVI_4"; String extName = ".MOV";
	//String fName = "MVI_1"; String extName = ".MOV";
	//String fName = "MVI_2"; String extName = ".MOV";
	//String fName = "VID-20181125-WA0005"; String extName = ".mp4"; very bad video
	//String fName = "GH010005";	String extName = ".MP4";
	//String fName = "GH010010";	String extName = ".MP4";
	String fName = "20190510_093213";	String extName = ".mp4";//Samsung

	ofstream fout;
	fout.open(dirName + fName + ".csv");
	String fullFileName = dirName + fName + extName;
	HBITMAP imgBuffer;
	imgBuffer = NULL;
	return FindShoots(fullFileName.c_str(),1, imgBuffer, 1066, 800, 1);
}
