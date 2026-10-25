//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_WINDOWING_H
#define METEOR_ENGINE_WINDOWING_H


#include "SFML/Graphics.hpp"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/object/object.h"
#include "core/utils/delegate.h"
#include "core/utils/glmhelper.h"

class MWindow : public MObject {
    DEFINE_OBJECT_SUBCLASS(MWindow)
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
    bool fullscreen = false;

private:
    SRenderBuffer* renderBuffer = nullptr;

protected:
    void handleWindowEvents(const std::optional<sf::Event>& event);

public:
    MWindow() = default;
    virtual ~MWindow() { delete renderBuffer; }

    // Call before initialiseWindow(). Fullscreen uses the requested size if
    // the display supports it as a fullscreen mode, else the desktop mode.
    void setFullscreen(bool inFullscreen) { fullscreen = inFullscreen; }

    virtual bool initialiseWindow(const SString& inTitle, SVector2 inSize, int inFps);
    virtual void clear();
    virtual void update(float deltaTime);
    virtual bool isOpen() const;
    [[nodiscard]] bool hasFocus() const { return coreWindow.hasFocus(); }
    virtual void close();

    // Fired with the new size when the user resizes the window.
    TMulticastDelegate<SVector2> onResized;
    void setVisible(bool visible);
    SRenderBuffer* getRenderBuffer() { return renderBuffer; }
};

#endif //METEOR_ENGINE_WINDOWING_H
