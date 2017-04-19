#include "FileHandler.h"

char fileNameForBGS[256] = "/root/CarCount/textfile.txt";
vector<string> listLine;

///////////////////////////////////////////////////////////////////////////////////////////////////
void writeFileForBGS(string pictureName, Rect roi) {
	fstream file;
	file.open(fileNameForBGS, ios::app);
	char s[128];
	file << pictureName;
	sprintf(s, ";\t%i", roi.x);
	file << s;
	sprintf(s, ";\t%i", roi.y);
	file << s;
	sprintf(s, ";\t%i", roi.width);
	file << s;
	sprintf(s, ";\t%i", roi.height);
	file << s;
	file << ";\n";
	file.close();
}
void readFile() {
	string line;
	fstream fileInput;
	fileInput.open(fileNameForBGS, fstream::in);
	while (getline(fileInput, line)) {
		listLine.push_back(line);
	}
	fileInput.close();
	//fileInput.open(fileNameForBGS, fstream::out);
	//fileInput<< "";
	
}
///////////////////////////////////////////////////////////////////////////////////////////////////
string getPictureName(int i) {
	readFile();
	string s = listLine[i];
	string delimiter = ";";
	string token = s.substr(0, s.find(delimiter));
	return token;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
Rect getRect(string pictureName) {
	Rect roi;
	string token ="x";
	string delimiter = pictureName;
	string s;
	uint i = 0;
	while(token != "" && i<listLine.size()) {
		s = listLine[i];
		token = s.substr(0, s.find(delimiter));
		i++;
	}
	string test = s;
	//int ix = s.find(";");
	token = s.substr(0, s.find(delimiter));
	roi.x = 1;
	roi.y = 1;

	roi.width = 1;
	roi.height = 1;

	return roi;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
time_t timeStamp(string pictureName) {
	return time(0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//string getActuallDate() {
//	time_t rawtime;
//	char s[64];
//	time(&rawtime);
//	sprintf(s, ";\t%i", rawtime);
//	string str(s);
//	return str;
//}

/* TimeStamp
struct tm * timeinfo;
char buffer[80];

file << ";\t" + getActuallDate() + "\n";

*/
