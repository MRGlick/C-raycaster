#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include "mystring.c"

bool winsock_initialized() {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET){
        return false;
    }

    closesocket(s);
    return true;
}

String get_local_ip() {
    IP_ADAPTER_INFO adapterInfo[16]; // Allocate space for up to 16 adapters
    DWORD bufferSize = sizeof(adapterInfo);

    GetAdaptersInfo(adapterInfo, &bufferSize);
    PIP_ADAPTER_INFO adapter = adapterInfo;

    return String(adapter->IpAddressList.IpAddress.String);
}


String get_public_ip() {
    WSADATA bruh;
    WSAStartup(MAKEWORD(2, 2), &bruh);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Error creating socket: %d\n", WSAGetLastError());
        return String_null;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    server.sin_addr.s_addr = inet_addr("172.67.74.152"); // IP for api.ipify.org

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Error connecting: %d\n", WSAGetLastError());
        closesocket(sock);
        return String_null;
    }

    // Send HTTP GET request
    const char* request = "GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n";
    send(sock, request, strlen(request), 0);

    // Receive response
    char buffer[512];
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0'; // Null-terminate the response
    }

    closesocket(sock);
    WSACleanup();

    String result = String(buffer);
    StringRef *parts = String_split(result, '\n');

    String ip = String_copy(parts[array_length(parts) - 1]);

    array_free(parts);

    String_delete(&result);

    return ip;
}


int main() {


    printf("da fuc \n");

    printf("public ip: %s \n", get_public_ip());

    printf("local ip: %s \n", get_local_ip());
}