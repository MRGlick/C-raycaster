
#ifndef MULTIPLAYER_C
#define MULTIPLAYER_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winsock.h>

#define DEFAULT_BUFFER_SIZE 1024

int SERVER_PORT = 1155;
char *SERVER_IP = "127.0.0.1";

typedef struct MPPacket {
    void *data;
    int len;
    bool is_broadcast;
} MPPacket;

void (*_MP_client_handle_recv)(MPPacket) = NULL;
void (*_MP_server_handle_recv)(SOCKET, MPPacket) = NULL;
void (*_MP_on_client_connected)(SOCKET) = NULL;
void (*_MP_on_client_disconnected)(SOCKET) = NULL;

void MPClient_send(MPPacket packet) {

    if (packet.len > DEFAULT_BUFFER_SIZE - sizeof(packet)) {
        fprintf(stderr, "Packet too big! Packet size: %d \n", packet.len);
        exit(-1);
    }

    char buff[DEFAULT_BUFFER_SIZE] = {0};

    memcpy(buff, &packet, sizeof(packet));
    memcpy(buff + sizeof(packet), packet.data, packet.len);

    send(MPClient_socket, &packet, DEFAULT_BUFFER_SIZE, 0);
}

void MP_init(const int port) {
    SERVER_PORT = port;
    WSAStartup(MAKEWORD(2, 2), NULL);
}

void MPClient(const char *ip) {
    SERVER_IP = ip;

    MPClient_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr = {
        .sin_port = htons(SERVER_PORT),
        .sin_family = AF_INET,
        .sin_addr.S_un.S_addr = inet_addr(SERVER_IP)
    };

    int res = connect(MPClient_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (res != SOCKET_ERROR) {
        printf("Input some text!! \n");
    } else {
        printf("error %d \n", res);
        exit(1);
    }
    
    HANDLE h = CreateThread(NULL, 0, _MPClient_handle_received_data, (PVOID)MPClient_socket, 0, NULL);
    CloseHandle(h);
}

SOCKET MPClient_socket;
SOCKET MPServer_socket;

DWORD WINAPI _MPClient_handle_received_data(void *data) {
    SOCKET client_socket = (SOCKET)data;

    char receive_buffer[DEFAULT_BUFFER_SIZE] = {0};
    
    while (TRUE) {

        recv(client_socket, receive_buffer, DEFAULT_BUFFER_SIZE, 0);

        MPPacket *packet = receive_buffer;
        // process packet from server...
        if (_MP_client_handle_recv != NULL) {
            _MP_client_handle_recv((MPPacket){.data = packet->data, .len =packet->len });
        }
    }
}

DWORD WINAPI _MPServer_handle_client(void *data) {
    SOCKET client_socket = (SOCKET)data;

    if (_MP_on_client_connected != NULL) {
        _MP_on_client_connected(client_socket);
    }
    


    char receive_buffer[DEFAULT_BUFFER_SIZE] = {0};

    while (TRUE) {
        int result = recv(client_socket, receive_buffer, DEFAULT_BUFFER_SIZE, 0);

        if (result <= 0) {
            printf("Client disconnected! \n");
            if (_MP_on_client_disconnected != NULL) {
                _MP_on_client_disconnected(client_socket);
            }
            return; 
        }

        MPPacket *packet = receive_buffer;
        packet->data = packet + 1;

        if (packet->len > DEFAULT_BUFFER_SIZE - sizeof(int)) {
            fprintf(stderr, "The packet is too big! packet size: %d \n", packet->len);
            exit(-1);
        }

        // do stuff with the client's message...
        if (_MP_server_handle_recv != NULL) {
            _MP_server_handle_recv(client_socket, (MPPacket){.data = packet->data, .len = packet->len, .is_broadcast = packet->is_broadcast});
        }
    }
}

void MPServer() {

    SOCKET client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_size = sizeof(client_addr);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket created.\n");

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Bind done.\n");

    listen(server_socket, SOMAXCONN);
    printf("Listening on port %d. \n", SERVER_PORT);

    while (TRUE) {
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);

        HANDLE h = CreateThread(NULL, 0, _MPServer_handle_client, (PVOID)client_socket, 0, NULL);
        CloseHandle(h);
    }

    printf("how did you even get here? im out. \n");
}

#endif