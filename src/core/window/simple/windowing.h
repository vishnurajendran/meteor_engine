//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_WINDOWING_H
#define METEOR_ENGINE_WINDOWING_H

#include "core/meteor_core.h"
#include "SFML/Graphics.hpp"

class MWindow : public MObject {
protected:
    sf::ContextSettings settings;
    SString title;
    sf::Event event;
    sf::RenderWindow coreWindow;
    sf::Clock deltaClock;
public:
    MWindow(const SString& title);
    MWindow(const SString& title, int sizeX, int sizeY, int fps);
    virtual void clear();
    virtual void update();
    virtual bool isOpen() const;
    virtual void close();
};

#endif //METEOR_ENGINE_WINDOWING_H
