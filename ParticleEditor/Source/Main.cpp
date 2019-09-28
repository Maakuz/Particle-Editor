#include "Emitter.h"
#include "SFML/System.hpp"
#include "Imgui/imgui.h"
#include "Imgui/misc/cpp/imgui_stdlib.h"
#include "Imgui/SFML-imgui/imgui-SFML.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

struct PlayVariables 
{
    float lifeSpan, particleLife, spawnRate, speed;
    int initailParticles, pps, angle, cone;
    int color[4];
    int clearColor[3];
    sf::Vector2f size;
    bool gravityOn;

    friend std::istream& operator>>(std::istream& in, PlayVariables& variables)
    {
        std::string trash;
        in >> trash >> trash;
        in >> variables.initailParticles;
        in >> variables.pps;
        in >> variables.lifeSpan;
        in >> variables.particleLife;
        in >> variables.speed;
        in >> variables.spawnRate;
        in >> variables.angle;
        in >> variables.cone;
        in >> variables.size.x >> variables.size.y;
        in >> variables.color[0] >> variables.color[1] >> variables.color[2] >> variables.color[3];
        in >> variables.gravityOn;

        return in;
    };
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Pickle a particle!");

    window.setFramerateLimit(120);


    PlayVariables variables{ 30000, 3000, 50, 2, 500, 5, 0, 360, sf::Color::Green.r, sf::Color::Green.g, sf::Color::Green.b, sf::Color::Green.a, 0, 0, 0, sf::Vector2f(2, 5), false };
    sf::Color color(variables.color[0], variables.color[1], variables.color[2], variables.color[3]);
    sf::Color clearColor(variables.clearColor[0], variables.clearColor[1], variables.clearColor[2]);

    Emitter emitto(sf::Vector2f(500, 500), variables.size, color, variables.spawnRate, variables.speed, variables.particleLife, variables.lifeSpan, variables.initailParticles, variables.pps);
   
    ImGui::SFML::Init(window);

    sf::Clock clock;
    sf::Clock updateTimer;
    sf::Time deltatime;
    sf::Time particleUpdateTime;

    std::string* outFileName = new std::string("Filename");

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            ImGui::SFML::ProcessEvent(event);
        }

        deltatime = clock.restart();

        ImGui::SFML::Update(window, deltatime);

        updateTimer.restart();
        emitto.update(deltatime.asMilliseconds());
        particleUpdateTime = updateTimer.getElapsedTime();

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            emitto.setEmitterPos(sf::Vector2f(sf::Mouse::getPosition(window)));
        }

        //imgui block
        {
            ImGui::Begin("Lekrutan");
            ImGui::DragFloat("Particle lifespan", &variables.particleLife, 1, 0, INT_MAX);
            ImGui::DragFloat("Emitter lifespan", &variables.lifeSpan, 1, 0, INT_MAX);
            ImGui::DragFloat("Particle speed", &variables.speed, 0.1f, 0, INT_MAX);
            ImGui::DragFloat("Spawn rate", &variables.spawnRate, 1, 0, INT_MAX);
            ImGui::DragInt("Initial particles", &variables.initailParticles, 1, 0, INT_MAX);
            ImGui::DragInt("Particle per spawn", &variables.pps, 1, 1, INT_MAX);
            ImGui::DragFloat("Size x", &variables.size.x, 0.05f, 0, INT_MAX);
            ImGui::DragFloat("Size y", &variables.size.y, 0.05f, 0, INT_MAX);
            ImGui::DragInt("Angle", &variables.angle, 1, 0, 360);
            ImGui::DragInt("Cone", &variables.cone, 1, 1, 360);
            ImGui::DragInt4("Color", variables.color, 1, 0, 255);
            ImGui::DragInt3("Background color", variables.clearColor, 1, 0, 255);

            ImGui::Checkbox("Enable gravity", &variables.gravityOn);
           
            ImGui::Text("Microseconds to update: %d", particleUpdateTime.asMicroseconds());

            if (ImGui::Button("Reset!"))
                emitto.reset();

            static bool save = false;
            if (ImGui::Button("Save!"))
                save = true;

            if (save)
            {
                ImGui::Begin("Save me please!");

                ImGui::InputText("Filename", outFileName);

                if (ImGui::Button("Save!!"))
                {
                    std::ofstream file("../Particles/" + *outFileName + ".part");
                    if (file.is_open())
                        file << emitto;

                    file.close();

                    save = false;
                }
                ImGui::End();
            }


            static bool load = false;
            if (ImGui::Button("Load!"))
                load = true;

            if (load)
            {
                fs::path pathicle = fs::current_path();
                pathicle = pathicle.parent_path();
                pathicle /= "Particles\\";

                ImGui::Begin("Load me please!");
                for (const auto& file : fs::directory_iterator(pathicle))
                {
                    if (ImGui::Button(file.path().filename().string().c_str())) //Hm().MM
                    {
                        std::ifstream file(file.path().string());
                        if (file.is_open())
                        {
                            file >> emitto;
                            file.clear();
                            file.seekg(0, std::ios::beg);

                            file >> variables;

                            file.close();
                        }

                        emitto.reset();
                        load = false;
                    }
                }



                ImGui::End();
            }
            
            ImGui::End();
        }

        emitto.setParticleLifeSpan(variables.particleLife);
        emitto.setEmitterLifeSpan(variables.lifeSpan);
        emitto.setSpeed(variables.speed);
        emitto.setSpawnRate(variables.spawnRate);
        emitto.setInitialParticles(variables.initailParticles);
        emitto.setParticlesPerSpawn(variables.pps);
        emitto.setSize(variables.size);
        emitto.enableGravity(variables.gravityOn);
        emitto.setAngle(variables.angle);
        emitto.setConeSize(variables.cone);

        color.r = variables.color[0];
        color.g = variables.color[1];
        color.b = variables.color[2];
        color.a = variables.color[3];
        emitto.setColor(color);

        clearColor.r = variables.clearColor[0];
        clearColor.g = variables.clearColor[1];
        clearColor.b = variables.clearColor[2];

        window.clear(clearColor);
        window.draw(emitto);
        ImGui::SFML::Render(window);
        window.display();

    }

    delete outFileName;
    ImGui::SFML::Shutdown();
    return 0;
}