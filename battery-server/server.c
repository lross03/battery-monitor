/*#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>


int main() {
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};

    hSerial = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hSerial == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening serial port\n");
        return 1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Error getting serial prot state\n");
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Error setting serial port state\n");
        CloseHandle(hSerial);
        return 1;
    }
    
    SYSTEM_POWER_STATUS status;
    int previousBattery;
    char buffer[1024];

    if(GetSystemPowerStatus(&status)) {
        previousBattery = status.BatteryLifePercent;
        if(previousBattery == 255) {
            printf("Battery percentage unknown\n");
        }
        else {
            snprintf(buffer, sizeof(buffer), "%d\n", previousBattery);
            
            //printf("Connected and sending battery percentage %d%%\n", previousBattery);

            DWORD bytesWritten;
            if (!WriteFile(hSerial, buffer, strlen(buffer), &bytesWritten, NULL)) {
                fprintf(stderr, "Error writing to serial port\n");
            }
        }
        
    }
    else {
        printf("Error reading battery status\n");
    }

    while(1) {
        if(GetSystemPowerStatus(&status)) {
            int battery = status.BatteryLifePercent;
            if(battery == 255) {
                printf("Battery percentage unknown\n");
            }
            else {
                if (abs(battery - previousBattery) >= 5){
                    snprintf(buffer, sizeof(buffer), "%d\n", battery);
                   // printf("Connected and sending battery percentage %s%%\n", buffer);
                    printf("%s", buffer);

                    DWORD bytesWritten;
                    if (!WriteFile(hSerial, buffer, strlen(buffer), &bytesWritten, NULL)) {
                        fprintf(stderr, "Error writing to serial prot\n");
                    }
                }
            }
        }
        else {
            printf("Error reading battery status\n");
        }
    }

    CloseHandle(hSerial);
    return 0;
}
*/

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define MAX_CONNECTIONS 2
#define BUFFER_SIZE 1024

int getBatteryStatus() {
    SYSTEM_POWER_STATUS status;
    while(1) {
        if(GetSystemPowerStatus(&status)) {
            int battery = status.BatteryLifePercent;
            if(battery == 255) {
                printf("Battery percentage unknown\n");
            }
            else {
                return battery;
            }
        }
        else {
            printf("Error reading battery status\n");
        }
    }

    return -1;
}

int main() {
    WSADATA wsaData;
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    int client_address_len = sizeof(client_address);

    char battery_str[20];


    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Windows Sockets\n");
        return 1;
    }

    //set up server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creating socket");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_address,
            sizeof(server_address)) < 0) { 
        
        printf("Failed to bind server socket\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, MAX_CONNECTIONS) < 0 ) {
        printf("Error listening\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Awaiting connections on port %d\n", PORT);
    if ((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len)) < 0) {
            printf("Error acceptng connection\n");
            return 1;
    }
    printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    while (1) {
        printf("start loop\n");

        int battery = getBatteryStatus();
        
        snprintf(battery_str, sizeof(battery_str), "%d\n", battery);

        if (battery >= 0 && battery <= 100) {
            printf("%s\n", battery_str);
            if(send(client_socket, battery_str, strlen(battery_str), 0) < 0){
                printf("Error sending message\n");
                return 1;
            }
            else {
                Sleep(1000);
            }
        } 
        else {
            printf("Error: battery percentage not within range\n");
        }
        printf("end loop\n");
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

