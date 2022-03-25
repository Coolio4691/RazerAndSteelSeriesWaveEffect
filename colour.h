#pragma once

#include <string>
#include <vector>
#include <math.h>

using namespace std;

struct RGB {

    float red;
    float green;
    float blue;
};

struct RGrg {
    float Red;
    float Green;
    float red;
    float green;
};

string RGBtoHEX(int r, int g, int b, bool with_head) { 
    char hexcol[16];

    snprintf(hexcol, sizeof(hexcol), "%02x%02x%02x", r, g, b);

    return string(hexcol);
}

vector<float> RGBtoHSV(float r, float g, float b) {
    float maxc = max(r, g);
    maxc = max(maxc, b);
    float minc = min(r, g);
    minc = min(minc, b);

    float v = maxc;
    if(minc == maxc)
        return {0, 0, v};
    float s = (maxc-minc) / maxc;
    float rc = (maxc-r) / (maxc-minc);
    float gc = (maxc-g) / (maxc-minc);
    float bc = (maxc-b) / (maxc-minc);
    float h = 4+gc-rc;

    if(r == maxc)
        h = bc-gc;
    else if(g == maxc)
        h = 2+rc-bc;
    

    h = fmod((h/6), 1);
    return { h, s, v };
}

RGrg HSVtoRGB(float h, float s, float v) {
    if(s == 0.0)
        return RGrg{v*256, v*256, v, v};
    int i = int(h*6.0);
    float f = (h*6.0) - i;
    float p = v*(1.0 - s);
    float q = v*(1.0 - s*f);
    float t = v*(1.0 - s*(1.0-f));

    float np = p * 256;
    float nq = q * 256;
    float nt = t * 256;
    float nv = v * 256;

    i %= 6;
    if(i == 0)
        return { nv, nt, v, t };
    
    if(i == 1)
        return { nq, nv, q, v };
    
    if(i == 2)
        return { np, nv, p, v };
    
    if(i == 3)
        return { np, nq, p, q };
    
    if(i == 4)
        return { nt, np, t, p };
    
    return { nv, np, v, p };
}

string HSVtoHEX(float h, float s, float v) {
    RGrg rgrg = HSVtoRGB(h, s, v);

    return RGBtoHEX(rgrg.Red, rgrg.Green, 102, true);
}
