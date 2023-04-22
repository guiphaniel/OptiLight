#include <iostream>
#include "Color.h"
#include "SerialPort.hpp"
#include <windows.h>
#include <mutex> 
#include <thread> 
#include <condition_variable>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <Wincodec.h>             // we use WIC for saving images
#pragma comment (lib,"Gdiplus.lib")

#define WIDTH 1920
#define HEIGHT 1080

#define NB_LED 40
#define ECH_X 24
#define ECH_Y 27
#define PORTION (WIDTH / NB_LED)
#define AVERAGE (int(1 + ((WIDTH / NB_LED) - 1) / ECH_X) * int(1 + (HEIGHT - 1) / ECH_Y))

using namespace std;
using namespace Gdiplus;

const char* portName;
SerialPort* arduino;
project::Color* processingBuffer;
project::Color* screenshotBuffer;
project::Color* tmpBuffer;
uint64_t tmpChecksum;

mutex newTmpAvailableMutex;
condition_variable newTmpAvailableCv;
bool newTmpAvailable;

ULONG_PTR gdiplusToken;
HDC winDC, memDC;
HBITMAP oldMemBM, memBM;
BYTE* bmPointer;

mutex tmpBufferMutex;

bool importData();
void exportData();
void init();
void initGDI();
void initArduino();
void initScreenshot();
void cleanup();

int main() {
	init();

	thread Texport(exportData);
	Texport.detach();

	thread Tscreenshot(importData);
	Tscreenshot.join();

	cleanup();
	return 0;
}

bool importData() {
	while (true) {
		//screen capture
		if (!BitBlt(memDC, 0, 0, WIDTH, HEIGHT, winDC, 0, 0, SRCCOPY)) {
			cout << "screenshot failed" << endl;
			return false;
		}

		//fill screenshotBuffer ; nb: bmPointer is BGR 
		bool reinit = true;
		uint64_t checksum = 0;
		for (int y = 0; y < HEIGHT; y += ECH_Y) {
			for (int x = 0; x < WIDTH; x += ECH_X) {
				project::Color& c = screenshotBuffer[x / PORTION];

				if (reinit)
				{
					c.b = bmPointer[x * 4 + y * WIDTH * 4]; // * 4 for RGBA
					c.g = bmPointer[x * 4 + 1 + y * WIDTH * 4];
					c.r = bmPointer[x * 4 + 2 + y * WIDTH * 4];

					checksum += c.b + c.g + c.r;
					checksum = checksum << 3 | checksum >> (sizeof(checksum) * 8 - 3); // rotate
				}
				else
				{
					c.b += bmPointer[x * 4 + y * WIDTH * 4]; // * 4 for RGBA
					c.g += bmPointer[x * 4 + 1 + y * WIDTH * 4];
					c.r += bmPointer[x * 4 + 2 + y * WIDTH * 4];

					checksum += c.b + c.g + c.r;
					checksum = checksum << 3 | checksum >> (sizeof(checksum) * 8 - 3); // rotate
				}
			}
			reinit = false;
		}

		//check if image is same as previous one
		if (checksum != tmpChecksum)
			tmpChecksum = checksum;
		else
			continue;

		tmpBufferMutex.lock();
		swap(screenshotBuffer, tmpBuffer);
		tmpBufferMutex.unlock();

		{
			lock_guard<mutex> lk(newTmpAvailableMutex);
			newTmpAvailable = true;
		}

		newTmpAvailableCv.notify_one();
	}
}

void exportData() 
{
	while (true)
	{
		unique_lock<mutex> lk(newTmpAvailableMutex);
		newTmpAvailableCv.wait(lk, [] {return newTmpAvailable; }); // wait unlock au moment d'attendre, et lock après 
		newTmpAvailable = false;
		lk.unlock();

		tmpBufferMutex.lock();
		swap(tmpBuffer, processingBuffer);
		tmpBufferMutex.unlock();

		arduino->writeSerialPort(200);

		uint8_t serialized[NB_LED * 3];
		for (int i = 0; i < NB_LED; ++i) {
			project::Color& c = processingBuffer[i];
			serialized[i * 3] = (c.r / AVERAGE) >> 1;
			serialized[i * 3 + 1] = (c.g / AVERAGE) >> 1;
			serialized[i * 3 + 2] = (c.b / AVERAGE) >> 1;
		}

		arduino->writeSerialPort(serialized, NB_LED * 3);

		arduino->writeSerialPort(201);

	}
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
	tmpChecksum = 0;
	newTmpAvailable = false;

	screenshotBuffer = new project::Color[NB_LED];
	tmpBuffer = new project::Color[NB_LED];
	processingBuffer = new project::Color[NB_LED];

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
	delete[] processingBuffer;
	delete[] screenshotBuffer;
	delete[] tmpBuffer;

	//Shutdown GDI+
	SelectObject(memDC, oldMemBM);
	DeleteDC(memDC);
	GdiplusShutdown(gdiplusToken);

	delete arduino;
}
