//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#define METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#include "editor/meteorite_minimal.h"

class MEditorSceneViewWindow : public MImGuiSubWindow{
private:
    sf::RenderTexture renderTexture;
    sf::ContextSettings settings;
public:
    MEditorSceneViewWindow();
    MEditorSceneViewWindow(int x, int y);
    void onGui() override;
};


#endif //METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
