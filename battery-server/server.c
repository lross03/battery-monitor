#include <stdio.h>
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
        previousBattery = 0;//status.BatteryLifePercent;
        if(previousBattery == 255) {
            printf("Battery percentage unknown\n");
        }
        else {
            snprintf(buffer, sizeof(buffer), "%d\n", previousBattery);
            printf("Connected and sending battery percentage %d%%\n", previousBattery);

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
                    snprintf(buffer, sizeof(buffer), "%d", battery);
                    printf("Connected and sending battery percentage %s%%\n", buffer);
                    printf("Sending data: %s\n", buffer);

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


