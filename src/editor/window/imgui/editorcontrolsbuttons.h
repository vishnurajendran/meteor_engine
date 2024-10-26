//
// Created by ssj5v on 26-10-2024.
//

#ifndef EDITORCONTROLSBUTTONS_H
#define EDITORCONTROLSBUTTONS_H
#include "../../../core/object/object.h"
#include "../../../../cmake-build-player_debug/_deps/sfml-src/include/SFML/Graphics.hpp"

class MEditorControlsButtons : MObject {
private:
    static sf::Texture playTexture;
public:
    static void playButton();
};



#endif //EDITORCONTROLSBUTTONS_H
