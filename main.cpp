#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/Graphics/RenderTexture.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

int main() {
    sf::RenderWindow window(sf::VideoMode(3024,1964), "ImGui + SFML = <3", sf::Style::Default);
    window.setFramerateLimit(0);
    ImGui::SFML::Init(window);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 2.0f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    sf::RenderTexture rt;
    if(!rt.create(1920, 1080)){
        return EXIT_FAILURE;
    }

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Image(rt);
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();
        rt.clear();
        rt.draw(shape);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}