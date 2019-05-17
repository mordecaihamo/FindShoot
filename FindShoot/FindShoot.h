#pragma once
#ifndef _FINDSHOOT_H
#define _FINDSHOOT_H

#define FINDSHOOTEXPORT __declspec(dllexport)
extern "C" FINDSHOOTEXPORT int Analyze(char* vidName, int isDebugMode);
extern "C" FINDSHOOTEXPORT int FindShoots(char* vidName, unsigned char* imgBuffer, int imgHeight, int imgWidth, int isDebugMode = 1);

#endif