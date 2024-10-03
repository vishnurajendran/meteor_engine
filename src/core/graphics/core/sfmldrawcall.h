//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_SFMLDRAWCALL_H
#define METEOR_ENGINE_SFMLDRAWCALL_H
#include "drawcall.h"

class MSFMLDrawCall : public MDrawCall {
private:
    sf::Drawable* drawable;
    sf::RenderTarget* renderTarget = nullptr;
public:
    MSFMLDrawCall(sf::Drawable* drawable);
    void setTarget(sf::RenderTarget* renderTarget);
    void draw() override;
};


#endif //METEOR_ENGINE_SFMLDRAWCALL_H
