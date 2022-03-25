#pragma once

#include "frame.h"
#include "colour.h"

#include <fmt/core.h>
#include <fmt/printf.h>

#include <string>
#include <qt/QtDBus/QDBusInterface>
#include <qt/QtDBus/QDBusArgument>

using namespace std;

struct columnValues {
    int col = 0;

    float hue = 0;
    bool reversed = false;
};

struct customLED {
public:
    float hue;
    float minHue;
    float maxHue;

    float colorStep = 0.01f;
    float colorBackStep = 0.025f;

    bool isRGB = false;
    bool reversed = false;
    RGB rgb;

    customLED(float minHue, float maxHue, float colorStep = 0.01f, float colorBackStep = 0.025f, bool reversed = false) : hue(minHue), minHue(minHue), maxHue(maxHue), colorStep(colorStep), colorBackStep(colorBackStep), isRGB(false), reversed(reversed) { }
    customLED(RGB rgb) : isRGB(true), rgb(rgb) { }

    ~customLED() {}  
};

class Keyboard {
private:
    string accessPath;

public:
    Frame* frame;
    QDBusInterface lighting_dbus;
    QDBusInterface misc_dbus;
    string id;

    string name;

    string vendorID;
    string productID;

    Keyboard(string id) : accessPath(string("/org/razer/device/").append(id)), 
    lighting_dbus("org.razer", this->accessPath.c_str(), "razer.device.lighting.chroma"), 
    misc_dbus("org.razer", this->accessPath.c_str(), "razer.device.misc"), 
    id(id) {
        QDBusMessage result = this->misc_dbus.call("getVidPid");

        QList<int> vidPid;

        QDBusArgument vidPidList = result.arguments().at(0).value<QDBusArgument>();
        vidPidList.beginArray();

        while (!vidPidList.atEnd()) {
            auto val = qdbus_cast<int>(vidPidList);
            vidPid.append(val);
        }       
        
        this->vendorID = fmt::sprintf("%04X", vidPid[0]);
        this->productID = fmt::sprintf("%04X", vidPid[1]);

        result = this->misc_dbus.call("getDeviceName");
        name = result.arguments().at(0).value<QString>().toStdString();


        QList<int> dimensions;

        result = misc_dbus.call("getMatrixDimensions");

        QDBusArgument dimList = result.arguments().at(0).value<QDBusArgument>();
        dimList.beginArray();

        while (!dimList.atEnd())
        {
            int val = qdbus_cast<int>(dimList);
            dimensions.append(val);
        }        

        this->frame = new Frame(dimensions[0], dimensions[1]);
    }

    void set_key(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
        this->frame->set_key(x, y, red, green, blue);
    }

    void set_col(int col, uint8_t red, uint8_t green, uint8_t blue) {
        
        for(size_t row = 0; row < this->frame->rows; row++) {

            this->frame->set_key(col, row, red, green, blue);
        }
    }

    vector<uint8_t> get_key(int x, int y) {
        return this->frame->get_key(x, y);
    }

    void draw() {
        lighting_dbus.callWithArgumentList(QDBus::CallMode::NoBlock, "setKeyRow", { this->frame->bytes() });
        lighting_dbus.call(QDBus::CallMode::NoBlock, "setCustom");
        
        this->frame->reset();
    }
};