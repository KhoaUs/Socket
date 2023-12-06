#include <winsock2.h>
#include <ws2tcpip.h>
#include "base64.h"
#include "my_file_lib.h"
#include <cstring>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT_SMTP 2500  // Port của SMTP server
#define DEFAULT_PORT_POP3 = 1100;  // Port của POP3 server
#define DEFAULT_PASSWORD "12345"   // Thiết lập mật khẩu mặc định
#define DEFAULT_USER "Hungkhoavo"  // Thiết lập user mặc định
#define DEFAULT_EMAIL "Vohungkhoa1999@gmail.com" // Email Address mặc định


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
    const string mailboxFolder = "C:\\SocketUS\\mailbox\\";  // Use backslash for Windows

    // Tạo thư mục nếu chưa tồn tại (C:\\SocketUS\\mailbox\\)
    system(("mkdir " + mailboxFolder).c_str());

    // Tìm vị trí của dấu chấm
    size_t dotPosition = uid_Email.find('.');

    // Lấy phần trước dấu chấm bằng cách sử dụng substr
    string result = uid_Email.substr(0, dotPosition);

    // Lưu nội dung email vào tệp trong thư mục mailbox
    string emailFileName = result + ".txt";
    ofstream file(mailboxFolder + emailFileName);
    file << emailContent;
    file.close();
}

// Hàm xử lý email
void processEmail(SOCKET clientSocket, const string uid_Email) {
    // Nhận dữ liệu từ client
    string emailData = receiveData(clientSocket);
    cout << emailData << endl;

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
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh EHLO để bắt đầu phiên giao tiếp với SMTP server
    const char* ehloCommand = "EHLO example.com\r\n";
    send(clientSocket, ehloCommand, strlen(ehloCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Thiết lập một số thông tin tài khoản mặc định
    string userName = DEFAULT_USER;
    string passWord = DEFAULT_PASSWORD;
    string emailAddress = DEFAULT_EMAIL;

    // Gửi lệnh MAIL FROM để chỉ định địa chỉ người gửi
    string mailFromCommand = "MAIL FROM: <" + emailAddress + ">\r\n";
    send(clientSocket, mailFromCommand.c_str(), mailFromCommand.length(), 0);
    //const char* mailFromCommand = "MAIL FROM: <sender@example.com>\r\n";

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

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
        buffer[sizeChar] = '\0';
        cout << "Server response: " << buffer << endl;
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
        buffer[sizeChar] = '\0';
        cout << "Server response: " << buffer << endl;
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
        buffer[sizeChar] = '\0';
        cout << "Server response: " << buffer << endl;
    }

    // Gửi dữ liệu email
    const char* dataCommand = "DATA\r\n";
    send(clientSocket, dataCommand, strlen(dataCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Nhâp chủ đề
    cout << "Input your Subject: ";
    string emailSubject = "";
    getline(cin, emailSubject);
    emailSubject = "Subject: " + emailSubject + "\r\n";
    send(clientSocket, emailSubject.c_str(), emailSubject.length(), 0);

    // Gửi thông tin người nhận TO
    string emailRpt = "To: ";
    for (auto x : listTO) {
        emailRpt += "<" + x + ">";
    }
    emailRpt += "\r\n";
    send(clientSocket, emailRpt.c_str(), emailRpt.length(), 0);


    // Gửi thông tin người nhận CC
    if (listCC.size() != 0) {
        emailRpt = "CC: ";
        for (auto x : listCC) {
            emailRpt += "<" + x + ">";
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
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Đóng kết nối
    closesocket(clientSocket);
    WSACleanup();
}


int rcvEmailFunction() {
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

    // Thiết lập địa chỉ và cổng của POP3 server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1100);  // Chuyển cổng về dạng số nguyên

    // Chuyển địa chỉ IP từ chuỗi sang dạng số nguyên
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) != 1) {
        cerr << "Failed to convert IP address." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Kết nối đến POP3 server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to connect to POP3 server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Đọc phản hồi từ server
    char buffer[DEFAULT_BUFLEN] = { 0 };
    int sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Server response: " << buffer << endl;

    // Thiết lập thông tin tài khoản
    string userCommand = DEFAULT_EMAIL;
    string passCommand = DEFAULT_PASSWORD;

    // Gửi lệnh CAPA
    const char* capaCommand = "CAPA\r\n";
    send(clientSocket, capaCommand, strlen(capaCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh USER để xác thực người dùng
    userCommand = "USER " + userCommand + "\r\n";
    send(clientSocket, userCommand.c_str(), userCommand.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh PASS để xác thực mật khẩu
    passCommand = "PASS " + passCommand + "\r\n";
    send(clientSocket, passCommand.c_str(), passCommand.length(), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh STAT để nhận thông tin trong hộp thư
    const char* statCommand = "STAT\r\n";
    send(clientSocket, statCommand, strlen(statCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh LIST để liệt kê các email trong hộp thư
    const char* listCommand = "LIST\r\n";
    send(clientSocket, listCommand, strlen(listCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Gửi lệnh UIDL để nhận mã tin nhắn (dùng để phân biệt các tin nhắn)
    const char* uidlCommand = "UIDL\r\n";
    send(clientSocket, uidlCommand, strlen(uidlCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Đọc thông tin các UID list từ buffer
    string temp = "";
    stringstream s(buffer);

    // Luot bo chu OK
    getline(s, temp); 

    // Lưu các UID vào trong một danh sách
    string array[100];
    int countEmail = 0;

    while (s >> temp) {
        if (temp == ".")
            break;
        s >> array[++countEmail]; 
        cout << array[countEmail] << endl;
    }

    // Tạo danh sách lưu các UID đã có sẵn
    string arrayLocal[100];
    int countLocalEmail = 0;

    // Mở tệp lưu các UID tin nhắn đã có sẵn
    ifstream fin("C:\\SocketUS\\UIDL.txt");
    if (!fin) {
        cout << "UIDL.txt not exit" << endl;
        system("mkdir C:\\SocketUS\\"); // Nếu chưa có thì tạo đường dẫn

        // Tạo tập tin để lưu danh sách các UID mail
        ofstream fout("C:\\SocketUS\\UIDL.txt");
        if (!fout) {
            cerr << "Error: Cannot create a file to save UIDL" << endl;
            return 1;
        }

        // Ghi các UID thành các dòng
        for (int i = 1; i <= countEmail; i++) {
            fout << array[i] << endl;
        }

        fout.close();

        // Gửi lệnh RETR để gửi tín hiệu nhận thư
        for (int i = 1; i <= countEmail; i++) {
            string retrCommand = "RETR " + to_string(i) + "\r\n";
            send(clientSocket, retrCommand.c_str(), retrCommand.length(), 0);

            // Xử lý nhận nội dung email
            processEmail(clientSocket, array[i]);
        }
    }
    else {
        // Đọc các nội dung trong file và lưu vào danh sách.
        while (fin >> arrayLocal[++countLocalEmail]);

        // So sánh xem UID đã tồn tại trên PC chưa
        for (int i = 1; i <= countEmail; i++) {

            // Set cờ để kiểm tra xem UID có tồn tại chưa
            bool flag = false;
            for (int j = 1; j <= countLocalEmail; j++) {
                if (array[i] == arrayLocal[j]) {
                    flag = true;
                    break;
                }
            }

            // Nếu chưa tồn tại thì gửi lệnh tải về
            if (!flag) {
                // Gửi lệnh để nhận mail
                string retrCommand = "RETR " + to_string(i) + "\r\n";
                send(clientSocket, retrCommand.c_str(), retrCommand.length(), 0);

                // Xử lý nhận nội dung email
                processEmail(clientSocket, array[i]);

                // Ghi UID mới vào tệp UIDL_email
                ofstream fout("C:\\SocketUS\\UIDL.txt", ios::app);
                fout << array[i] << endl;

                fout.close();
            }
        }
    }
    fin.close();

    // Gửi lệnh QUIT
    const char* quitCommand = "QUIT\r\n";
    send(clientSocket, quitCommand, strlen(quitCommand), 0);

    // Đọc phản hồi từ server
    sizeChar = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[sizeChar] = '\0';
    cout << "Server response: " << buffer << endl;

    // Đóng kết nối
    closesocket(clientSocket);
    WSACleanup();

    
}

// Hàm mã hóa file PDF và lưu vào một file mới
void encodePDF(const string& pdfFilePath, const string& encodedFilePath) {
    // Đọc nội dung file PDF
    string fileContent = readFile(pdfFilePath);

    // Mã hóa nội dung file PDF sang base64
    string encodedFileContent = base64_encode(fileContent);

    // Lưu nội dung đã mã hóa vào file mới
    ofstream encodedFile(encodedFilePath, ios::binary);
    encodedFile << encodedFileContent;
    encodedFile.close();

    cout << "Encoded content saved to: " << encodedFilePath << endl;
}

// Hàm giải mã và lưu nội dung file PDF từ file đã mã hóa
void decodePDF(const string& encodedFilePath, const string& saveFilePath) {
    // Đọc nội dung file đã mã hóa
    string encodedFileContent = readFile(encodedFilePath);

    // Giải mã nội dung từ base64
    string decodedContent = base64_decode(encodedFileContent);

    // Lưu nội dung đã giải mã vào file
    ofstream outputFile(saveFilePath, ios::binary);
    outputFile << decodedContent;
    outputFile.close();

    cout << "Decoded content saved to: " << saveFilePath << endl;
}
int main() {
    // Tinh nang gui email
    //sendEmailFunction();
    //rcvEmailFunction();
    //processEmailFile("mailbox\\", "D:\\Wall_paper\\", "email_1.txt");
    // Đường dẫn file PDF
    //const string pdfFilePath = "D:\\Tailieu_namnhat\\HKIII\\DSA\\Document\\2023-CSC10004-22CLC03-HW02-AlgorithmEfficiency.pdf";

    //// Đường dẫn để lưu file đã mã hóa
    //const string encodedFilePath = "D:\\Tailieu_namnhat\\HKIII\\DSA\\Document\\encoded_example.txt";

    //// Đường dẫn để lưu file đã giải mã
    //const string saveFilePath = "D:\\Tailieu_namnhat\\HKIII\\DSA\\Document\\encoded_example.pdf";

    //// Gọi hàm để mã hóa file PDF
    //encodePDF(pdfFilePath, encodedFilePath);

    //// Gọi hàm để giải mã và lưu file
    //decodePDF(encodedFilePath, saveFilePath);

    cout << "hello" << endl;
    return 0;
}


