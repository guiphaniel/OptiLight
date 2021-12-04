#include <iostream>
#include "Couleur.h"
#include "SerialPort.hpp"
#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <Wincodec.h>             // we use WIC for saving images
#pragma comment (lib,"Gdiplus.lib")

#define WIDTH 1920
#define HEIGHT 1080

#define NB_LED 40
#define ECH_X 20
#define ECH_Y 100
#define AVERAGE (int(1 + ((WIDTH / NB_LED) - 1) / ECH_X) * int(1 + (HEIGHT - 1) / ECH_Y))
#define PORTION (WIDTH / NB_LED)

using namespace std;
using namespace Gdiplus;

const char* portName;
SerialPort arduino;
Couleur tmpBuffer[NB_LED];
Couleur buffer[NB_LED];
Couleur oldBuffer[NB_LED];

Bitmap* p_bmp;
HDC scrdc, memdc;
HBITMAP membit;

bool importData();
bool exportData();

int main() {
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);    

    //init arduino
    portName = "\\\\.\\COM3";
    arduino = SerialPort(portName);

    //init screenshot 
    scrdc = ::GetDC(NULL);
    memdc = CreateCompatibleDC(scrdc);
    membit = CreateCompatibleBitmap(scrdc, WIDTH, HEIGHT);

	//TODO: create mutex
	//TODO: wait for newBuffer's mutex

	//TODO: create threads

    while (true) {
        if (buffer.importData())
        {
            buffer.exportData();
        }
        buffer.reinit();
       // Sleep(1000/(60*4)); // *4 pour reduire la latence par 4
    }

    //Shutdown GDI+
    GdiplusShutdown(gdiplusToken);

    return 0;
}

bool importData() {
	while (true) {
		//reinit tmpBuffer
		for (int i = 0; i < NB_LED; i++) {
			tmpBuffer[i].reinit();
		}

		//screen capture
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(memdc, membit);
		if (!BitBlt(memdc, 0, 0, WIDTH, HEIGHT, scrdc, 0, 0, SRCCOPY)) {
			cout << "screenshot failed" << endl;
			return false;
		}

		p_bmp = Bitmap::FromHBITMAP(membit, NULL);

		//fill tmpBuffer
		Color c;
		for (int y = 0; y < HEIGHT; y += ECH_Y) {
			for (int x = 0; x < WIDTH; x += ECH_X) {
				p_bmp->GetPixel(x, y, &c);
				ARGB cValue = c.GetValue();
				tmpBuffer[x / PORTION].addR((cValue & 0xFF0000) >> 17);
				tmpBuffer[x / PORTION].addG((cValue & 0xFF00) >> 9);
				tmpBuffer[x / PORTION].addB((cValue & 0xFF) >> 1);
			}
		}
		delete p_bmp;

		//check if image is same as previous one
		bool same = true;
		for (int i = 0; i < NB_LED; i++) {
			if (oldBuffer[i].getR() != tmpBuffer[i].getR() || oldBuffer[i].getG() != tmpBuffer[i].getG() || oldBuffer[i].getB() != tmpBuffer[i].getB()) {
				same = false;
				break;
			}
		}

		if (same)
			continue;


		//set buffer to tmpBuffer
		//TODO: wait for buffer's mutex 
		for (int i = 0; i < NB_LED; i++) {
			buffer[i].setR(tmpBuffer[i].getR());
			buffer[i].setG(tmpBuffer[i].getG());
			buffer[i].setB(tmpBuffer[i].getB());
		}
		//TODO: release buffer's mutex 
		//TODO: release newBuffer's mutex 

		//save buffer in oldBuffer
		for (int i = 0; i < NB_LED; i++) {
			oldBuffer[i].setR(buffer[i].getR());
			oldBuffer[i].setG(buffer[i].getG());
			oldBuffer[i].setB(buffer[i].getB());
		}
	}
}

bool exportData() {
	while (true) {
		//TODO: wait for newBuffer's mutex
		//TODO: wait for buffer's mutex		
		arduino.writeSerialPort(1);
		for (int i = 0; i < NB_LED; i++) {
			arduino.writeSerialPort(char(buffer[i].getR() / AVERAGE));
			arduino.writeSerialPort(char(buffer[i].getG() / AVERAGE));
			arduino.writeSerialPort(char(buffer[i].getB() / AVERAGE));
		}
		//TODO: release buffer's mutex 
	}
}
