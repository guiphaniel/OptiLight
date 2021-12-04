#pragma once
#include <iostream>
#include <string>

using namespace std;

class Couleur
{
public:
    Couleur(int r = 0, int g = 0, int b = 0);
    void reinit() { r = g = b = 0; }
    int getR() { return r; }
    int getG() { return g; }
    int getB() { return b; }
    void setR(int _r) { r = _r; }
    void setG(int _g) { g = _g; }
    void setB(int _b) { b = _b; }
    void addR(int _r) { r += _r; }
    void addG(int _g) { g += _g; }
    void addB(int _b) { b += _b; }
private:
    int r, g, b;
};

