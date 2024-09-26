//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_SFMLDRAWCALL_H
#define METEOR_ENGINE_SFMLDRAWCALL_H
#include "core/meteor_core_minimal.h"
#include "drawcall.h"

class MSFMLDrawCall : public MDrawCall {
private:
    sf::Drawable* drawable;
public:
    MSFMLDrawCall(sf::Drawable* drawable);
    void draw(sf::RenderTarget *target) override;
};


#endif //METEOR_ENGINE_SFMLDRAWCALL_H
