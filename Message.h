#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Attachment.h"
#include <sstream>
using namespace std;
class Message
{
private:
	string _id;
	string _subject;
	string _fromEmail;
	string _fromUser;
	bool _haveAttachment;
	bool _isRead;
	vector<string> _ccList;
	vector<string> _toList;
	vector<string> _content;
	vector<Attachment> _attachmentList;
public:
	Message() 
	{
		_haveAttachment = false;
		_isRead = false;
	}

	string getSubject() 
	{
		return _subject;
	}

	string getId() 
	{
		return _id;
	}

	string getFromEmail() 
	{
		return _fromEmail;
	}

	string getFromUser() 
	{
		return _fromUser;
	}

	vector<string> getContent() 
	{
		return _content;
	}

	bool getAttachmentStatus() 
	{
		return _haveAttachment;
	}

	void setSubject(string subject) 
	{
		_subject = subject;
	}

	void setContent(string content) 
	{
		stringstream s(content);
		string line;

		// Lưu từng dòng vào thuộc tính nội dung
		while (getline(s, line)) 
		{
			_content.push_back(line);
		}
	}

	void saveAttachment(const string& filePath) 
	{
		for (auto x : _attachmentList) 
		{
			x.decodeAndSaveToFilePath(filePath);
		}
	}

	void printMessage() 
	{

		// Nội dung bao gồm cả phần đầu đề và body
		cout << "Subject: " << _subject << endl;

		// In danh sách người nhận To
		cout << "To: ";
		for (auto x : _toList) 
		{
			cout << x << " ";
		}
		cout << endl;

		// In danh sách người nhận CC
		if (!_ccList.empty()) 
		{
			cout << "CC: ";
			for (auto x : _ccList)
			{
				cout << x << " ";
			}
			cout << endl;
		}

		// In thông tin người gửi
		cout << "From: " << _fromUser << " " << _fromEmail << endl;

		// In nội dung tin nhắn
		for (auto x : _content) 
		{
			cout << x << endl;
		}

		// File đính kèm
		if (_haveAttachment) 
		{
			cout << "File dinh kem: " << _attachmentList.size() << endl;
			for (auto x : _attachmentList) 
			{
				cout << x.getNameFile() << endl;
			}
			cout << endl;
		}	
	}

	void readContentMail(string contentMail) 
	{
		string temp, line;
		stringstream s(contentMail);

		// Thiết lập trạng thái cho chủ đề và người gửi
		bool isHaveSJ = false;
		bool isHaveFrom = false;
		bool isHaveTOList = false;
		bool isHaveCCList = false;

		// Đọc dòng trạng thái thư (đã đọc hay chưa)
		getline(s, line);

		// Thiết lập trạng thái thư (đã đọc chưa)
		size_t position = line.find("True");
		if (position != string::npos) 
		{
			_isRead = true;
		}

		// Đọc nội dung email
		while (getline(s, line))
		{
			if (line.find("---EndOfContent---") != string::npos)
				break;

			// Tìm chủ đề
			position = line.find("Subject");
			if (position != string::npos && !isHaveSJ) 
			{
				// Nhận chủ đề
				_subject = line.substr(position + strlen("Subject: "));

				// Set trạng thái đã nhận được chủ đề
				isHaveSJ = true;
				continue;
			}

			// Tìm người gửi
			position = line.find("From");
			if (position != string::npos && !isHaveFrom) 
			{
				// Xử lý nhận thông tin người gửi, loại bỏ ký tự \r\n
				size_t position_end_line = line.find("\r\n");
				string infor_from_user = line.substr(position + strlen("From: "), position_end_line);

				// Lưu thông tin người gửi vào các thuộc tính của Message
				stringstream user(infor_from_user);
				user >> _fromUser >> _fromEmail;

				// Set trạng thái đã nhận được thông tin 
				isHaveFrom = true;
				continue;
			}

			// Tìm người nhận TO
			position = line.find("To");
			if (position != string::npos && !isHaveTOList) 
			{
				// Xử lý dữ liệu TO, loại bỏ ký tự \r\n
				size_t position_end_line = line.find("\r\n");
				string lineTO = line.substr(position + strlen("To: "), position_end_line);

				// Sử dụng stringstream để chia chuỗi
				stringstream to_user(lineTO);
				string user;
	
				// Thêm vào danh sách
				while (to_user >> user) 
				{
					_toList.push_back(user);
				}
				continue;
			}

			// Tìm người nhận CC
			position = line.find("Cc");
			if (position != string::npos && !isHaveTOList) 
			{
				// Xử lý dữ liệu, loại bỏ ký tự \r\n
				size_t position_end_line = line.find("\r\n");
				string lineCC = line.substr(position + strlen("Cc: "), position_end_line);

				// Sử dụng stringstream để chia chuỗi
				stringstream cc_user(lineCC);
				string user;

				// Thêm vào danh sách
				while (cc_user >> user) 
				{
					_ccList.push_back(user);
				}
				continue;
			}

			// Ghi nội dung email nếu đã đọc được phần đầu của email
			if (isHaveFrom && isHaveSJ) 
			{
				_content.push_back(line);
			}
		}

		// Đọc nội dung tệp đính kèm nếu có
		while (true) 
		{
			getline(s, line);
			size_t pos = line.find("---StartAttachment---");
			size_t pos_2 = line.find("---AttachmentBelow---");

			if ((pos != string::npos) || (pos_2 != string::npos)) 
			{
				// Đọc tên file
				getline(s, line, '\r');
				string contentFileEncode;
				string fileName = line.substr(line.find("File-name") + strlen("File-name: "));

				// Đọc bỏ ký tự xuống hàng
				getline(s, temp);
	
				// Đọc tệp mã hóa
				while (getline(s, line, '\r')) 
				{
					// Dung khi cuoi file
					if (line.empty())
						break;

					// Noi dung
					contentFileEncode += line;

					// Đọc bỏ ký tự xuống hàng
					getline(s, temp);
				}
				// Luu ten file va noi dung file bi ma hoa vao thuoc tinh attachment
				Attachment attachment_file(fileName, contentFileEncode);
				_attachmentList.push_back(attachment_file);
				_haveAttachment = true;

				// Đọc bỏ ký tự xuống hàng
				getline(s, temp);
			}
			else break;
		}
	}
};

