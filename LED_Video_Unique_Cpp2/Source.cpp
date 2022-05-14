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
HDC winDC, memDC;
HBITMAP oldMemBM, memBM;
BYTE* bmPointer;

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

	importData();

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
		if (!BitBlt(memDC, 0, 0, WIDTH, HEIGHT, winDC, 0, 0, SRCCOPY)) {
			cout << "screenshot failed" << endl;
			return false;
		}

		//fill tmpBuffer
		Color c; 
		for (int y = 0; y < HEIGHT; y += ECH_Y) {
			for (int x = 0; x < WIDTH; x += ECH_X) {
				tmpBuffer[x / PORTION].addR(bmPointer[x * 3 + y * 3* WIDTH] >> 1);
				tmpBuffer[x / PORTION].addG(bmPointer[x * 3+ 1 + y * 3 * WIDTH] >> 1);
				tmpBuffer[x / PORTION].addB(bmPointer[x * 3 + 2 + y * 3 * WIDTH] >> 1);
			}
		}

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
	winDC = ::GetDC(NULL);
	memDC = CreateCompatibleDC(winDC);

	BITMAPINFO bmInfo;
	bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
	bmInfo.bmiHeader.biWidth = WIDTH;
	bmInfo.bmiHeader.biHeight = HEIGHT;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	bmInfo.bmiHeader.biSizeImage = WIDTH * 3 * HEIGHT;
	bmInfo.bmiHeader.biClrUsed = 0;
	bmInfo.bmiHeader.biClrImportant = 0;
	memBM = CreateDIBSection(memDC, &bmInfo, DIB_RGB_COLORS, (void**)(&bmPointer), NULL, NULL); //TODO: DIB_PAL_COLORS ??

	oldMemBM = (HBITMAP)SelectObject(memDC, memBM);
}

void initArduino()
{
	portName = "\\\\.\\COM3";
	arduino = new SerialPort(portName);
}

void cleanup()
{
	//Shutdown GDI+
	SelectObject(memDC, oldMemBM);
	DeleteDC(memDC);
	GdiplusShutdown(gdiplusToken);

	delete arduino;
}
