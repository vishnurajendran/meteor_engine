//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "sfmldrawcall.h"

void MSFMLDrawCall::draw(sf::RenderTarget *target) {
    if(target && drawable) {
        target->draw(*drawable);
    }
}

MSFMLDrawCall::MSFMLDrawCall(sf::Drawable* drawable) {
    this->drawable = drawable;
}
