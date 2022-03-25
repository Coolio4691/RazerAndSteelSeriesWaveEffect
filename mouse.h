#pragma once

#include <string>
#include <python3.10/Python.h>
#include <unordered_map>

using namespace std;

struct customMouseLED {
public:
    float hue;
    float minHue;
    float maxHue;

    float colorStep = 0.01f;
    float colorBackStep = 0.025f;

    bool isRGB = false;
    bool reversed = false;
    RGB rgb;

    customMouseLED(float minHue, float maxHue, float colorStep = 0.01f, float colorBackStep = 0.025f, bool reversed = false) : hue(minHue), minHue(minHue), maxHue(maxHue), colorStep(colorStep), colorBackStep(colorBackStep), isRGB(false), reversed(reversed) { }
    customMouseLED(RGB rgb) : isRGB(true), rgb(rgb) { }

    ~customMouseLED() {}  
};

struct mouseColumnValues {
    string col = "";
    vector<pair<bool, PyObject*>> leds = {};
    unordered_map<string, int> ledIndexes = {};

    float hue = 0;
    bool reversed = false;

    mouseColumnValues(string col, vector<string> leds) : col(col) {
        int i = 0;
        for(string led : leds) {
            this->leds.push_back({true, nullptr});
            ledIndexes.insert(pair<string, int>(led, i));
            i++;
        }
    }
};
