//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "color.h"
SColor::SColor(float red, float green, float blue, float alpha) {
    r = red;
    g = green;
    b = blue;
    a = alpha;
}

SColor SColor::red() {
    return SColor(1,0,0,1);
}

SColor SColor::blue() {
    return SColor(0,0,1,1);
}

SColor SColor::green() {
    return SColor(0,1,0,1);
}

SColor SColor::skyBlue() {
    return SColor(0.52f,0.80f,1,1);
}

SColor SColor::yellow() {
    return SColor(1, 1, 0, 1);
}
