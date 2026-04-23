//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "windowing.h"

const int MWindow::DEFAULT_FPS = 60;
const SVector2 MWindow::DEFAULT_WINDOW_SIZE {800, 600};

void MWindow::close()
{
    coreWindow.close();
}

void MWindow::setVisible(bool visible)
{
    coreWindow.setVisible(visible);
    if (visible)
    {
        HWND hwnd = coreWindow.getNativeHandle(); // SFML lets you grab the native Win32 handle
        ShowWindow(hwnd, SW_SHOW); // Ensure it is shown
        SetForegroundWindow(hwnd); // Bring to foreground
        SetFocus(hwnd);
    }
}

bool MWindow::initialiseWindow(const SString& inTitle, SVector2 inSize, int inFps)
{
    this->name = SString("Window-") + inTitle;
    this->title = inTitle;
    this->windowSize = inSize;
    this->title = inTitle;
    this->targetFPS = inFps;

    // setup settings
    sf::ContextSettings settings;
    settings.majorVersion = 4;
    settings.minorVersion = 6;
    settings.depthBits = 24;
    this->settings = settings;

    const auto& size = sf::Vector2u(inSize.x, inSize.y);
    MLOG(STR("INIT WINDOW: ") + STR(inTitle) + " | " +
            std::to_string(size.x) + " x " + std::to_string(size.y)
            + " @ " + std::to_string(inFps) + " FPS"
            );

    auto videoMode = sf::VideoMode(size, 32);
    coreWindow.create(videoMode, this->title.str(),  sf::State::Windowed, this->settings);
    coreWindow.setFramerateLimit(this->targetFPS);

    if (!SRenderBuffer::makeFromRenderTarget(&coreWindow, renderBuffer))
    {
        MERROR(SString::format("Failed to create render buffer for window {0}", this->title));
    }

    // initialise glew.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        MERROR(STR("Failed to initialize GLEW" ));
        return false;
    }

    auto* pipelineManagerInstance = MRenderPipelineManager::getInstance();
    if (pipelineManagerInstance == nullptr) return false;

    auto* renderTarget = static_cast<sf::RenderTarget*>(&coreWindow);
    SRenderBuffer* buffer = nullptr;
    if (SRenderBuffer::makeFromRenderTarget(renderTarget, buffer))
    {
        pipelineManagerInstance->setRenderTarget(buffer);
    }
    else
        return false;

    return true;
}

bool MWindow::isOpen() const {
    return coreWindow.isOpen();
}

void MWindow::clear() {
    coreWindow.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MWindow::update(float deltaTime) {

    // no fps value, make the window not update
    if(targetFPS <= 0)
    {
        MERROR("FPS must be greater than zero");
        return;
    }

    const sf::Time frameTime = sf::seconds(1.f / targetFPS);
    std::optional<sf::Event> event;
    while (event = coreWindow.pollEvent()) {
        if (event.has_value() && event->is<sf::Event::Closed>()) {
            close();
            return;
        }
    }

    coreWindow.clear();

    auto* pipelineManagerInstance = MRenderPipelineManager::getInstance();
    if (pipelineManagerInstance != nullptr)
    {
        pipelineManagerInstance->preRender();
        pipelineManagerInstance->render();
        pipelineManagerInstance->postRender();
    }

    coreWindow.display();

    sf::Time elapsed = clock.getElapsedTime();
    sf::Time sleepTime = frameTime - elapsed;
    if (sleepTime > sf::Time::Zero) {
        sf::sleep(sleepTime);  // Sleep to limit the framerate
    }
    clock.restart();
}
