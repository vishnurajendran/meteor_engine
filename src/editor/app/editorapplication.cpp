//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

#include "core/graphics/core/render_queue.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/editorwindows/assetwindow/editorassetwindow.h"
#include "editor/window/imgui/imguiwindow.h"
#include "editor/window/menubar/menubartree.h"


MSpatialEntity* MEditorApplication::Selected = nullptr;
MEditorApplication* MEditorApplication::editorInst = nullptr;


MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}

void MEditorApplication::run() {
    if(window == nullptr)
        return;

    window->clear();

    //Todo: set real delta time here
    MSceneManager::getSceneManagerInstance()->update(0.0);
    window->update();
}

void MEditorApplication::cleanup() {
    MLOG(STR("Cleanup..."));

    MLOG(STR("Deleting SceneManager"));
    delete sceneManagerRef;

    MLOG(STR("Closing Window"));
    if (window != nullptr)
        window->close();
    window = nullptr;
    MLOG(STR("Editor Application Cleanup Complete"));
}

void MEditorApplication::initialise() {

    std::thread splashThread(&MEditorApplication::showSplashScreen, this);
    editorInst = this;
    //appRunning = true;
    MLOG(STR("Initialising Editor"));
    window = new MImGuiWindow(STR("Meteorite Editor"));
    if(window == nullptr){
        MERROR(STR("Invalid Window Type"));
        return;
    }
    if(!window->isOpen()) {
        MERROR(STR("Failed to open window"));
        return;
    }
    window->setVisible(false);
    loadPrerequisites();

    // Load first, so that it can log everything from here on out.
    subWindows.push_back(new MEditorConsoleWindow());
    subWindows.push_back(new MEditorHierarchyWindow());
    subWindows.push_back(new MEditorInspectorWindow());
    subWindows.push_back(new MEditorSceneViewWindow());
    subWindows.push_back(new MEditorAssetWindow());
    MLOG(STR("Loaded Editor Windows"));

    MMenubarTreeNode::buildTree();
    MLOG(STR("Built Menubar Items"));

    window->setVisible(true);
    splashShowing = false;
    splashThread.join();
}

bool MEditorApplication::isRunning() const
{
    if (window == nullptr)
        return false;

    return window->isOpen();
}

void MEditorApplication::showSplashScreen()
{
    sf::RenderWindow splashWindow(sf::VideoMode(960, 540), "Meteorite-Splash", sf::Style::None);
    splashWindow.setFramerateLimit(60);
    splashWindow.setActive(true);
    splashWindow.setVisible(true);
    splashWindow.requestFocus();

    //Hack to force to top
    HWND hwnd = splashWindow.getSystemHandle();  // SFML lets you grab the native Win32 handle
    ShowWindow(hwnd, SW_SHOW);               // Ensure it is shown
    SetForegroundWindow(hwnd);               // Bring to foreground
    SetFocus(hwnd);


    sf::Texture splashTexture;
    splashTexture.loadFromFile("meteor_assets/splash.png");

    sf::Sprite sprite(splashTexture);
    sprite.setScale(splashWindow.getSize().x / (float)splashTexture.getSize().x,
                    splashWindow.getSize().y / (float)splashTexture.getSize().y);

    sf::Font font;
    font.loadFromFile("meteor_assets/fonts/open-sans/OpenSans-Regular.ttf");
    sf::Text text;
    text.setFont(font);
    text.setString("Loading Meteorite...");
    text.setCharacterSize(12); // in pixels
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(30, 500));

    while (splashShowing && splashWindow.isOpen())
    {
        sf::Event event;
        while (splashWindow.pollEvent(event))
        {
            continue;
        }

        text.setString(MLogger::getLastMessage().c_str());

        splashWindow.clear();
        splashWindow.draw(sprite);
        splashWindow.draw(text);
        splashWindow.display();
    }


    splashWindow.close();
}

void MEditorApplication::loadPrerequisites()
{
    //Refresh Asset Manager
    MAssetManager::registerAssetManagerInstance(new MEditorAssetManager());
    MAssetManager::getInstance()->refresh();

    //setup scene manager
    sceneManagerRef = new MEditorSceneManager();
    MSceneManager::registerSceneManager(sceneManagerRef);
}

void MEditorApplication::exit()
{
    if (editorInst != nullptr)
        editorInst->window->close();
}

MCameraEntity* MEditorApplication::getSceneCamera()
{
    if (editorInst == nullptr)
        return nullptr;

    return editorInst->sceneManagerRef->getEditorSceneCamera();
}

#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif
