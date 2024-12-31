
#ifndef MULTIPLAYER_C
#define MULTIPLAYER_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winsock.h>

#define MP_DEFAULT_BUFFER_SIZE 2048
#define MP_MAX_CLIENTS 100

int MP_SERVER_PORT = 1155;
char *MP_SERVER_IP = "127.0.0.1";
bool MP_is_server = false;

typedef struct MPPacket { // 12 bytes
    int len;
    int type;
    bool is_broadcast;
} MPPacket;

SOCKET MPClient_socket;
SOCKET MPServer_socket;

SOCKET MP_clients[MP_MAX_CLIENTS];
int MP_clients_amount = 0;

void (*_MP_client_handle_recv)(MPPacket, void *) = NULL;
void (*_MP_server_handle_recv)(SOCKET, MPPacket, void *) = NULL;
void (*_MP_on_client_connected)(SOCKET) = NULL;
void (*_MP_on_client_disconnected)(SOCKET) = NULL;


DWORD WINAPI _MPClient_handle_received_data(void *data);

void MP_print_hex(const unsigned char *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf(" %02X, ", buf[i]);
    }
    printf("\n");
}

void MPClient_send(MPPacket packet, void *data) {

    if (packet.len > MP_DEFAULT_BUFFER_SIZE - sizeof(packet)) {
        fprintf(stderr, "Packet too big to send! Packet size: %d \n", packet.len);
        //exit(-1);
        return; // what could ever go wrong?
    }
    if (packet.len < 0) {
        fprintf(stderr, "Invalid packet size! Packet size: %d \n", packet.len);
        //exit(-1);
        return; // :)
    }

    char buff[MP_DEFAULT_BUFFER_SIZE] = {0};

    memcpy(buff, &packet, sizeof(packet));
    memcpy(buff + sizeof(packet), data, packet.len);

    send(MPClient_socket, buff, packet.len + sizeof(MPPacket), 0);
}

void MP_init(const int port) {
    MP_SERVER_PORT = port;
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
}

DWORD WINAPI _MPClient(void *ip) {
    MP_SERVER_IP = (char *)ip;

    MPClient_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr = {
        .sin_port = htons(MP_SERVER_PORT),
        .sin_family = AF_INET,
        .sin_addr.S_un.S_addr = inet_addr(MP_SERVER_IP)
    };

    int res = connect(MPClient_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (res != SOCKET_ERROR) {
        printf("Client connected. \n");
    } else {
        printf("error %d \n", res);
        exit(1);
    }
    
    HANDLE h = CreateThread(NULL, 0, _MPClient_handle_received_data, (PVOID)MPClient_socket, 0, NULL);
    CloseHandle(h);
}

void MPClient(char *ip) {
    HANDLE h = CreateThread(NULL, 0, _MPClient, ip, 0, NULL);
    CloseHandle(h);
}


int MP_full_recv(SOCKET sock, char *buf, int size) {
    int bytes_received = recv(sock, buf, size, 0);

    MPPacket *first_packet = buf;

    if (first_packet->len > MP_DEFAULT_BUFFER_SIZE) {
        printf("Corrupted first packet \n");
        commit_sudoku();
    }

    int total_bytes_received = bytes_received;


    MPPacket *current_packet = first_packet;

    int buffer_end = (int)buf + total_bytes_received;

    int bytes_left = total_bytes_received;

    while ((char *)current_packet + sizeof(MPPacket) + current_packet->len < (char *)buffer_end) {
        current_packet = (char *)current_packet + current_packet->len + sizeof(MPPacket);
    }

    while ((char *)current_packet + current_packet->len > (char *)buffer_end) {
        int bytes = recv(sock, buf + total_bytes_received, first_packet->len - (total_bytes_received - sizeof(MPPacket)), 0);
        total_bytes_received += bytes;
        buffer_end = (int)buf + total_bytes_received;

        while ((char *)current_packet + sizeof(MPPacket) + current_packet->len < (char *)buffer_end) {
            current_packet = (char *)current_packet + current_packet->len + sizeof(MPPacket);
        }
    }


    return total_bytes_received;
}

DWORD WINAPI _MPClient_handle_received_data(void *data) {
    SOCKET client_socket = (SOCKET)data;

    
    
    while (TRUE) {
        
        char receive_buffer[MP_DEFAULT_BUFFER_SIZE] = {0};

        char *data_ptr = receive_buffer;

        int bytes_received = MP_full_recv(client_socket, receive_buffer, MP_DEFAULT_BUFFER_SIZE);

        if (bytes_received == SOCKET_ERROR) {
            fprintf(stderr, "Received socket error. Disconnecting... \n");
            shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
            exit(-1);
        }


        while (data_ptr < receive_buffer + bytes_received) {

            int data_ptr_num = (int)data_ptr;
            int receive_buffer_num = (int)receive_buffer;
            int diff = data_ptr_num - receive_buffer_num;

            MPPacket *packet = data_ptr;
            
            
            // process packet from server...
            if (_MP_client_handle_recv != NULL) {
                _MP_client_handle_recv(*packet, data_ptr + sizeof(MPPacket));
            }

            if (packet->len > MP_DEFAULT_BUFFER_SIZE) {
                printf("Packet length bigger than 1024! \n");
                commit_sudoku();
            }

            data_ptr += sizeof(MPPacket) + packet->len;
        }

        
    }
}

void _MPServer_disconnect_client(SOCKET client_socket) {
    int idx = -1;
    for (int i = 0; i < MP_clients_amount; i++) {
        if (MP_clients[i] == client_socket) {
            idx = i;
            break;
        }
    }

    SOCKET temp = MP_clients[idx];
    MP_clients[idx] = MP_clients[MP_clients_amount - 1];
    MP_clients[MP_clients_amount - 1] = temp;
    MP_clients_amount--;
}

DWORD WINAPI _MPServer_handle_client(void *data) {
    SOCKET client_socket = (SOCKET)data;

    
    MP_clients[MP_clients_amount++] = client_socket;
   
    
    if (_MP_on_client_connected != NULL) {
        _MP_on_client_connected(client_socket);
    }



    while (TRUE) {
        
        char receive_buffer[MP_DEFAULT_BUFFER_SIZE] = {0};
        
        int bytes_received = MP_full_recv(client_socket, receive_buffer, MP_DEFAULT_BUFFER_SIZE);

        if (bytes_received <= 0) {
            if (bytes_received == SOCKET_ERROR) {
                printf("Received socket error. Attempting to disconnect client. \n");
            } else {
                printf("Client disconnected! \n");
            }
            _MPServer_disconnect_client(client_socket);

            if (_MP_on_client_disconnected != NULL) {
                _MP_on_client_disconnected(client_socket);
            }

            return 0;
        }

        char *data_ptr = receive_buffer;

        while (data_ptr < receive_buffer + bytes_received) {
            MPPacket *packet = data_ptr;

            // process packet from server...
            if (_MP_client_handle_recv != NULL) {
                _MP_server_handle_recv(client_socket, *packet, data_ptr + sizeof(MPPacket));
            }

            data_ptr += sizeof(MPPacket) + packet->len;
        }
    }
}

void MPServer_send(MPPacket packet, void *data) {

    if (packet.len > MP_DEFAULT_BUFFER_SIZE - sizeof(packet)) {
        fprintf(stderr, "Packet too big to send! Packet size: %d \n", packet.len);
        exit(-1);
    }
    if (packet.len < 0) {
        fprintf(stderr, "Negative packet size! Packet size: %d \n", packet.len);
        exit(-1);
    }

    char buf[MP_DEFAULT_BUFFER_SIZE] = {0};

    memcpy(buf, &packet, sizeof(packet));

    memcpy(buf + sizeof(packet), data, packet.len);

    for (int i = 0; i < MP_clients_amount; i++) {
        send(MP_clients[i], buf, packet.len + sizeof(MPPacket), 0);
    }
}

void MPServer_send_to(MPPacket packet, void *data, SOCKET target) {
    char buf[MP_DEFAULT_BUFFER_SIZE] = {0};

    memcpy(buf, &packet, sizeof(packet));

    memcpy(buf + sizeof(packet), data, packet.len);

    send(target, buf, packet.len + sizeof(MPPacket), 0);
    
}

DWORD WINAPI _MPServer(void *data) {
    SOCKET client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_size = sizeof(client_addr);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket created.\n");

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(MP_SERVER_PORT);

    // Bind
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Bind done.\n");

    listen(server_socket, SOMAXCONN);
    printf("Listening on port %d. \n", MP_SERVER_PORT);

    MP_is_server = true;

    while (TRUE) {
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);

        HANDLE h = CreateThread(NULL, 0, _MPServer_handle_client, (PVOID)client_socket, 0, NULL);
        CloseHandle(h);
    }

    printf("how did you even get here? im out. \n");
}

void MPServer() {
    HANDLE h = CreateThread(NULL, 0, _MPServer, NULL, 0, NULL);
    CloseHandle(h);
}

void MP_set_port(int port) {
    MP_SERVER_PORT = port;
}

#endif