//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "sfmldrawcall.h"

void MSFMLDrawCall::draw() {
    if(renderTarget && drawable) {
        renderTarget->draw(*drawable);
    }
}

MSFMLDrawCall::MSFMLDrawCall(sf::Drawable* drawable) {
    this->drawable = drawable;
}

void MSFMLDrawCall::setTarget(sf::RenderTarget *renderTarget) {
    this->renderTarget = renderTarget;
}
