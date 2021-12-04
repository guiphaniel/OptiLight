#include "Buffer.h"

Buffer::Buffer(const char* pN, int nL, int w, int h, int eX, int eY) : portName(pN), nbLED(nL), width(w), height(h), echantillonageX(eX), echantillonageY(eY)
{
	average = int(1 + ((width / nbLED) - 1) / echantillonageX) * int(1 + (height - 1) / echantillonageY);
	portion = width / nbLED;
	buffer = new Couleur[nbLED];
	oldBuffer = new Couleur[nbLED];
	arduino = new SerialPort(portName);

	scrdc = ::GetDC(NULL);
	memdc = CreateCompatibleDC(scrdc);
	membit = CreateCompatibleBitmap(scrdc, width, height);
}

Buffer::~Buffer()
{
	DeleteObject(memdc);
	DeleteObject(membit);
	::ReleaseDC(0, scrdc);
	delete buffer;
	delete arduino;
	delete portName;
}

bool Buffer::importData() {
	//screen capture
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(memdc, membit);
	if (!BitBlt(memdc, 0, 0, width, height, scrdc, 0, 0, SRCCOPY)) {
		cout << "screenshot failed" << endl;
		return false;
	}

	p_bmp = Bitmap::FromHBITMAP(membit, NULL);	

	//fill buffer
	Color c;
	for (int y = 0; y < height; y += echantillonageY) {
		for (int x = 0; x < width; x += echantillonageX) {
			p_bmp->GetPixel(x, y, &c);
			ARGB cValue = c.GetValue();
			buffer[x / portion].addR((cValue & 0xFF0000) >> 17);
			buffer[x / portion].addG((cValue & 0xFF00) >> 9);
			buffer[x / portion].addB((cValue & 0xFF) >> 1);
		}
	}
	delete p_bmp;

	bool same = true;
	for (int i = 0; i < nbLED; i++) {
		buffer[i].setR(buffer[i].getR() / average);
		buffer[i].setG(buffer[i].getG() / average);
		buffer[i].setB(buffer[i].getB() / average);

		if (oldBuffer[i].getR() != buffer[i].getR() || oldBuffer[i].getG() != buffer[i].getG() || oldBuffer[i].getB() != buffer[i].getB())
			same = false;

		oldBuffer[i].setR(buffer[i].getR());
		oldBuffer[i].setG(buffer[i].getG());
		oldBuffer[i].setB(buffer[i].getB());
	}	

	if (same)
		return false;
	else
		return true;
}

bool Buffer::exportData() {
	arduino->writeSerialPort(1);
	for (int i = 0; i < nbLED; i++) {
		arduino->writeSerialPort(char(buffer[i].getR()));
		arduino->writeSerialPort(char(buffer[i].getG()));
		arduino->writeSerialPort(char(buffer[i].getB()));
	}
	return true;
}

void Buffer::reinit()
{
	for (int i = 0; i < nbLED; i++)
		buffer[i].reinit();
}
