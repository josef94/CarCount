// main.cpp

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "Blob.h"
#include "FileHandler.h"

using namespace std;
using namespace cv;

// global variables ///////////////////////////////////////////////////////////////////////////////

// PATH //
string path = "../";
string videoName = "video_170330_2.avi";
string pathSaveROIPicture = "../Crops/";

int countSaveROIPicture = 0;
bool savedROI = false;
char file[256];
int pixX1 = -1;
int pixX2 = -1;
double fps;

// function prototypes ////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(vector<Blob> &existingBlobs, vector<Blob> &currentFrameBlobs);
void addBlobToExistingBlobs(Blob &currentFrameBlob, vector<Blob> &existingBlobs, int &intIndex);
void addNewBlob(Blob &currentFrameBlob, vector<Blob> &existingBlobs);
double distanceBetweenPoints(Point point1, Point point2);
bool checkIfBlobsCrossedTheLineRightToLeft(vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount);
bool checkIfBlobsCrossedTheLineLeftToRight(vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount);
int saveRegionOfInterest(vector<Blob> &blobs, Mat &imgFrame2Copy, bool countVehicle);
double calculateVelocity(int pixX1, int pixX2);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(void) {
	//// VARIABLES ////
	VideoCapture capVideo;
	Mat imgFrame1, imgFrame2, imgDifference, imgFrame1Copy, imgFrame2Copy, structuringElement5x5;
	vector<Blob> blobs, currentFrameBlobs;
	bool blnFirstFrame = true;
	int thresholdValue = 30;
	vector<vector<Point> > contours;

	// CarCount
	int countPic = 0;
	Point crossingLine[2];
	int countLinePosition;
	int carCountLeft = 0;
	int carCountRight = 0;

	// Speed
	//double speed = 0;


	//// PROGRAM START ////

	//capVideo.open(0)   // for live video
	capVideo.open(path + videoName); 	// for saved video

	if (!capVideo.isOpened()) {                                                 // if unable to open video file
		cout << "error reading video file" << endl << endl;      // show error message
		return(0);                                                              // and exit program
	}else{
		cout << "start of video\n";
	}

	capVideo.read(imgFrame1);
	capVideo.read(imgFrame2);
	fps = capVideo.get(CV_CAP_PROP_FPS);

	// Position of countingline
	countLinePosition = (int)round((double)imgFrame1.cols * 0.5);
	crossingLine[0].x = countLinePosition;
	crossingLine[0].y = 0;
	crossingLine[1].x = countLinePosition;
	crossingLine[1].y = imgFrame1.rows - 1;

	// go trough, frame by frame
	while (capVideo.isOpened()) {
		countPic++;
		if (countPic == 20) {
			string s = pathSaveROIPicture + "bgs.tif";
			imwrite(s, imgFrame1);
		}

		imgFrame1Copy = imgFrame1.clone();
		imgFrame2Copy = imgFrame2.clone();

		cvtColor(imgFrame1Copy, imgFrame1Copy, CV_BGR2GRAY);
		cvtColor(imgFrame2Copy, imgFrame2Copy, CV_BGR2GRAY);
		GaussianBlur(imgFrame1Copy, imgFrame1Copy, Size(3, 3), 0);
		GaussianBlur(imgFrame2Copy, imgFrame2Copy, Size(3, 3), 0);
		absdiff(imgFrame1Copy, imgFrame2Copy, imgDifference);
		threshold(imgDifference, imgDifference, thresholdValue, 255.0, CV_THRESH_BINARY);
		structuringElement5x5 = getStructuringElement(MORPH_RECT, Size(5, 5));

		for (unsigned int i = 0; i < 2; i++) {
			dilate(imgDifference, imgDifference, structuringElement5x5);
			dilate(imgDifference, imgDifference, structuringElement5x5);
			erode(imgDifference, imgDifference, structuringElement5x5);
		}

		findContours(imgDifference, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		vector<vector<Point>> convexHulls(contours.size());

		for (unsigned int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], convexHulls[i]);
		}

		for (auto &convexHull : convexHulls) {
			Blob possibleBlob(convexHull);

			// 1. minimum size of Box 
			if (possibleBlob.currentBoundingRect.area() > 10000 &&
				possibleBlob.dblCurrentAspectRatio > 0.8 &&
				possibleBlob.dblCurrentAspectRatio < 4.0 &&
				possibleBlob.currentBoundingRect.width > 120 &&
				possibleBlob.currentBoundingRect.height > 80 &&
				possibleBlob.dblCurrentDiagonalSize > 300 &&
				(contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
				currentFrameBlobs.push_back(possibleBlob);
			}
		}

		if (blnFirstFrame == true) {
			for (auto &currentFrameBlob : currentFrameBlobs) {
				blobs.push_back(currentFrameBlob);
			}
		}
		else {
			matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
		}

		bool blnAtLeastOneBlobCrossedTheLineRightToLeft = checkIfBlobsCrossedTheLineRightToLeft(blobs, countLinePosition, carCountRight);
		bool blnAtLeastOneBlobCrossedTheLineLeftToRight = checkIfBlobsCrossedTheLineLeftToRight(blobs, countLinePosition, carCountLeft);


//		pixX1 = saveRegionOfInterest(blobs, imgFrame2Copy, false);
//		if (pixX1 != -1 && pixX2 != -1) {
//			speed = calculateVelocity(pixX1, pixX2);
//		}

		pixX2 = saveRegionOfInterest(blobs, imgFrame2, blnAtLeastOneBlobCrossedTheLineRightToLeft || blnAtLeastOneBlobCrossedTheLineLeftToRight);
		currentFrameBlobs.clear();

		imgFrame1 = imgFrame2.clone();           // move frame 1 up to where frame 2 is
		capVideo.read(imgFrame2);
		if (imgFrame2.empty()){
			cout << "end of video\n";
			break;
		}

		blnFirstFrame = false;
	}
	return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(vector<Blob> &existingBlobs, vector<Blob> &currentFrameBlobs) {
	for (auto &existingBlob : existingBlobs) {
		existingBlob.blnCurrentMatchFoundOrNewBlob = false;
		existingBlob.predictNextPosition();
	}

	for (auto &currentFrameBlob : currentFrameBlobs) {
		int intIndexOfLeastDistance = 0;
		double dblLeastDistance = 100000.0;

		for (unsigned int i = 0; i < existingBlobs.size(); i++) {
			if (existingBlobs[i].blnStillBeingTracked == true) {
				double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);
				if (dblDistance < dblLeastDistance) {
					dblLeastDistance = dblDistance;
					intIndexOfLeastDistance = i;
				}
			}
		}

		if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) {
			addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
		}
		else {
			addNewBlob(currentFrameBlob, existingBlobs);
		}
	}
	for (auto &existingBlob : existingBlobs) {
		if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
			existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
		}

		if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
			existingBlob.blnStillBeingTracked = false;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob &currentFrameBlob, vector<Blob> &existingBlobs, int &intIndex) {
	existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
	existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;
	existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());
	existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
	existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;
	existingBlobs[intIndex].blnStillBeingTracked = true;
	existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, vector<Blob> &existingBlobs) {
	currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;
	existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(Point point1, Point point2) {
	int intX = abs(point1.x - point2.x);
	int intY = abs(point1.y - point2.y);
	return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineRightToLeft(vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].x > intHorizontalLinePosition && blob.centerPositions[currFrameIndex].x <= intHorizontalLinePosition) {
				carCount++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}
		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineLeftToRight(vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[currFrameIndex].x > intHorizontalLinePosition && blob.centerPositions[prevFrameIndex].x <= intHorizontalLinePosition) {
				carCount++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}
		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int saveRegionOfInterest(vector<Blob> &blobs, Mat &imgFrame2Copy, bool countVehicle) {
	/* Set Region of Interest*/
	if (countVehicle || savedROI) {
		for (unsigned int i = 0; i < blobs.size(); i++) {
			if (blobs[i].blnStillBeingTracked == true) {
				
				Rect roi;
				roi.x = blobs[i].currentBoundingRect.x;
				roi.y = blobs[i].currentBoundingRect.y;

				roi.width = blobs[i].currentBoundingRect.width;
				roi.height = blobs[i].currentBoundingRect.height;

				if (savedROI) {
					savedROI = false;
					// for calculate speed 
					return roi.x;
				}
				else {
					countSaveROIPicture++;
					/* Save Picture */
					Mat crop = imgFrame2Copy(roi);
					sprintf(file, "crop%03i.tif", countSaveROIPicture);
					string s = pathSaveROIPicture + file;
					imwrite(s, crop);
					savedROI = true;
					
					writeFileForBGS(file, roi);
					// for calculate speed 
					
					//getRect(getPictureName(3));
					return roi.x;
				}
				
			}
		}
	}
	else {
		savedROI = false;
		return -1;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double calculateVelocity(int pixX1, int pixX2) {
	double pxPerMm = 0.12;
	int deltaPix = abs(pixX2 - pixX1);
	double deltaX = (deltaPix / pxPerMm) / 1000; //Distance in Meters
	double deltaT = 1 / fps;
	double velocity = deltaX / deltaT * 3.6;
	return velocity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////














