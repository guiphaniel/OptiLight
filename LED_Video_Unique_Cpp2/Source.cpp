#include <iostream>
#include "Buffer.h"

#define WIDTH 1920
#define HEIGHT 1080

using namespace std;

int main() {
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    Buffer buffer("\\\\.\\COM3", 40, width, height, 20, 100);

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
