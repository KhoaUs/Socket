#pragma once
#include <iostream>
#include <fstream>
#include "base64.h"
using namespace std;
class Attachment
{
private:
	string _name;
	string _contentFile;
public:
	Attachment() {}
	Attachment(string name, string content): _name(name), _contentFile(content){}

	string getNameFile() {
		return _name;
	}

	string getContentFile() {
		return _contentFile;
	}

	void decodeAndSaveToFilePath(const string& filePath) {
		// Vị trí lưu file đính kèm
		string saveFilePath = filePath + "\\" + _name;

		// Giải mã nội dung từ base64
		string decodedContent = base64_decode(_contentFile);

		// Lưu nội dung đã giải mã vào file
		ofstream outputFile(saveFilePath, ios::binary);
		if (!outputFile) {
			cerr << "Loi he thong: tep chua duoc luu vao vi tri: " << filePath << endl;
			return;
		}

		outputFile << decodedContent;
		outputFile.close();

		cout << "Tep dinh kem da duoc luu tai vi tri: " << saveFilePath << endl;
	}
};

