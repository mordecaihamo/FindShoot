#pragma once
#ifndef _FINDSHOOT_H
#define _FINDSHOOT_H
#include <Windows.h>

#define FINDSHOOTEXPORT __declspec(dllexport)
extern "C" FINDSHOOTEXPORT int Analyze(char* vidName, int isDebugMode);                           
extern "C" FINDSHOOTEXPORT int FindShoots(const char* vidName, int selectedCh, HBITMAP imgBuffer, int imgHeight, int imgWidth, int isDebugMode);

#endif