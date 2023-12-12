#pragma once
#include <fstream>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include "base64.h"
using namespace std;
namespace fs = std::filesystem;

// Khai báo*
// Tính toán kích thước file
uintmax_t getFileSize(const string& filePath);

// Đọc file thông qua đường truyền tới file
string readFile(const string& filePath);

// Hàm mã hóa, dữ liệu hóa một số thông tin để chuẩn bị gửi file
void subSendAttachment(SOCKET clientSocket, string filePath);

// Gửi file đính kèm
void sendAttachment(SOCKET clientSocket);

// Lưu file vào máy tính
void saveEmailToFile(const string& emailContent, const string& filePath);

// Hàm trả về danh sách các file có trong 1 folder
vector<string> getFilesInDirectory(const string& path);

// Hàm để sao chép một tệp tin từ thư mục nguồn sang thư mục đích
void copyFile(const string& sourcePath, const string& destinationPath);

// Cài đặt*
// Tính toán kích thước file
uintmax_t getFileSize(const string& filePath)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) 
    {
        ULARGE_INTEGER fileSize;
        fileSize.LowPart = fileInfo.nFileSizeLow;
        fileSize.HighPart = fileInfo.nFileSizeHigh;
        return fileSize.QuadPart;
    }
    else 
    {
        cerr << "Failed to get file size." << endl;
        return 0;
    }
}

// Hàm đọc nội dung từ file
string readFile(const string& filePath) 
{
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) 
    {
        cerr << "Error opening file: " << filePath << endl;
        return "";
    }

    // Đọc nội dung file vào chuỗi
    string fileContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    return fileContent;
}

// Ham gui file dinh kem, bang cach truyen vao duong dan file
void subSendAttachment(SOCKET clientSocket, string filePath) 
{
    string fileContent = readFile(filePath);

    // Mã hóa nội dung tệp tin sang base64
    string encodedFileContent = base64_encode(fileContent);
    encodedFileContent += "\r\n";

    // Lấy đuôi mở rộng của tệp tin
    size_t dotPosition = filePath.find_last_of(".");
    string fileExtension = filePath.substr(dotPosition + 1);

    // Lấy tên tệp
    string fileName;
    stringstream s(filePath);
    while (getline(s, fileName, '\\'));

    // Thêm đính kèm vào phần đầu của email
    string attachmentHeader = "File-name: " + fileName + "\r\n";

    // Gửi thông tin file đính kèm
    send(clientSocket, attachmentHeader.c_str(), attachmentHeader.length(), 0);

    // Số ký tự tối đa trên mỗi dòng (76 là giới hạn thường được sử dụng)
    const int chunkSize = 76;

    // Chia dữ liệu mã hóa thành các phần (dòng)
    for (size_t i = 0; i < encodedFileContent.length(); i += chunkSize) 
    {
        size_t chunkEnd = min(i + chunkSize, encodedFileContent.length());
        string chunk = encodedFileContent.substr(i, chunkEnd - i);

        // Thêm xuống hàng sau mỗi dòng (phần)
        chunk += "\r\n";

        // Gửi từng phần mã hóa đến server
        send(clientSocket, chunk.c_str(), chunk.length(), 0);
    }
}

// Gửi file đính kèm
void sendAttachment(SOCKET clientSocket) {
    int select;
    bool isSendFile = false;  // Gắn cờ nếu có gửi file
    while (true) 
    {
        // Nhận lựa chọn
        cout << "Do you want to send attachments? (1. Yes, 0. No): ";
        cin >> select;
        cin.ignore();

        // Xử lý gửi file
        if (select == 0) 
        {
            return;
        }
        else if (select == 1) 
        {
            string filePath;
            while (true)
            {
                // Nhận đường dẫn file
                cout << "\nCaution: the limit size of file is 3MB\n";
                cout << "Input file-path(input [stop] whenever you want to end): ";
                getline(cin, filePath);

                // Thực hiện gọi hàm gửi file ra để mã hóa và gửi file(nếu đường dẫn hợp lệ)
                if (filePath != "stop" && filePath != "STOP" && filePath != "Stop") 
                {
                    // Tính toán kích thước file
                    uintmax_t fileSize = getFileSize(filePath);
                    cout << "File size: " << fileSize / 1024 / 1024.0 << "MB" << endl;

                    // Kiểm tra xem kích thước có phù hợp hay không
                    const uintmax_t maxFileSize = 3 * 1024 * 1024;  // 3MB
                    if (fileSize > maxFileSize) 
                    {
                        cerr << "File size exceeds the limit of 3MB.\nCannot send!" << endl;
                        continue;
                    }

                    // Đường dẫn không mở được hoặc sai
                    if (fileSize == 0) 
                    {
                        cerr << "Wrong format file-path!\n";
                        continue;
                    }

                    // Nếu đã gửi attachment từ trước rồi nhưng vẫn còn gửi thêm, thì sẽ gửi thêm dòng ngăn cách các tệp đính kèm
                    if (isSendFile) 
                    {
                        string endAttachment = "---AttachmentBelow---\r\n";
                        send(clientSocket, endAttachment.c_str(), endAttachment.length(), 0);
                    }

                    // Nếu chưa từng gửi file từ trước thì thêm đoạn bắt đầu gửi file
                    else 
                    {
                        string startAttachment = "---StartAttachment---\r\n";
                        send(clientSocket, startAttachment.c_str(), startAttachment.length(), 0);
                    }

                    // Gửi nội dung file đã mã hóa
                    subSendAttachment(clientSocket, filePath);
                    isSendFile = true;
                }
                else
                {
                    // Nếu có gửi file thì gửi ký hiệu kết thúc quá trình gửi, còn k gửi file thì kết thúc tại đây
                    if (isSendFile) 
                    {
                        string endAttachmen = "---EndAttachment---\r\n";
                        send(clientSocket, endAttachmen.c_str(), endAttachmen.length(), 0);
                    }
                    return;   // Thoát khỏi quá trình gửi file
                }
            }
        }
        // Nếu người dùng nhập sai (khác 0, 1) thì người dùng sẽ được nhập lại
        else cout << "Wrong answer, pls enter value 1 or 0" << endl;
    }
}

// Lưu file vào bộ nhớ
void saveEmailToFile(const string& emailContent, const string& filePath)
{
    ofstream outputFile(filePath);
    if (outputFile.is_open())
    {
        outputFile << emailContent;
        outputFile.close();
    }
    else 
    {
        cerr << "Failed to open file: " << filePath << endl;
    }
}

// Trả về danh sách các tên file trong 1 folder
vector<string> getFilesInDirectory(const string& path) 
{
    vector<string> fileList;

    try 
    {
        for (const auto& entry : fs::directory_iterator(path)) 
        {
            if (fs::is_regular_file(entry)) 
            {
                fileList.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return fileList;
}

// Hàm để sao chép một tệp tin từ thư mục nguồn sang thư mục đích
void copyFile(const string& sourcePath, const string& destinationPath)
{
    ifstream sourceFile(sourcePath, ios::binary);
    ofstream destinationFile(destinationPath, ios::binary);

    if (!sourceFile || !destinationFile)
    {
        cerr << "Failed to open source or destination file." << endl;
        return;
    }

    // Đọc và ghi từng byte
    char byte;
    while (sourceFile.get(byte))
    {
        destinationFile.put(byte);
    }
    sourceFile.close();
    destinationFile.close();
}
