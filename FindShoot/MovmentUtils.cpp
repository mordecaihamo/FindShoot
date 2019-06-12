#include "pch.h"
#include "MovmentUtils.h"
#include <vector>
#include "opencv2/highgui.hpp"
#include <iostream>

using namespace cv;

/**  @function Erosion  */
void Erosion(Mat &src, Mat &dst, int erosion_size, int erosion_type)
{
	int erosion_elem = 2;
	int dilation_elem = 0;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	
	//if (erosion_elem == 0) { erosion_type = MORPH_RECT; }
	//else if (erosion_elem == 1) { erosion_type = MORPH_CROSS; }
	//else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

	Mat element = getStructuringElement(erosion_type,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));

	/// Apply the erosion operation
	erode(src, dst, element);
}

/** @function Dilation */
void Dilation(Mat &src, Mat &dst, int dilation_size, int dilation_type)
{
	int erosion_elem = 2;
	int dilation_elem = 0;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	
	//if (dilation_elem == 0) { dilation_type = MORPH_RECT; }
	//else if (dilation_elem == 1) { dilation_type = MORPH_CROSS; }
	//else if (dilation_elem == 2) { dilation_type = MORPH_ELLIPSE; }

	Mat element = getStructuringElement(dilation_type,
		Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		Point(dilation_size, dilation_size));
	/// Apply the dilation operation
	dilate(src, dst, element);
}

void FindMovment(Mat& a, Mat& b, int& x, int& y, Rect& rct, int lookDis,bool isToZero, bool isToDisplay)
{
	int H = a.size().height;
	int W = a.size().width;
	Rect rctMov(rct);
	rctMov.height -= 2 * lookDis;
	rctMov.width -= 2 * lookDis;
	Mat ar = a(rctMov);
	rctMov.x += lookDis;
	rctMov.y += lookDis;
	Mat br = b(rctMov);
	Mat diff(rctMov.height, rctMov.width, CV_8U);
	diff.setTo(0);
	double dMn = DBL_MAX;
	int rMn = -1;
	int cMn = -1;
	for (int r = rct.y - lookDis + 1; r < rct.y + lookDis - 1; ++r)
	{
		rctMov.y = r;
		for (int c = rct.x - lookDis + 1; c < rct.x + lookDis - 1; ++c)
		{
			rctMov.x = c;
			Mat br = b(rctMov);
			diff = abs(ar - br);
			Scalar s = sum(diff);
			double d = s[0] + s[1] + s[2];
			if (isToDisplay)
			{
				cout << r << "(" << rct.y - lookDis + 1 << "," << rct.y + lookDis - 1 << ")" << endl;
				cout << c << "(" << rct.x - lookDis + 1 << "," << rct.x + lookDis - 1 << ")" << endl;
				cout << d << endl;
				imshow("ar", ar);
				imshow("br", br);
				imshow("diff", diff);
				waitKey();
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
		Mat br = b(rctMov);
		imshow("ar", ar);
		ar.setTo(0, br);
		imshow("br", br);
		imshow("arAfter", ar);
		waitKey();
	}
	x = cMn - rct.x;
	y = rMn - rct.y;
}

using namespace std;
Rect FindInboundRect(Rect rct, const Point* rectPoints)
{
	Rect rctInBound(rct);
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

void ThresholdByLightMap(Mat& inMat, Mat& outMat, Mat& lightMat, float percFromLight, Rect& inRect, Rect& lightRect)
{
	Mat img, l;
	inMat.copyTo(img);
	lightMat.copyTo(l);
	Size sz = lightMat.size();
	
	Mat l1 = img(inRect);
	Mat l2 = l(lightRect);
	
	l1 = l1 - l2 * percFromLight;
	threshold(img, outMat, 10.0f, 255, THRESH_BINARY_INV);
	//imshow("l1", l1);
	//imshow("l2", l2);
	imshow("I", inMat);
	imshow("O", outMat);
	imshow("L", l);
	imshow("D", img);
	//waitKey();
}