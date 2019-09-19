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

	String lastFramePath = dirName + fName + "/lastFrame.bmp";
	String mdFileName = dirName + fName + ".txt";

	
	String s1 = dirName + fName + "/HistOfShots.xml";
	String s2 = dirName + fName + "/TimeOfShots.xml";
	String s3 = dirName + fName + "/ShotsResults.csv";
	String s4 = dirName + fName + "/xyMove.csv";
	AnalyzeShotsResult ana(s1, s2, mdFileName,lastFramePath,s4);
	int res = -1;
	if (ana.GetWidth() > 0 && ana.GetHeight() > 0)
	{
		res = ana.Compute(s3, isDebugMode);
	}
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
	resize(frame, smallFrame, sz, 0, 0);
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
	Mat mapNot;
	bitwise_not(map, mapNot);
	
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

	firstFrame.setTo(0, map);
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
	
	Rect rectInBound = FindInboundRect(rctMargin, metaData.mPoints);
	//std::cout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
	fout << rectInBound.x << "," << rectInBound.y << "," << rectInBound.width << "," << rectInBound.height << "," << map.cols << "," << map.rows << endl;
	smallFrame.copyTo(firstFrame);

	Mat shot = map.clone();

	Mat shotsHistogramMat(cropSz.height, cropSz.width, CV_32SC1);
	Mat shotsFrameNumMat(cropSz.height, cropSz.width, CV_32SC1);
	shotsHistogramMat.setTo(1);
	shotsHistogramMat.setTo(0, map);
	shotsFrameNumMat.setTo(0);
	//cv::imshow("firstFrame", firstFrame);
	//cv::imshow("firstGrad", firstGrad);
	//cv::waitKey();
	Mat firstFrameSmooth;
	int kernelSz = (int)floor(0.01 * sz.width);
	blur(firstFrame, firstFrameSmooth, Size(kernelSz, kernelSz));

	//imshow("firstFrame", firstFrame);
	//imshow("firstFrameSmooth", firstFrameSmooth);
	//waitKey();
	Mat matAdpt(cropSz.height, cropSz.width, CV_8UC1);
	Mat frameRgb;
	Mat frameRgbDisplayed;
	shot.setTo(0);

	bool isToBreak = false;
	bool isToSave = false;
	bool isInspectNms = false;
	bool isFromFile = false && !isToSave;
	if (isToSave)
	{
		string folderCreateCommand = "mkdir \"" + dirName + fName + "\"";
		system(folderCreateCommand.c_str());
	}
	
	int x = 0, y = 0;
	int xLastFrame = 0, yLastFrame = 0;
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

			resize(frame, frameRgb, sz);
			if (abs(x) < 25 && abs(y) < 25)
			{
				smallFrame.copyTo(prevFrame);
				xLastFrame = x;
				yLastFrame = y;
			}

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
		if (cntFrameNum < 100000000000766)
			FindMovment(firstFrame, smallFrame, x, y, rectInBound, look, false, false);
		else
			FindMovment(firstFrame, smallFrame, x, y, rectInBound, look, false, true);
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
		//Zero the area outside the target after move correction
		smallFrame.setTo(0, mapMove);
		//smallFrame - (firstFrameSmooth*0.7) > 1 => matAdpt [done with move correction]
		if (cntFrameNum < 100000000000766)
			ThresholdByLightMap(smallFrame, matAdpt, firstFrameSmooth, 0.7, rctMove, rctInFirstFrame);
		else
			ThresholdByLightMap(smallFrame, matAdpt, firstFrameSmooth, 0.7, rctMove, rctInFirstFrame, true);
		//The pixels than survived the threshold are shot cand and will be added to the histogram.
		//We want to delete the false one that were generated from little movments between the frames.
		//We will do that by checking if 


		matAdpt.setTo(0, mapMove);
		char buf[256] = { '\0' };
		sprintf_s(buf, "FindShot: F=%d move x=%d y=%d.\n", cntFrameNum, x, y);
		OutputDebugStringA(buf);

		//if (isDebugMode )
		{
			cv::imshow("I", smallFrame);
			cv::imshow("firstFrameSmooth", firstFrameSmooth);
			cv::imshow("matAdptAfterClean", matAdpt);
			cv::waitKey();
		}
			
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
		
		if (isDebugMode)
		{
			cv::imshow("firstFrame", firstFrame);
			cv::imshow("matAdpt", matAdpt);
			//cv::imshow("gradThr", grad8Thr);
			//cv::setMouseCallback("gradThr", mouse_callback, (void*)&grad8Thr);
		}
		Trace("****t07*****");
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

	std::stringstream bufH, bufT, bufMov;
	bufH << dirName << fName;
	if (!std::experimental::filesystem::exists(bufH.str()))
	{
		std::experimental::filesystem::create_directories(bufH.str());
	}
	bufH << "\\HistOfShots.xml";
	bufT << dirName << fName << "\\TimeOfShots.xml";
	bufMov << dirName << fName << "\\xyMove.csv";
	cv::FileStorage fileHisto(bufH.str(), cv::FileStorage::WRITE);
	cv::FileStorage fileTime(bufT.str(), cv::FileStorage::WRITE);
	ofstream outStream(bufMov.str(), std::ofstream::out);
	outStream << xLastFrame << " " << yLastFrame << endl;
	outStream.close();
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
