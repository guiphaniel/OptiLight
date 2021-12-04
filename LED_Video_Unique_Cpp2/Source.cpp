#include <iostream>
#include "Couleur.h"
#include "SerialPort.hpp"
#include <windows.h>
#include <mutex> 
#include <thread> 
#include <gdiplus.h>
#include <memory>
#include <string>
#include <Wincodec.h>             // we use WIC for saving images
#pragma comment (lib,"Gdiplus.lib")

#define WIDTH 1920
#define HEIGHT 1080

#define NB_LED 40
#define ECH_X 20
#define ECH_Y 20
#define AVERAGE (int(1 + ((WIDTH / NB_LED) - 1) / ECH_X) * int(1 + (HEIGHT - 1) / ECH_Y))
#define PORTION (WIDTH / NB_LED)

using namespace std;
using namespace Gdiplus;

const char* portName;
SerialPort* arduino;
Couleur tmpBuffer[NB_LED];
Couleur buffer[NB_LED];
Couleur oldBuffer[NB_LED];

ULONG_PTR gdiplusToken;
Bitmap* p_bmp;
HDC scrdc, memdc;
HBITMAP membit;

mutex bufferMutex;
bool newBuffer = false;

bool importData();
void exportData();
void init();
void initGDI();
void initArduino();
void initScreenshot();
void cleanup();

int main() {		
	init();

	thread Timport(importData);
	Timport.join();

	cleanup();
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

		bufferMutex.lock();

		//set buffer to tmpBuffer
		for (int i = 0; i < NB_LED; i++) {
			buffer[i].setR(tmpBuffer[i].getR());
			buffer[i].setG(tmpBuffer[i].getG());
			buffer[i].setB(tmpBuffer[i].getB());
		}
		bufferMutex.unlock();
		thread Texport(exportData);
		Texport.detach();

		//save buffer in oldBuffer
		for (int i = 0; i < NB_LED; i++) {
			oldBuffer[i].setR(buffer[i].getR());
			oldBuffer[i].setG(buffer[i].getG());
			oldBuffer[i].setB(buffer[i].getB());
		}
	}
}

void exportData() {
	bufferMutex.lock();

	arduino->writeSerialPort(1);
	for (int i = 0; i < NB_LED; i++) {
		arduino->writeSerialPort(char(buffer[i].getR() / AVERAGE));
		arduino->writeSerialPort(char(buffer[i].getG() / AVERAGE));
		arduino->writeSerialPort(char(buffer[i].getB() / AVERAGE));
	}
	bufferMutex.unlock();
}

void init()
{
	initGDI();
	initArduino();
	initScreenshot();
}

void initGDI()
{
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void initScreenshot()
{
	scrdc = ::GetDC(NULL);
	memdc = CreateCompatibleDC(scrdc);
	membit = CreateCompatibleBitmap(scrdc, WIDTH, HEIGHT);
}

void initArduino()
{
	portName = "\\\\.\\COM3";
	arduino = new SerialPort(portName);
}

void cleanup()
{
	//Shutdown GDI+
	GdiplusShutdown(gdiplusToken);

	delete arduino;
}
