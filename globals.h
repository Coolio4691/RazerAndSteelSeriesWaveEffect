#pragma once
#include <atomic>
#include <unordered_map>
#include "keyboard.h"
#include "mouse.h"
#include "password.h"

#include <python3.10/Python.h>

#include <glm/glm.hpp>

#include <json/value.h>

using namespace std;

struct vec2Hash {
    size_t operator()(const glm::vec2& k)const {
        return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
    }

    bool operator()(const glm::vec2& a, const glm::vec2& b)const {
        return a.x == b.x && a.y == b.y;
    }
};

bool vmOn = false;
bool hasPing = false;
atomic<bool> exitApp;
Keyboard* keyboard;

string mouseProductID;
string mouseVendorID;

string exePath;

Json::Value settings;

unordered_map<glm::vec2, customLED, vec2Hash> customKeyboardLEDS = {};
unordered_map<string, customMouseLED> customMouseLEDS = {};

//PyObject* locateOnScreen;
//PyObject* discordImgPy;