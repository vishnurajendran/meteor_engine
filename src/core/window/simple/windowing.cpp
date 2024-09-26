//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "windowing.h"

MWindow::MWindow(const SString& title) : MWindow(title, 800, 600, 0) {

}

MWindow::MWindow(const SString& title, int sizeX, int sizeY, int fps) : MObject() {
    this->title =  title;
    name = TEXT("Window - ") + title;
    coreWindow.create(sf::VideoMode(sizeX, sizeY), title.str(), sf::Style::Default);
    coreWindow.setFramerateLimit(fps);

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
    while (coreWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            close();
        }
    }

    coreWindow.clear();
    MGraphicsRenderer::draw();
    coreWindow.display();
}
