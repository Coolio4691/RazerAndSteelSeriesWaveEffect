#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>

#include <chrono>
#include <thread>
#include <atomic>

#include <signal.h>

#include <json/value.h>

#include <qt/QtGui/QRgb>
#include <qt/QtDBus/QDBusConnection>
#include <qt/QtDBus/QDBusInterface>
#include <qt/QtDBus/QDBusMessage>
#include <qt/QtDBus/QDBusReply>
#include <qt/QtDBus/QDBusArgument>
#include <qt/QtCore/QVariant>
#include <qt/QtCore/QList>
#include <qt/QtCore/QtDebug>
#include <qt/QtDBus/QDBusMetaType>

#include <python3.10/Python.h>


#include "keyboard.h"
#include "mouse.h"
#include "checkerThread.h"
#include "util.h"

using namespace std;

QDBusMessage drawKeyboard;


float minHue = 0.64;
float maxHue = 0.84;

float colorStep = 0.01;
float colorBackStep = 0.025;

int mouseFramerate = 1/20*1000;
int keyboardFramerate = 1/20*1000;
int speed = 1/20*1000;


vector<mouseColumnValues> mouseCols = {};
vector<columnValues> keyboardCols = {};

PyObject* pyArg;

void mouseLoop() {    
    mouseFramerate = 1/settings["mouseFramerate"].asFloat()*1000;
    while(!exitApp) {
        for(auto& col : mouseCols) {
            float hue = col.hue;

            if(hue > maxHue) hue = maxHue;
            if(hue < minHue) hue = minHue;

            string hex = HSVtoHEX(hue, 1, 0.4);

            for(auto led : col.leds) {
                if(led.first) {
                    pyArg = PyUnicode_FromString(hex.c_str());
                    PyObject_CallOneArg(led.second, pyArg);

                    Py_DecRef(pyArg);
                }
            }
        }

        this_thread::sleep_for(chrono::milliseconds(mouseFramerate));
    }
}

void keyboardLoop() {
    keyboardFramerate = 1/settings["keyboardFramerate"].asFloat()*1000;

    while(!exitApp) { 
        for(auto& col : keyboardCols) {
            float hue = col.hue;

            if(hue > maxHue) hue = maxHue;
            if(hue < minHue) hue = minHue;

            RGrg RGB = HSVtoRGB(hue, 1, 0.4);

            keyboard->set_col(col.col, RGB.Red, RGB.Green, 102);
        }
        for(auto& led : customKeyboardLEDS) {
            RGB rgb;

            if(led.second.isRGB) rgb = led.second.rgb;
            else {
                float hue = led.second.hue;

                if(hue > led.second.maxHue) hue = led.second.maxHue;
                if(hue < led.second.minHue) hue = led.second.minHue;


                RGrg rgrg = HSVtoRGB(hue, 1, 0.4);
                rgb = RGB{rgrg.Red, rgrg.Green, 102};
            }

            keyboard->set_key(led.first.x, led.first.y, rgb.red, rgb.green, rgb.blue);
        }

        keyboard->draw();
        this_thread::sleep_for(chrono::milliseconds(keyboardFramerate));
    }
}

void hueChangerLoop() {
    speed = 1/settings["speed"].asFloat()*1000;

    while(!exitApp) { 
        for(auto& col : keyboardCols) {
            if(col.reversed) col.hue -= colorBackStep;
            else col.hue += colorStep;

            if(col.hue <= minHue) col.reversed = false;
            if(col.hue >= maxHue) col.reversed = true;
        }

        for(auto& col : mouseCols) {
            if(col.reversed) col.hue -= colorBackStep;
            else col.hue += colorStep;
            
            if(col.hue <= minHue) col.reversed = false;
            if(col.hue >= maxHue) col.reversed = true;
        }

        for(auto& led : customKeyboardLEDS) {
            if(led.second.reversed) led.second.hue -= led.second.colorBackStep;
            else led.second.hue += led.second.colorStep;

            if(led.second.hue <= led.second.minHue) led.second.reversed = false;
            if(led.second.hue >= led.second.maxHue) led.second.reversed = true;
        }
        
        /*
        for(auto& led : customMouseLEDS) {
            if(led.second.reversed) led.second.hue -= led.second.colorBackStep;
            else led.second.hue += led.second.colorStep;

            if(led.second.hue <= led.second.minHue) led.second.reversed = false;
            if(led.second.hue >= led.second.maxHue) led.second.reversed = true;
        }
        */
        this_thread::sleep_for(chrono::milliseconds(speed));
    }
}

void exitAppF() {
    this_thread::sleep_for(chrono::seconds(1));

    exit(0);
}

void signal_callback_handler (int signum) {
	printf("Caught signal %d\n",signum);	
    exitApp = true;

    exitAppF();
}

string executable_name() {
    string sp;
    ifstream("/proc/self/comm") >> sp;
    return sp;
}

string get_selfpath() {
    char buff[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1) {
        buff[len] = '\0';
        string pathStr = buff;

        pathStr = pathStr.substr(0, pathStr.size() - (executable_name().size()));

        return pathStr;
    }
    
    return "";
}

int main() {
    exitApp = false;
    signal(SIGINT, signal_callback_handler);
    
    exePath = get_selfpath();

    settings = openJSON(exePath + "settings.json");
    
    if(settings["checkVM"].asBool()) vmOn = virtualMachineOn();

    Py_Initialize();
    PyObject* rivalcfg = PyImport_ImportModule("rivalcfg");
    PyObject* moduleDict = PyModule_GetDict(rivalcfg);
    PyObject* callable = PyDict_GetItemString(moduleDict, "get_first_mouse");
    PyObject* mouse = PyObject_CallObject(callable, NULL);

    PyObject* vID = PyObject_GetAttrString(mouse, "vendor_id");
    mouseVendorID = fmt::sprintf("%04X", PyLong_AsLong(vID));

    PyObject* pID = PyObject_GetAttrString(mouse, "product_id");
    mouseProductID = fmt::sprintf("%04X", PyLong_AsLong(pID));
    
    Py_DecRef(pID);
    Py_DecRef(vID);    

    Py_DecRef(callable);
    Py_DecRef(moduleDict);
    Py_DecRef(rivalcfg);

    /*PyObject* pyautogui = PyImport_ImportModule("pyautogui");
    PyObject* pyautoguiDict = PyModule_GetDict(pyautogui);

    locateOnScreen = PyDict_GetItemString(pyautoguiDict, "locateOnScreen");

    Py_DecRef(pyautogui);
    Py_DecRef(pyautoguiDict);

    discordImgPy = PyTuple_Pack(1, PyUnicode_FromString(string(exePath + "discord.png").c_str()));
    */

    cout << "SessionBus Connected" << endl;
    cout << "SystemBus Connected" << endl << endl;

    QDBusMessage msg = QDBusMessage::createMethodCall("org.razer", "/org/razer", "razer.devices", "getDevices");
    QDBusMessage response = QDBusConnection::sessionBus().call(msg);
    
    if(response.arguments().size() >= 1) {
        keyboard = new Keyboard(response.arguments()[0].toString().toStdString());

        cout << "Got Keyboard with ID: " << keyboard->id << endl << endl;
    }
    else {
        return 1;
    }
    
    mouseCols.push_back({"mouseright", {"z3", "z5", "z7"}});
    mouseCols.push_back({"mousemiddle", {"logo", "wheel"}});
    mouseCols.push_back({"mouseleft", {"z2", "z4", "z6"}});

    minHue = settings["minHue"].asFloat();
    maxHue = settings["maxHue"].asFloat();

    colorStep = settings["colorStep"].asFloat();
    colorBackStep = settings["colorBackstep"].asFloat();

    int i = 0;
    for(size_t x = 0; x < keyboard->frame->cols; x++) {
        float hue = minHue + (colorStep * x);

        if(hue < minHue) hue = minHue;
        if(hue > maxHue) {
            hue = maxHue;
        }

        keyboardCols.push_back({(int)x, hue, false});
        
        i++;
    }

    PyObject* getAttr = PyObject_GetAttrString(mouse, "__getattr__");
    
    for(auto& col : mouseCols) {

        col.hue = minHue + (colorStep * i);

        if(col.hue < minHue) col.hue = minHue;
        if(col.hue > maxHue) col.hue = maxHue;

        i++;

        for(auto led : col.ledIndexes) {
            pyArg = PyUnicode_FromString(string("set_" + led.first + "_color").c_str());

            col.leds[led.second].second = PyObject_CallOneArg(getAttr, pyArg);

            Py_DecRef(pyArg);
        }
    }

    Py_DecRef(getAttr);

    for(auto i : customMouseLEDS) {
        for(auto col : mouseCols) {
            if(col.ledIndexes.find(i.first) != col.ledIndexes.end()) {
                col.leds[col.ledIndexes.at(i.first)].first = false;
                break;
            }
        }
    }

    vector<thread> threads;

    if(settings["checkVM"].asBool())
        threads.push_back(thread(&keyListener));
    if(settings["checkVM"].asBool() || settings["checkDiscord"].asBool())
        threads.push_back(thread(&checkerLoop));
    
    threads.push_back(thread(&hueChangerLoop));
    threads.push_back(thread(&keyboardLoop));
    threads.push_back(thread(&mouseLoop));

    for(thread& i : threads) {
        i.join();
    }
    
    for(auto i : mouseCols) {
        for(auto led : i.leds) {
            Py_DecRef(led.second);
        }
    }
    
    //Py_DecRef(locateOnScreen);
    Py_DecRef(mouse);
    Py_Finalize();

    return 0;
}