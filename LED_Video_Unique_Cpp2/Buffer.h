#pragma once
#include "Couleur.h"
#include "SerialPort.hpp"
#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <Wincodec.h>             // we use WIC for saving images
#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

class Buffer
{
public:
    Buffer(const char* pN, int nL, int w, int h, int eX, int eY);
	~Buffer();
    bool importData();
    bool exportData();
    void reinit();
private:    
	int nbLED;
    int width;
    int height;
    int echantillonageX;
    int echantillonageY;    
    int average;
    int portion;
    Couleur* buffer;
    Couleur* oldBuffer;
    const char* portName;
    SerialPort* arduino;

    Bitmap* p_bmp;
    HDC scrdc, memdc;
    HBITMAP membit;
};



