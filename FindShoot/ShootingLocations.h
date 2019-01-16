#pragma once
#ifndef _SHOOTINGLOCATIONS
#define	_SHOOTINGLOCATIONS
#include "opencv2/core.hpp"
#include "ShootTargetMetaData.h"

using namespace cv;
using namespace std;
class ShootingLocations
{
public:
	ShootingLocations();
	~ShootingLocations();
	ShootTargetMetaData mTargetData;
};

#endif

