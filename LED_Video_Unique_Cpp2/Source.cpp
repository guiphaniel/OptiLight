#include <iostream>
#include "Color.h"
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
project::Color* tmpBuffer;
project::Color* buffer;
project::Color* oldBuffer;

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
			project::Color& c = tmpBuffer[i];
			c.r = c.g = c.b = 0;
		}

		//screen capture
		if (!BitBlt(memDC, 0, 0, WIDTH, HEIGHT, winDC, 0, 0, SRCCOPY)) {
			cout << "screenshot failed" << endl;
			return false;
		}

		//fill tmpBuffer ; nb: bmPointer is BGR 
		for (int y = 0; y < HEIGHT; y += ECH_Y) {
			for (int x = 0; x < WIDTH; x += ECH_X) {
				project::Color& c = tmpBuffer[x / PORTION];
				c.b += bmPointer[x * 4 + y * 4 * WIDTH] >> 1; // * 3 for pixel depth
				c.g += bmPointer[x * 4 + 1 + y * 4 * WIDTH] >> 1;
				c.r += bmPointer[x * 4 + 2 + y * 4 * WIDTH] >> 1;
			}
		}

		//check if image is same as previous one
		bool same = true;
		for (int i = 0; i < NB_LED; i++) {
			project::Color& oldC = oldBuffer[i];
			project::Color& tmpC = tmpBuffer[i];
			if (oldC.r != tmpC.r || oldC.g != tmpC.g || oldC.b != tmpC.b) {
				same = false;
				break;
			}
		}

		if (same)
			continue;

		bufferMutex.lock();

		//set buffer to tmpBuffer
		project::Color* tmp;
		tmp = buffer;
		buffer = tmpBuffer;
		tmpBuffer = tmp;

		bufferMutex.unlock();

		thread Texport(exportData);
		Texport.detach();

		//save buffer in oldBuffer
		for (int i = 0; i < NB_LED; i++) {
			project::Color& oldC = oldBuffer[i];
			project::Color& buffC = buffer[i];
			oldC.r = buffC.r;
			oldC.g = buffC.g;
			oldC.b = buffC.b;
		}
	}
}

void exportData() {
	bufferMutex.lock();

	arduino->writeSerialPort(1);
	for (int i = 0; i < NB_LED; i++) {
		project::Color& c = buffer[i];
		arduino->writeSerialPort(c.r / AVERAGE);
		arduino->writeSerialPort(c.g / AVERAGE);
		arduino->writeSerialPort(c.b / AVERAGE);
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
	tmpBuffer = new project::Color[NB_LED];
	buffer = new project::Color[NB_LED];
	oldBuffer = new project::Color[NB_LED];

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
	memBM = CreateDIBSection(memDC, &bmInfo, DIB_PAL_COLORS, (void**)(&bmPointer), NULL, NULL);

	oldMemBM = (HBITMAP)SelectObject(memDC, memBM);
}

void initArduino()
{
	portName = "\\\\.\\COM3";
	arduino = new SerialPort(portName);
}

void cleanup()
{
	delete[] tmpBuffer;
	delete[] buffer;
	delete[] oldBuffer;

	//Shutdown GDI+
	SelectObject(memDC, oldMemBM);
	DeleteDC(memDC);
	GdiplusShutdown(gdiplusToken);

	delete arduino;
}
