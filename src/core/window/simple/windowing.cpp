//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "windowing.h"

MWindow::MWindow(const SString& title) : MWindow(title, 800, 600, 60) {

}

MWindow::MWindow(const SString& title, int sizeX, int sizeY, int fps) : MObject() {
    this->title =  title;
    this->targetFPS = fps;
    name = STR("Window - ") + title;

    sf::ContextSettings settings;
    settings.majorVersion = 4;
    settings.minorVersion = 6;
    settings.depthBits = 24;

    coreWindow.create(sf::VideoMode(sizeX, sizeY), title.str(), sf::Style::Default, settings);
    coreWindow.setFramerateLimit(fps);

    // initialise glew.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        MERROR(STR("Failed to initialize GLEW" ));
        return;
    }
    MGraphicsRenderer::initialise(&coreWindow);
}

void MWindow::close() {
    coreWindow.close();
}

bool MWindow::isOpen() const {
    return coreWindow.isOpen();
}

void MWindow::clear() {
    coreWindow.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MWindow::update() {
    if(targetFPS <= 0) //just a safety check
        targetFPS = 60;

    const sf::Time frameTime = sf::seconds(1.f / targetFPS);
    while (coreWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            close();
            return;
        }
    }

    coreWindow.clear();
    MGraphicsRenderer::prepare();
    MGraphicsRenderer::draw();
    coreWindow.display();

    sf::Time elapsed = clock.getElapsedTime();
    sf::Time sleepTime = frameTime - elapsed;
    if (sleepTime > sf::Time::Zero) {
        sf::sleep(sleepTime);  // Sleep to limit the framerate
    }
    clock.restart();
}
