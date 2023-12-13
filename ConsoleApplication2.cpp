#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include "Message.h"
#include "my_file_lib.h"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT_SMTP 2500  // Port của SMTP server
#define DEFAULT_PORT_POP3 1100  // Port của POP3 server
#define DEFAULT_PASSWORD "12345"   // Thiết lập mật khẩu mặc định
#define DEFAULT_USER "Hungkhoavo"  // Thiết lập user mặc định
#define DEFAULT_EMAIL "Vohungkhoa1999@gmail.com" // Email Address mặc định
#define DEFAULT_FILE_PATH "C:\\SocketUS\\"  // Đường dẫn thư mục tổng


// Hàm nhận dữ liệu gửi từ server
string receiveData(SOCKET clientSocket) {
    const int bufferSize = 1024;
    char buffer[bufferSize];
    string receivedData = "";

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
        if (bytesReceived <= 0) {
            break;
        }

        receivedData.append(buffer, bytesReceived);

        if (bytesReceived < bufferSize) {
            break;
        }
    }
    return receivedData;
}


// Hàm lưu nội dung email vào thư mục mailbox
void saveEmailContent(const string& emailContent, const string uid_Email) {
    // Use backslash for Windows
    string mailboxFolder(DEFAULT_FILE_PATH);
    mailboxFolder += "mailbox\\";  

    // Tạo thư mục nếu chưa tồn tại (C:\\SocketUS\\mailbox\\)
    fs::create_directory(mailboxFolder);

    // Tìm vị trí của dấu chấm
    size_t dotPosition = uid_Email.find('.');

    // Lấy phần trước dấu chấm bằng cách sử dụng substr
    string result = uid_Email.substr(0, dotPosition);

    // Lưu nội dung email vào tệp trong thư mục mailbox
    string emailFileName = result + ".txt";
    ofstream file(mailboxFolder + emailFileName);
    if (!file) {
        cerr << "Cannot create the file" << endl;
        return;
    }
    file << emailContent;

    // Đặt con trỏ về đầu file để ghi đè
    file.seekp(0, ios::beg);

    // Ghi đè dòng đầu tiên bằng "False" - có nghĩa là mail chưa được đọc
    file << "False\r\n";

    file.close();
}

// Hàm xử lý email
void processEmail(SOCKET clientSocket, const string uid_Email) {
    // Nhận dữ liệu từ client
    string emailData = receiveData(clientSocket);

    // Lưu nội dung email
    saveEmailContent(emailData, uid_Email);
}

int sendEmailFunction() {
    // Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock." << endl;
        return 1;
    }

    // Tạo socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    // Thiết lập địa chỉ và cổng của SMTP server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DEFAULT_PORT_SMTP);  // Chuyển cổng về dạng số nguyên

    // Chuyển địa chỉ IP từ chuỗi sang dạng số nguyên
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) != 1) {
        cerr << "Failed to convert IP address." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Kết nối đến SMTP server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to connect to SMTP server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Đọc phản hồi từ server
    char buffer[DEFAULT_BUFLEN] = { 0 };
    int sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh EHLO để bắt đầu phiên giao tiếp với SMTP server
    const char* ehloCommand = "EHLO example.com\r\n";
    send(clientSocket, ehloCommand, strlen(ehloCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);


    // Thiết lập một số thông tin tài khoản mặc định
    string userName = DEFAULT_USER;
    string passWord = DEFAULT_PASSWORD;
    string emailAddress = DEFAULT_EMAIL;

    // Gửi lệnh MAIL FROM để chỉ định địa chỉ người gửi
    string mailFromCommand = "MAIL FROM: <" + emailAddress + ">\r\n";
    send(clientSocket, mailFromCommand.c_str(), mailFromCommand.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Quá trình đọc và gửi các địa chỉ người nhận cho server
    vector<string> listTO;    // Danh sách địa chỉ người nhận trực tiếp
    string rcptToCommand = "";  // Biến tạm lưu địa chỉ người nhận

    // Nếu gửi đến nhiều người thì nhập thêm địa chỉ, STOP để dừng lại
    cout << "Input STOP when you finished" << endl;
    while (true) {

        // Nhập địa chỉ người nhận
        cout << "Input EMAIL TO: ";
        getline(cin, rcptToCommand);

        // Thoát vòng lặp khi người dùng đã hoàn thành xong tất cả các địa chỉ nhận TO
        if (rcptToCommand == "STOP" || rcptToCommand == "stop" || rcptToCommand == "Stop") {
            rcptToCommand.clear();
            break;
        }

        // Thêm địa chỉ vào danh sách người nhận
        listTO.push_back(rcptToCommand);

        // Gửi lệnh RCPT TO để chỉ định địa chỉ người nhận TO
        rcptToCommand = "RCPT TO: <" + rcptToCommand + ">\r\n";
        send(clientSocket, rcptToCommand.c_str(), rcptToCommand.length(), 0);

        // Đọc phản hồi từ server
        sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << endl;
    }

    // Nhập các địa chỉ của người nhận CC
    vector <string> listCC;  // Tạo list các địa chỉ người nhận CC
    cout << "(Input STOP when you finished)" << endl;
    while (true) {

        // Nhập địa chỉ người nhận
        cout << "Input EMAIL CC: ";
        getline(cin, rcptToCommand);

        // Thoát vòng lặp khi người dùng đã hoàn thành xong tất cả các địa chỉ CC
        if (rcptToCommand == "STOP" || rcptToCommand == "stop" || rcptToCommand == "Stop") {
            rcptToCommand.clear();
            break;
        }

        // Thêm địa chỉ vào danh sách người nhận CC
        listCC.push_back(rcptToCommand);

        // Gửi lệnh RCPT TO để chỉ định địa chỉ người nhận CC
        rcptToCommand = "RCPT TO: <" + rcptToCommand + ">\r\n";
        send(clientSocket, rcptToCommand.c_str(), rcptToCommand.length(), 0);

        // Đọc phản hồi từ server
        sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << endl;
    }

    // Nhập địa chỉ người nhận BCC
    cout << "(Input STOP when you finished)" << endl;
    while (true) {

        // Nhập địa chỉ người nhận
        cout << "Input EMAIL BCC: ";
        getline(cin, rcptToCommand);

        // Thoát vòng lặp khi người dùng đã hoàn thành xong tất cả các địa chỉ BCC
        if (rcptToCommand == "STOP" || rcptToCommand == "stop" || rcptToCommand == "Stop") {
            rcptToCommand.clear();
            break;
        }

        // Gửi lệnh RCPT TO để chỉ định địa chỉ người nhận BCC
        rcptToCommand = "RCPT TO: <" + rcptToCommand + ">\r\n";
        send(clientSocket, rcptToCommand.c_str(), rcptToCommand.length(), 0);

        // Đọc phản hồi từ server
        sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << endl;
    }

    // Gửi dữ liệu email
    const char* dataCommand = "DATA\r\n";
    send(clientSocket, dataCommand, strlen(dataCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    
    // Nhâp chủ đề
    cout << "Input your Subject: ";
    string emailSubject = "";
    getline(cin, emailSubject);
    emailSubject = "Subject: " + emailSubject + "\r\n";
    send(clientSocket, emailSubject.c_str(), emailSubject.length(), 0);

    // Gửi thông tin người nhận TO
    string emailRpt = "To: ";
    for (const auto& x : listTO) {
        emailRpt += "<" + x + "> ";
    }
    emailRpt += "\r\n";
    send(clientSocket, emailRpt.c_str(), emailRpt.length(), 0);


    // Gửi thông tin người nhận CC
    if (listCC.size() != 0) {
        emailRpt = "CC: ";
        for (const auto& x : listCC) {
            emailRpt += "<" + x + "> ";
        }
        emailRpt += "\r\n";
        send(clientSocket, emailRpt.c_str(), emailRpt.length(), 0);
    }

    // Gửi thông tin người gửi
    emailRpt = "From: " + userName + " <" + emailAddress + ">" + "\r\n";
    send(clientSocket, emailRpt.c_str(), emailRpt.length(), 0);
    cout << endl;

    // Nhập nội dung
    cout << "Input your Content \n(Enter + 'Stop') to end the content\n";
    string emailContent = "";
    string emailMess = "";

    // Nhập nội dung
    while (emailContent != "stop" && emailContent != "Stop" && emailContent != "STOP") {
        emailMess += (emailContent + "\r\n");
        getline(cin, emailContent);
    }

    // Thêm ký tự xuống hàng để phân biệt các phần trong email
    emailMess += "\r\n---EndOfContent---\r\n";

    // Gửi nội dung
    send(clientSocket, emailMess.c_str(), emailMess.length(), 0);

    // Gửi file đính kèm
    sendAttachment(clientSocket);

    // Thêm dấu chấm và dòng kết thúc để kết thúc và gửi tin nhắn đến server
    emailMess = "---EndOfMail---\r\n.\r\n";
    send(clientSocket, emailMess.c_str(), emailMess.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Đóng kết nối
    closesocket(clientSocket);
    WSACleanup();
}


int rcvEmailFunction() {
    // Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock." << endl;
        return 0;
    }

    // Tạo socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 0;
    }

    // Thiết lập địa chỉ và cổng của POP3 server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1100);  // Chuyển cổng về dạng số nguyên

    // Chuyển địa chỉ IP từ chuỗi sang dạng số nguyên
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) != 1) {
        cerr << "Failed to convert IP address." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Kết nối đến POP3 server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to connect to POP3 server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Đọc phản hồi từ server
    char buffer[DEFAULT_BUFLEN] = { 0 };
    int sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);


    // Quá trình khởi tạo Socket và kết nối với Server hoàn thành
    // Bắt đầu quá trình gửi thông tin lên server
    // Thiết lập thông tin tài khoản
    string userCommand = DEFAULT_EMAIL;
    string passCommand = DEFAULT_PASSWORD;

    // Gửi lệnh CAPA
    const char* capaCommand = "CAPA\r\n";
    send(clientSocket, capaCommand, strlen(capaCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh USER để xác thực người dùng
    userCommand = "USER " + userCommand + "\r\n";
    send(clientSocket, userCommand.c_str(), userCommand.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh PASS để xác thực mật khẩu
    passCommand = "PASS " + passCommand + "\r\n";
    send(clientSocket, passCommand.c_str(), passCommand.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh STAT để nhận thông tin trong hộp thư
    const char* statCommand = "STAT\r\n";
    send(clientSocket, statCommand, strlen(statCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh LIST để liệt kê các email trong hộp thư
    const char* listCommand = "LIST\r\n";
    send(clientSocket, listCommand, strlen(listCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Gửi lệnh UIDL để nhận mã tin nhắn (dùng để phân biệt các tin nhắn)
    const char* uidlCommand = "UIDL\r\n";
    send(clientSocket, uidlCommand, strlen(uidlCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Đọc thông tin các UID list từ buffer
    string temp = "";
    stringstream s(buffer);

    // Luot bo chu OK
    getline(s, temp); 

    // Tạo danh sách UID có sẵn trên PC và danh sách UID nhận về
    vector<string> arrayLocal;
    vector<string> array;

    while (s >> temp) {
        if (temp == ".")
            break;
        s >> temp;
        array.push_back(temp); 
    }

    // Tao duong dang den file UIDL ("C:\\SocketUS\\UIDL.txt")
    string filePathSocket(DEFAULT_FILE_PATH);
    string filePathUIDL = filePathSocket + "UIDL.txt";

    // Tạo cờ hiệu đánh giấu thư mới
    bool isRCPT = false;

    // Mở tệp lưu các UID tin nhắn đã có sẵn
    ifstream fin(filePathUIDL);
    if (!fin) {
        // Tạo thư mục mới
        fs::create_directory(filePathSocket);

        // Tạo tập tin để lưu danh sách các UID mail
        ofstream fout(filePathUIDL);
        if (!fout) {
            cerr << "Error: Cannot create a file to save UIDL" << endl;
            return 0;
        }

        // Ghi các UID thành các dòng
        for (int i = 0; i < array.size(); i++) {
            fout << array[i] << endl;
        }

        // Đóng file
        fout.close();

        // Gửi lệnh RETR để gửi tín hiệu nhận thư
        for (int i = 1; i <= array.size(); i++) {
            string retrCommand = "RETR " + to_string(i) + "\r\n";
            send(clientSocket, retrCommand.c_str(), retrCommand.length(), 0);

            // Xử lý nhận nội dung email
            processEmail(clientSocket, array[i - 1]);
        }

        // Đánh dấu có thư mới
        isRCPT = true;
    }
    else {
        // Đọc các nội dung trong file và lưu vào danh sách.
        while (fin >> temp) {
            arrayLocal.push_back(temp);
        }

        // So sánh xem UID đã tồn tại trên PC chưa
        for (int i = 0; i < array.size() ; i++) {
            // Set cờ để kiểm tra xem UID có tồn tại chưa
            bool flag = false;
            for (int j = 0; j < arrayLocal.size(); j++) {
                if (array[i] == arrayLocal[j]) {
                    flag = true;
                    break;
                }
            }

            // Nếu chưa tồn tại thì gửi lệnh tải về
            if (!flag) {
                // Gửi lệnh để nhận mail
                string retrCommand = "RETR " + to_string(i + 1) + "\r\n";
                send(clientSocket, retrCommand.c_str(), retrCommand.length(), 0);

                // Xử lý nhận nội dung email
                processEmail(clientSocket, array[i]);

                // Ghi UID mới vào tệp UIDL_email
                ofstream fout(filePathUIDL, ios::app);
                fout << array[i] << endl;

                fout.close();
                
                // Đánh dấu có thư mới
                isRCPT = true;
            }
        }
    }
    fin.close();

    // Gửi lệnh QUIT
    const char* quitCommand = "QUIT\r\n";
    send(clientSocket, quitCommand, strlen(quitCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Đóng kết nối
    closesocket(clientSocket);
    WSACleanup();
    
    // Nếu có thư mới thì trả về 1, ngược lại trả về 0
    if (isRCPT) {
        return 1;
    }
    return 0;
}


// DUY
string classifyEmail(const string& emailContent) {
    // Convert the email content to lowercase for case-insensitive matching
    string lowercaseContent = emailContent;
    transform(lowercaseContent.begin(), lowercaseContent.end(), lowercaseContent.begin(), ::tolower);

    // Check for keywords indicating important email
    if (lowercaseContent.find("important") != string::npos ||
        lowercaseContent.find("urgent") != string::npos) {
        return "important";
    }

    // Check for keywords indicating spam
    if (lowercaseContent.find("win") != string::npos ||
        lowercaseContent.find("lottery") != string::npos ||
        lowercaseContent.find("free") != string::npos) {
        return "spam";
    }

    // If no rules match, consider it a regular email
    return "regular";
}

void classifyAndMove(const string& filePath)
{
    string a = "important";
    string b = "regular";
    string c = "spam";

    // Tạo các đường dẫn thư mục để phân loại
    const string filePathMailBox = filePath + "mailbox\\";
    const string filePathImportant = filePath + a + "\\";
    const string filePathRegular = filePath + b + "\\";
    const string filePathSpam = filePath + c + "\\";

    // Tạo các thư mục để phân loại (nếu chưa tồn tại)
    // Thư mục thư quan trọng
    if (!fs::exists(filePathImportant)) {
        fs::create_directory(filePathImportant);
    }

    // Thư mục thư thông thường
    if (!fs::exists(filePathRegular)) {
        fs::create_directory(filePathRegular);
    }

    // Thư mục thư spam
    if (!fs::exists(filePathSpam)) {
        fs::create_directory(filePathSpam);
    }

    // Danh sách các tin nhắn cần phân loại
    vector<string> listEmail = getFilesInDirectory(filePathMailBox);

    for (const auto& x : listEmail)
    {
        // Đọc nội dung email:
        string data = readFile(filePathMailBox + x);

        // Phân loại email dựa trên các từ khóa:
        string classify = classifyEmail(data);

        // Copy email đến các folder phù hợp;
        if (classify == a)
            copyFile(filePathMailBox + x, filePathImportant + x);
        else if (classify == b)
            copyFile(filePathMailBox + x, filePathRegular + x);
        else if (classify == c)
            copyFile(filePathMailBox + x, filePathSpam + x);
    }
}

vector <Message> readMessage(const string& filePath) {
    // Lấy danh sách mail trong thư mục
    vector <string> listEmail = getFilesInDirectory(filePath);

    // Tạo danh sách lưu kết quả trả về
    vector <Message> result;

    for (const auto& x : listEmail) {
        Message message;

        // Tạo file path đến các tin nhắn và đọc nội dung
        const string path = filePath + x;
        const string contentEmail = readFile(path);

        // Xử lý nội dung và lưu dữ liệu
        message.readContentMail(contentEmail);

        // Thêm vào danh sách các tin nhắn đã được xử lý
        result.push_back(message);
    }
    return result;
}

void backgroundMailReceiver() {
    while (true) {
        if (rcvEmailFunction()) {
            classifyAndMove(DEFAULT_FILE_PATH);
            cout << "Co thu moi" << endl;
        }

        // Đợi 10 giây trước khi thử lại
        this_thread::sleep_for(chrono::seconds(10));
    }
}

void Menu() {
    cout << "1. Gui mail\n"
         << "2. Hop thu\n"
         << "0. Thoat\n";
}

void ex() {
    thread mailReceiverThread(backgroundMailReceiver);
    while (true) {
        int slt;
        Menu();

        cin >> slt;
        cin.ignore();

        if (slt == 1) {
            sendEmailFunction();
        }
        else if (slt == 2) {
            vector<Message> temp = readMessage("C:\\SocketUS\\mailbox\\");
            for (auto x : temp) {
                x.printMessage();

                /*while (true) {
                    cout << "Co muon luu tep dinh kem hay khong (1. Co, 0. Khong): ";
                    int sec;
                    cin >> sec;
                    cin.ignore();

                    if (sec) {
                        cout << "Nhap duong dan de luu: ";
                        string filePATH;
                        getline(cin, filePATH);

                        x.saveAttachment(filePATH);
                        break;
                    }
                    else if(!sec) break;
                }*/
            }
        }
        else return;
        system("pause");
        system("cls");
    }
}


int main() {   
    //Phân loại email
    ex();
    return 0;
}
