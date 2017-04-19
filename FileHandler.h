#pragma once
//FileHandler.h

#ifndef MY_FILEHANDLER
#define MY_FILEHANDLER

#include<fstream> 
#include<iostream>
#include <ctime>
#include <time.h>
#include <vector>

#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

void writeFileForBGS(string pictureName, Rect roi);
string getPictureName(int i);
Rect getRect(string pictureName);
time_t timeStamp(string pictureName);
string getActuallDate();


void readFile();

#endif

