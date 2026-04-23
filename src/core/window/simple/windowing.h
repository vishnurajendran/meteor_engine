//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_WINDOWING_H
#define METEOR_ENGINE_WINDOWING_H

#include "core/meteor_core.h"
#include "SFML/Graphics.hpp"

class MWindow : public MObject {
public:
    static const int DEFAULT_FPS;
    static const SVector2 DEFAULT_WINDOW_SIZE;

protected:
    int targetFPS = 0;
    sf::Clock clock;
    sf::ContextSettings settings;
    SString title = "Window Instance";
    sf::RenderWindow coreWindow;
    sf::Clock deltaClock;
    SVector2 windowSize;

private:
    SRenderBuffer* renderBuffer = nullptr;
public:
    MWindow() = default;
    virtual ~MWindow() { delete renderBuffer; }

    virtual bool initialiseWindow(const SString& inTitle, SVector2 inSize, int inFps);
    virtual void clear();
    virtual void update(float deltaTime);
    virtual bool isOpen() const;
    virtual void close();

    void setVisible(bool visible);
    SRenderBuffer* getRenderBuffer() { return renderBuffer; }
};

#endif //METEOR_ENGINE_WINDOWING_H
