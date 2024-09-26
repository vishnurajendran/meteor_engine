//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_COLOR_H
#define METEOR_ENGINE_COLOR_H


struct SColor {
public:
    float r;
    float g;
    float b;
    float a;
    SColor(float red, float green,float blue, float alpha);

    static SColor red();
    static SColor blue();
    static SColor green();
    static SColor yellow();
    static SColor skyBlue();
};


#endif //METEOR_ENGINE_COLOR_H
