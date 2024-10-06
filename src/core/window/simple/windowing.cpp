//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "windowing.h"

MWindow::MWindow(const SString& title) : MWindow(title, 800, 600, 0) {

}

MWindow::MWindow(const SString& title, int sizeX, int sizeY, int fps) : MObject() {
    this->title =  title;
    this->targetFPS = fps;
    name = STR("Window - ") + title;
    coreWindow.create(sf::VideoMode(sizeX, sizeY), title.str(), sf::Style::Default);
    coreWindow.setFramerateLimit(fps);

    // initialise glew.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        MERROR(STR("Failed to initialize GLEW" ));
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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
}

void MWindow::update() {
    const sf::Time frameTime = sf::seconds(1.f / targetFPS);
    while (coreWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            close();
            return;
        }
    }

    coreWindow.clear();
    MGraphicsRenderer::draw();
    coreWindow.display();

    sf::Time elapsed = clock.getElapsedTime();
    sf::Time sleepTime = frameTime - elapsed;
    if (sleepTime > sf::Time::Zero) {
        sf::sleep(sleepTime);  // Sleep to limit the framerate
    }
    clock.restart();
}
