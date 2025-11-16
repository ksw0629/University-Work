#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

void sendAll(SOCKET sock, const char* buffer, int size) {
    int sent = 0;
    while (sent < size) {
        int n = send(sock, buffer + sent, size - sent, 0);
        if (n <= 0) break;
        sent += n;
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    cout << "[DEBUG] Current Path = "
        << fs::current_path() << endl;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 1);
    cout << "서버 시작. 클라이언트 기다리는 중..." << endl;

    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
    cout << "클라이언트 접속" << endl;

    while (true) {
        char cmd[256] = { 0 };
        recv(clientSocket, cmd, 256, 0);

        string command(cmd);

        // 1. "list" 요청 처리
        if (command == "list") {
            string list = "";

            for (const auto& entry : fs::directory_iterator(".")) {
                if (entry.path().extension() == ".txt" ||
                    entry.path().extension() == ".png")
                {
                    list += entry.path().filename().string() + "\n";
                }
            }

            // 목록 길이 먼저 전송
            int listSize = list.size();
            send(clientSocket, (char*)&listSize, sizeof(listSize), 0);

            // 목록 내용 전송
            if (listSize > 0)
                sendAll(clientSocket, list.c_str(), listSize);
        }

        // 2. "get <파일명>" 요청 처리
        else if (command.rfind("get ", 0) == 0) {
            string filename = command.substr(4);

            ifstream file(filename, ios::binary);
            if (!file.is_open()) {
                string msg = "ERROR: file not found";
                sendAll(clientSocket, msg.c_str(), msg.size());
                continue;
            }

            file.seekg(0, ios::end);
            int size = file.tellg();
            file.seekg(0);

            // 파일 크기 먼저 전송
            send(clientSocket, (char*)&size, sizeof(size), 0);

            // 파일 내용 전송
            char buffer[1024];
            while (!file.eof()) {
                file.read(buffer, 1024);
                int readSize = file.gcount();
                sendAll(clientSocket, buffer, readSize);
            }
            file.close();
        }
    }



    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
