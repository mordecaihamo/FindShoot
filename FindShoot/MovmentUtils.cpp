#include "pch.h"
#include "MovmentUtils.h"
#include <vector>
#include "opencv2/highgui.hpp"
//#include <iostream>
#include <windows.h>

//using namespace cv;

/**  @function Erosion  */
void Erosion(cv::Mat &src, cv::Mat &dst, int erosion_size, int erosion_type)
{
	int erosion_elem = 2;
	int dilation_elem = 0;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	
	//if (erosion_elem == 0) { erosion_type = MORPH_RECT; }
	//else if (erosion_elem == 1) { erosion_type = MORPH_CROSS; }
	//else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

	cv::Mat element = getStructuringElement(erosion_type,
		cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		cv::Point(erosion_size, erosion_size));

	/// Apply the erosion operation
	erode(src, dst, element);
}

/** @function Dilation */
void Dilation(cv::Mat &src, cv::Mat &dst, int dilation_size, int dilation_type)
{
	int erosion_elem = 2;
	int dilation_elem = 0;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	
	//if (dilation_elem == 0) { dilation_type = MORPH_RECT; }
	//else if (dilation_elem == 1) { dilation_type = MORPH_CROSS; }
	//else if (dilation_elem == 2) { dilation_type = MORPH_ELLIPSE; }

	cv::Mat element = getStructuringElement(dilation_type,
		cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		cv::Point(dilation_size, dilation_size));
	/// Apply the dilation operation
	dilate(src, dst, element);
}

void FindMovment(cv::Mat& a, cv::Mat& b, int& x, int& y, cv::Rect& rct, int lookDis,bool isToZero, bool isToDisplay)
{
	int H = a.size().height;
	int W = a.size().width;
	cv::Rect rctMov(rct);
	rctMov.height -= 2 * lookDis;
	rctMov.width -= 2 * lookDis;
	cv::Mat ar = a(rctMov);
	rctMov.x += lookDis;
	rctMov.y += lookDis;

	cv::Mat br = b(rctMov);
	cv::Mat diff(rctMov.height, rctMov.width, CV_8U);
	diff.setTo(0);
	double dMn = DBL_MAX;
	int rMn = -1;
	int cMn = -1;
	int yst = rct.y + y - lookDis + 1;
	if (yst < 0)
		yst = 0;
	int yed = rct.y + y + lookDis - 1;
	if (yed >= H)
		yed = H - 1;
	int xst = rct.x + x - lookDis + 1;
	if (xst < 0)
		xst = 0;
	int xed = rct.x + x + lookDis - 1;
	if (xed >= W)
		xed = W - 1;

	for (int r = yst; r < yed; ++r)
	{
		rctMov.y = r;
		for (int c = xst; c < xed; ++c)
		{
			rctMov.x = c;
			cv::Mat br = b(rctMov);
			diff = abs(ar - br);
			cv::Scalar s = sum(diff);
			double d = s[0] + s[1] + s[2];
			if (isToDisplay)
			{
				char buf[512] = { '\0' };
				sprintf_s(buf, "Move (%d,%d)\n", c,r);
				OutputDebugStringA(buf);
				sprintf_s(buf, "(%d,%d)\n", rctMov.y - lookDis + 1, rctMov.y + lookDis - 1);
				OutputDebugStringA(buf);
				sprintf_s(buf, "(%d,%d)\n", rctMov.x - lookDis + 1, rctMov.x + lookDis - 1);
				OutputDebugStringA(buf);
				sprintf_s(buf, "=%f\n", d);
				OutputDebugStringA(buf);
				imshow("ar", ar);
				imshow("br", br);
				imshow("diff", diff);
				cv::waitKey();
			}
			if (d < dMn)
			{
				dMn = d;
				rMn = r;
				cMn = c;
			}
		}
	}
	if (isToZero)
	{
		rctMov.x = cMn;
		rctMov.y = rMn;
		cv::Mat br = b(rctMov);
		imshow("ar", ar);
		ar.setTo(0, br);
		imshow("br", br);
		imshow("arAfter", ar);
		cv::waitKey();
	}
	x = cMn - rct.x;
	y = rMn - rct.y;
}

using namespace std;
cv::Rect FindInboundRect(cv::Rect rct, const cv::Point* rectPoints)
{
	cv::Rect rctInBound(rct);
	vector<int> x(4,0), y(4,0);
	for (int i = 0; i < 4; ++i)
	{
		x[i] = rectPoints[i].x;
		y[i] = rectPoints[i].y;
	}

	sort(x.begin(), x.end());
	sort(y.begin(), y.end());

	//Check that all points are inside rect if not add them
	if (x[1] < rct.x)
		x[1] = rct.x;
	if (y[1] < rct.y)
		y[1] = rct.y;

	if (x[2] >= rct.x + rct.width)
		x[2] = rct.x + rct.width - 1;
	if (y[2] >= rct.y + rct.height)
		y[2] = rct.y + rct.height - 1;

	rctInBound.x = x[1];
	rctInBound.y = y[1];
	rctInBound.width = x[2] - x[1];
	rctInBound.height = y[2] - y[1];

	return rctInBound;
}

void ThresholdByLightMap(cv::Mat& inMat, cv::Mat& outMat, cv::Mat& lightMat, float percFromLight, cv::Rect& inRect, cv::Rect& lightRect, bool isDebug)
{
	cv::Mat img, l;
	inMat.copyTo(img);
	lightMat.copyTo(l);
	cv::Size sz = lightMat.size();

	cv::Mat l1 = img(inRect);
	cv::Mat l2 = l(lightRect);
	if (isDebug)
	{
		imshow("l1", l1);
	}
	cv::Mat l3 = l1 - l2 * percFromLight;
	img.setTo(0);
	l3.copyTo(l1);
	threshold(img, outMat, 1.0f, 255, cv::THRESH_BINARY_INV);
	if (isDebug)
	{
		imshow("l2", l2);		
		imshow("I", inMat);
		imshow("O", outMat);
		imshow("L", l);
		imshow("D", img);
		threshold(l1, l3, 1.0f, 255, cv::THRESH_BINARY_INV);
		imshow("l3", l3);
		cv::waitKey();
	}
}