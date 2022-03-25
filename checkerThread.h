#pragma once
#include <iostream>

#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <jpeglib.h>

#include <fstream>
#include <string>
#include <fcntl.h>

#include <thread>
#include <chrono>

#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <regex>

#include "robin_hood.h"
#include "util.h"
#include "globals.h"

using namespace std;

string open_device (std::string vendorId, string productId, bool keyboard, int offset = 0) {       
    ifstream file_input;
    size_t pos;
    string device_path, current_line, search_str, event_str, device_list_file = "/proc/bus/input/devices";
    bool vid_pid_found = false;
    
    file_input.open(device_list_file.c_str());

    search_str = "Vendor=" + vendorId + " Product=" + productId;
    while (getline(file_input, current_line)) {
        if (!vid_pid_found) {
            pos = current_line.find(search_str);
            if (pos != string::npos) {
                vid_pid_found = true;
                search_str = "event";
            }               
        }
        else {
            pos = current_line.find(search_str);
            if (pos != string::npos) {
                event_str = current_line.substr(pos);
                vector<string> listStr = split(current_line.substr(9, pos), ' ');

                for(string i : listStr) {
                    if(i == "leds") {
                        vid_pid_found = false;
                        
                        break;
                    }

                    if(keyboard && i.find("mouse") != string::npos) {
                        vid_pid_found = false;
                        
                        break;
                    }
                    else if(!keyboard && i.find("keyboard") != string::npos) {
                        vid_pid_found = false;
                        
                        break;
                    } 
                }

                if(!vid_pid_found) {
                    
                    search_str = "Vendor=" + vendorId + " Product=" + productId;

                    continue;
                }

                event_str = split(event_str, ' ')[0];
                break;
            }
        }
    }

    string numbers = regex_replace(event_str, regex("[^0-9]"), "");
    string text = regex_replace(event_str, regex("[0-9]"), "");

    numbers = to_string(stoi(numbers) + offset);

    return "/dev/input/" + text + numbers;
}

bool findImage(int match_method, cv::Mat img, cv::Mat templ);

XImage* ImageFromDisplay() {
    Display* display = XOpenDisplay(0);
    Window root = DefaultRootWindow(display);

    int monitorCnt;

    XRRMonitorInfo * xMonitors = XRRGetMonitors(display, root, true, &monitorCnt );

    XImage* img;

    bool gotImg = false;

    for(int m = 0; m < monitorCnt; m++) {
        if(xMonitors[m].primary) {
            gotImg = true;
            img = XGetImage(display, root, xMonitors[m].x, xMonitors[m].y, 36, xMonitors[m].height, AllPlanes, ZPixmap);
        }
    }

    if(!gotImg) {
        img = XGetImage(display, root, xMonitors[0].x, xMonitors[0].y, 36, xMonitors[0].height, AllPlanes, ZPixmap);
    }

    XCloseDisplay(display);
    return img;
}

int write_jpeg(XImage *img, const char* filename) {
    FILE* outfile;
    unsigned long pixel;
    int x, y;
    char* buffer;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;
    JSAMPROW row_pointer;

    outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open output file %s", filename);
        return 1;
    }

    /* collect separate RGB values to a buffer */
    buffer = (char*)malloc(sizeof(char)*3*img->width*img->height);
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            pixel = XGetPixel(img,x,y);
            buffer[y*img->width*3+x*3+0] = (char)(pixel>>16);
            buffer[y*img->width*3+x*3+1] = (char)((pixel&0x00ff00)>>8);
            buffer[y*img->width*3+x*3+2] = (char)(pixel&0x0000ff);
        }
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = img->width;
    cinfo.image_height = img->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline
                      *(img->depth>>3)*img->width];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    free(buffer);
    jpeg_finish_compress(&cinfo);
    fclose(outfile);

    return 0;
}

bool findImage(int match_method, cv::Mat img, cv::Mat templ) {
    /// Create the result matrix
    cv::Mat result;

    int result_cols =  img.cols - templ.cols + 1;
    int result_rows = img.rows - templ.rows + 1;

    result.create( result_rows, result_cols, CV_32FC1 );

    /// Do the Matching and Normalize
    matchTemplate( img, templ, result, match_method );
    normalize( result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
    /// Localizing the best match with minMaxLoc
    double minVal; 
    double maxVal; 
    cv::Point minLoc; 
    cv::Point maxLoc;
    cv::Point matchLoc;

    minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );

    /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
    if(match_method  == cv::TM_SQDIFF || match_method == cv::TM_SQDIFF_NORMED) { 
        matchLoc = minLoc; 
    }
    else { 
        matchLoc = maxLoc; 
    }

    if(minVal < 0) return 1;

    return 0;
}

vector<int> shortcutKeys = {};

bool inputLocked = false;
int keyQueue = 0;
bool skipCheck = false;
bool lockReq = false;

robin_hood::unordered_flat_map<int, bool> keys; 

int keyboardFD;
int mouseFD;

void keyDown(int key) {
    keys[key] = true;
    keyQueue++;

    if(inputLocked) {
        bool hasAll = true;

        for(int i : shortcutKeys) {
            if(!keys[i]) {
                hasAll = false;
                break;
            }
        }

        if(hasAll) {
            inputLocked = !inputLocked;
            ioctl(keyboardFD, EVIOCGRAB, inputLocked);
            ioctl(mouseFD, EVIOCGRAB, inputLocked);

            skipCheck = true;
        }
    }
}

void keyUp(int key) {
    if(settings["lockDevices"] && !skipCheck) {
        bool hasAll = true;

        for(int i : shortcutKeys) {
            if(!keys[i]) {
                hasAll = false;
                break;
            }
        }

        if(hasAll) lockReq = true;
    }

    keys[key] = false;
    skipCheck = false;
    keyQueue--;

    if(keyQueue <= 0 && lockReq) {
        inputLocked = !inputLocked;
        ioctl(keyboardFD, EVIOCGRAB, inputLocked);
        ioctl(mouseFD, EVIOCGRAB, inputLocked);

        lockReq = false;
    }
}

struct input_event InputEvent[64];
struct input_event InputEventMouse[64];

string keyboardPath;
string mousePath;

bool vmToggleCooldown = false;

void keyListener() {
    keyboardPath = open_device(keyboard->vendorID, keyboard->productID, true, 0);
    keyboardFD = open(keyboardPath.c_str(), O_RDONLY);

    mousePath = open_device(mouseVendorID, mouseProductID, false, 0);
    mouseFD = open(mousePath.c_str(), O_RDONLY);    

    for(auto i : settings["lockShortcut"]) {
        shortcutKeys.push_back(i.asInt());
    }

    while(!exitApp) {
        int ReadDevice = read(keyboardFD, InputEvent, sizeof(struct input_event) * 64);

        if (ReadDevice < (int) sizeof(struct input_event)) {
            perror("KeyboardMonitor error reading - keyboard lost?");
            close(keyboardFD);
            close(mouseFD);
            return;
        }
        else {
            for (long unsigned int i = 0; i < ReadDevice / sizeof(struct input_event); i++) {

                if (InputEvent[i].type == EV_KEY) {
                    switch (InputEvent[i].value) {
                        case 0:
                            keyUp(InputEvent[i].code);

                            break;
                        case 1:
                            keyDown(InputEvent[i].code);

                            break;
                        default:
                            break;
                    }
                }
            }
        }
        
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

bool fakeOff = false;
bool prevState = false;
int checkPingVal = 5;
int checkPing = 0;

cv::Mat discord;

void checkVM() {
    vmOn = virtualMachineOn();

    if(prevState != vmOn) {
        vmToggleCooldown = false;

        if(!fakeOff && vmOn) {
            customKeyboardLEDS.erase(glm::vec2(16, 0));
        }
        else {
            fakeOff = false;
            customKeyboardLEDS.insert(make_pair(glm::vec2(16, 0), customLED(RGB{24, 0, 0})));
        }
    }

    prevState = vmOn;
}

void checkDiscord() {
    XImage* xImg = ImageFromDisplay();
    write_jpeg(xImg, ".screenshot.png");
    XDestroyImage(xImg);
    delete xImg;

    cv::Mat img = cv::imread(".screenshot.png");

    bool oldPing = hasPing;
    hasPing = findImage(cv::TM_CCOEFF, img, discord);

    if(oldPing != hasPing) {
        if(!hasPing) {
            customKeyboardLEDS.erase(glm::vec2(1, 0));
        }
        else {
            customKeyboardLEDS.insert(make_pair(glm::vec2(1, 0), customLED(RGB{24, 0, 0})));
        }
    }
}

int toVMToggle = 0;
int neededToVMToggle = 2;

void checkerLoop() {    
    if(settings["checkDiscord"].asBool()) {
        discord = cv::imread(exePath + "discord.png");

        checkDiscord();
    }

    /* python way to check ping 
    PyObject* kwargs = PyDict_New();

    PyObject* regionTuple = PyTuple_Pack(4, PyLong_FromLong(1280), PyLong_FromLong(0), PyLong_FromLong(36), PyLong_FromLong(1080));

    PyDict_SetItemString(kwargs, "region", regionTuple);
    PyDict_SetItemString(kwargs, "grayscale", Py_True);    

    PyObject* pyobj = PyObject_Call(locateOnScreen, discordImgPy, kwargs);//, args, kwargs);

    Py_DecRef(kwargs);
    Py_DecRef(regionTuple);

    */

    prevState = !vmOn;

    while(!exitApp) {
        if(settings["checkVM"].asBool()) {
            checkVM();

            if(toVMToggle >= neededToVMToggle) {
                vmToggleCooldown = true;
                    
                if(!vmOn) {    
                    execCommand("echo \"" + password + "\" | sudo -S /home/kian/Executables/VMMode.sh > /dev/null");
                }
                else {    
                    execCommand("echo \"" + password + "\" | sudo -S /home/kian/Executables/VMMode.sh off > /dev/null");
                    if(settings["fakeOff"])
                        fakeOff = true;

                    customKeyboardLEDS.insert(make_pair(glm::vec2(16, 0), customLED(RGB{24, 0, 0})));
                }

                toVMToggle = 0;
            }

            if(keys[KEY_RIGHTALT] && !vmToggleCooldown)
                toVMToggle += 1;
            else    
                toVMToggle = 0;
        }

        if(settings["checkDiscord"].asBool()) {
            if(checkPing >= checkPingVal) {
                checkDiscord();

                checkPing = 0;
            }

            checkPing += 1;
        }

        this_thread::sleep_for(chrono::seconds(1));
    }
}