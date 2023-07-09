/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
*/

#pragma once

#define ARDUINO_WAIT_TIME 2000
#define MAX_DATA_LENGTH 255

#include <windows.h>
#include <iostream>

using namespace std;

class SerialPort
{
private:
    HANDLE handler;
    bool connected;
    COMSTAT status;
    DWORD errors;
public:
    explicit SerialPort(const char* portName);
    ~SerialPort();

    int readSerialPort(const char* buffer, unsigned int buf_size);
    bool writeSerialPort(const uint8_t& buffer);
    bool writeSerialPort(const uint8_t* buffer, int size);
    bool isConnected();
    void closeSerial();
};