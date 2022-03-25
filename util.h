#pragma once

#include <string>
#include <vector>
#include <sstream>

#include <json/value.h>
#include <json/json.h>

#include <fstream>

#include "colour.h"
#include "globals.h"

using namespace std;

Json::Value openJSON(string path) {

    Json::Reader reader;
    Json::Value cfg_root;
    std::ifstream cfgfile(path.c_str());
    cfgfile >> cfg_root;

    return cfg_root;
}

vector<string> split (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

string execCommand(const string cmd) {
    auto pPipe = popen(cmd.c_str(), "r");
    if(pPipe == nullptr) {
        throw runtime_error("Cannot open pipe");
    }

    array<char, 256> buffer;

    string result;

    while(!feof(pPipe)) {
        auto bytes = fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    pclose(pPipe);

    return result;
}

bool virtualMachineOn() {
    return (execCommand("echo \"" + password + "\" | sudo -S virsh list --all | grep \" win10 \" | awk '{{ print $3}}'")[0] == 'r');
}