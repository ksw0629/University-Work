#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int recvAll(SOCKET sock, char* buffer, int size) {
    int received = 0;
    while (received < size) {
        int n = recv(sock, buffer + received, size - received, 0);
        if (n <= 0) return n;
        received += n;
    }
    return received;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "서버에 연결됨.\n";

    while (true) {
        cout << "\n명령 입력 (list / get 파일명): ";
        string cmd;
        getline(cin, cmd);

        send(sock, cmd.c_str(), cmd.size(), 0);

        // 1. list 요청 시 목록 수신
        if (cmd == "list") {

            int listSize = 0;

            // 서버가 보낸 목록 크기를 먼저 받음
            int ret = recvAll(sock, (char*)&listSize, sizeof(listSize));

            if (ret <= 0 || listSize <= 0) {
                cout << "목록 없음 또는 오류!\n";
                continue;
            }

            // 목록 내용을 크기만큼 정확히 수신
            char* buffer = new char[listSize + 1];
            recvAll(sock, buffer, listSize);
            buffer[listSize] = 0;

            cout << "\n--- 서버 파일 목록 ---\n" << buffer;

            delete[] buffer;
        }

        // 2. get <파일명> 요청 시 파일 수신
        else if (cmd.rfind("get ", 0) == 0) {
            int fileSize = 0;

            // 파일 크기 먼저 수신
            int ret = recvAll(sock, (char*)&fileSize, sizeof(fileSize));

            if (ret <= 0 || fileSize <= 0) {
                cout << "파일 없음 또는 오류!\n";
                continue;
            }

            string filename = cmd.substr(4);
            ofstream out(filename, ios::binary);

            char buffer[1024];
            int received = 0;

            cout << filename << " 수신 시작 (" << fileSize << " bytes)\n";

            while (received < fileSize) {
                int n = recv(sock, buffer, 1024, 0);
                if (n <= 0) break;
                out.write(buffer, n);
                received += n;
            }

            out.close();
            cout << "파일 저장 완료.\n";
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
