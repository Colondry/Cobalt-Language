#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <stdbool.h>
#include <string.h>
#include <string>
#include "network.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

extern "C" {
    void DownloadUrl(const wchar_t* host, const wchar_t* path) {
        HINTERNET session = NULL, connect = NULL, request = NULL;
        DWORD size = 0, downloaded = 0;
        char buffer[4096];

        session = WinHttpOpen(L"AWinHTTPClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) return;

        connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!connect) { WinHttpCloseHandle(session); return; }

        request = WinHttpOpenRequest(connect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return; }

        if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            if (WinHttpReceiveResponse(request, NULL)) {
                do {
                    size = 0;
                    if (WinHttpQueryDataAvailable(request, &size) && size > 0) {
                        if (size > sizeof(buffer) - 1) size = sizeof(buffer) - 1;
                        if (WinHttpReadData(request, buffer, size, &downloaded)) {
                            buffer[downloaded] = '\0';
                            printf("%s", buffer);
                        }
                    }
                } while (size > 0);
            }
        }

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
    }

}

static bool checkTCP(int port) {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    bool connected;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(port);

    connected = (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0);

    closesocket(sock);
    WSACleanup();
    return connected;
}
static char addition[512] = "\n";
static char title[32] = "MyTCP";
static int listenPort = 8080;

extern "C" {

    void ServerBody(const char ui[512]) {
        snprintf(addition, sizeof(addition), "\n%.*s\n", (int)sizeof(addition) - 3, ui);
    }

    void ServerTitle(char temp_title[32]) {
        strncpy(title, temp_title, sizeof(title) - 1);
        title[sizeof(title) - 1] = '\0';
    }

    void RServer(int port) {
        WSADATA wsa;
        SOCKET server_sock, client_sock;
        struct sockaddr_in server, client;
        int c;
        char client_reply[2048];
        listenPort = port;

        WSAStartup(MAKEWORD(2, 2), &wsa);
        server_sock = socket(AF_INET, SOCK_STREAM, 0);

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_port = htons(listenPort);

        bind(server_sock, (struct sockaddr*)&server, sizeof(server));
        listen(server_sock, 3);

        printf("Server running at http://localhost:%d/\n", port);
        c = sizeof(struct sockaddr_in);

        while ((client_sock = accept(server_sock, (struct sockaddr*)&client, &c)) != INVALID_SOCKET) {
            int received = recv(client_sock, client_reply, sizeof(client_reply) - 1, 0);
            if (received >= 0) client_reply[received] = '\0'; // request itself isn't otherwise used

            std::string body =
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<!DOCTYPE html><html>"
                "<head><title>";
            body += title;
            body += "</title>\n</head>\n<body>";
            body += addition;
            body += "\n</body>\n</html>\n";

            send(client_sock, body.c_str(), (int)body.size(), 0);
            closesocket(client_sock);
        }
        closesocket(server_sock);
        WSACleanup();
    }

}