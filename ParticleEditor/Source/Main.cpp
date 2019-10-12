#include "Emitter.h"
#include "SFML/System.hpp"
#include "Imgui/imgui.h"
#include "Imgui/misc/cpp/imgui_stdlib.h"
#include "Imgui/SFML-imgui/imgui-SFML.h"
#include "LightQueue.h"
#include "ShaderHandler.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
void restart(Emitter* emitter);
sf::VertexArray generateGrid(sf::Color color, float size = 32);

struct PlayVariables 
{
    float lifeSpan, particleLife, spawnRate, speed;
    int initailParticles, pps, angle, cone;
    int color[4];
    int colorDev[4];
    int clearColor[3];
    sf::Vector2f size;
    bool gravityOn;
    float jitter;
    float friction;
    float particleLightRadius;

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
        in >> variables.colorDev[0] >> variables.colorDev[1] >> variables.colorDev[2] >> variables.colorDev[3];
        in >> variables.gravityOn;
        in >> variables.jitter;
        in >> variables.friction;
        in >> trash;
        in >> variables.particleLightRadius;
        return in;
    };
};

PlayVariables variables; 

const int WIDTH = 1920;
const int HEIGHT = 1080;
int main()
{
    int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
    _CrtSetDbgFlag(flag);
    //_CrtSetBreakAlloc(689); // Comment or un-comment on need basis

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Pickle a particle!");

    window.setFramerateLimit(120);
    sf::RectangleShape fullscreenboi(sf::Vector2f(WIDTH, HEIGHT));
    ShaderHandler shaders;
    sf::RenderTexture renderTargets[3];
    sf::View view;

    sf::Color gridColor(sf::Color::Magenta);
    sf::VertexArray grid = generateGrid(gridColor);

    bool repeating = false;
    bool lightOn = false;
    float zoomLevel = 1;

    for (int i = 0; i < 3; i++)
    {
        renderTargets[i].create(1920, 1080);
    }

    Emitter emitto;

    restart(&emitto);

    sf::Color color(variables.color[0], variables.color[1], variables.color[2], variables.color[3]);
    sf::Color clearColor(variables.clearColor[0], variables.clearColor[1], variables.clearColor[2]);

   
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



        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            emitto.setEmitterPos(sf::Vector2f(sf::Mouse::getPosition(window)) / zoomLevel);
        }

        //imgui block
        {
            ImGui::Begin("Lekrutan");
            ImGui::BeginTabBar("Lekrutan");
            if (ImGui::BeginTabItem("Particles"))
            {
                ImGui::DragFloat("Particle lifespan", &variables.particleLife, 1, 0, INT_MAX);
                ImGui::DragFloat("Emitter lifespan", &variables.lifeSpan, 1, 0, INT_MAX);
                ImGui::DragFloat("Particle speed", &variables.speed, 0.01f, 0, INT_MAX);
                ImGui::DragFloat("Spawn rate", &variables.spawnRate, 1, 0, INT_MAX);
                ImGui::DragInt("Initial particles", &variables.initailParticles, 1, 0, INT_MAX);
                ImGui::DragInt("Particle per spawn", &variables.pps, 1, 1, INT_MAX);
                ImGui::DragFloat("Size x", &variables.size.x, 0.05f, 0, INT_MAX);
                ImGui::DragFloat("Size y", &variables.size.y, 0.05f, 0, INT_MAX);
                ImGui::DragInt("Angle", &variables.angle, 1, 0, 360);
                ImGui::DragInt("Cone", &variables.cone, 1, 1, 360);
                ImGui::DragInt4("Color", variables.color, 1, 0, 255);
                ImGui::DragInt4("Color Deviation", variables.colorDev, 1, 0, 255);
                ImGui::DragFloat("Friction", &variables.friction, 0.001, 0, 2);
                ImGui::DragFloat("Jitter", &variables.jitter, 0.001, 0, 1);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lighting"))
            {
                if (ImGui::Button("Add light"))
                {
                    emitto.addLight(sf::Vector2f(0, 0), 100, sf::Vector3f(1, 1, 1));
                }

                ImGui::DragFloat("Particle light radius", &variables.particleLightRadius, 1, 0, 200);
                ImGui::Separator();
                ImGui::BeginChild("LightList", sf::Vector2i(0, -200));

                for (size_t i = 0; i < emitto.getLights()->size(); i++)
                { 
                    std::string collabel = "color " + std::to_string(i);
                    std::string poslabel = "pos " + std::to_string(i);
                    std::string radiuslabel = "radius " + std::to_string(i);

                    Emitter::EmitterLight* light = &emitto.getLights()->at(i);
                    
                    float col[3] = { light->initialColor.x, light->initialColor.y, light->initialColor.z };
                    float pos[2] = { light->offset.x, light->offset.y};
                    
                    ImGui::DragFloat(radiuslabel.c_str(), &light->light->radius, 1, 0, 3000);
                    if (ImGui::ColorEdit3(collabel.c_str(), col))
                    {
                        light->light->color.x = col[0];
                        light->light->color.y = col[1];
                        light->light->color.z = col[2];
                        light->initialColor.x = col[0];
                        light->initialColor.y = col[1];
                        light->initialColor.z = col[2];
                    }

                    ImGui::DragFloat2(poslabel.c_str(), pos, 1, -1000, 1000);
                    ImGui::Separator();

                   
                    light->offset.x = pos[0];
                    light->offset.y = pos[1];
                    light->light->pos = emitto.getEmitterPos() + light->offset;
                }


                ImGui::EndChild();
                ImGui::Separator();


                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Editor"))
            {
                float col[3] = { gridColor.r, gridColor.g, gridColor.b };

                if (ImGui::DragFloat3("grid color", col))
                {
                    gridColor = sf::Color(col[0], col[1], col[2]);
                    grid = generateGrid(gridColor);
                }

                if (ImGui::DragFloat("Zooooom", &zoomLevel, 1, 1, 8))
                {
                    view.setSize(WIDTH / zoomLevel, HEIGHT / zoomLevel);
                    view.setCenter(WIDTH / zoomLevel / 2.f, HEIGHT / zoomLevel / 2.f);
                    window.setView(view);
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

            ImGui::DragInt3("Background color", variables.clearColor, 1, 0, 255);
            ImGui::Checkbox("Enable gravity", &variables.gravityOn);
            ImGui::SameLine();
            ImGui::Checkbox("Enable light", &lightOn);
            ImGui::SameLine();
            ImGui::Checkbox("Repeating", &repeating);
            ImGui::SameLine();
            if (ImGui::Button("ENABLE PARTICLELIGHT?!"))
                emitto.enableParticleLight();

            ImGui::Text("Microseconds to update: %d", particleUpdateTime.asMicroseconds());

            if (ImGui::Button("Reset!"))
                emitto.reset();

            ImGui::SameLine();
            static bool save = false;
            if (ImGui::Button("Save!"))
                save = true;

            ImGui::SameLine();
            static bool load = false;
            if (ImGui::Button("Load!"))
                load = true;

            ImGui::SameLine();
            if (ImGui::Button("New!"))
            {
                restart(&emitto);
            }


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

        if (repeating)
            if (emitto.isVeryDead())
                emitto.reset();

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
        emitto.setFriction(variables.friction);
        emitto.setJitter(variables.jitter);
        emitto.setParticleLightRadius(variables.particleLightRadius);

        updateTimer.restart();
        emitto.update(deltatime.asMilliseconds());
        particleUpdateTime = updateTimer.getElapsedTime();

        color.r = variables.color[0];
        color.g = variables.color[1];
        color.b = variables.color[2];
        color.a = variables.color[3];
        emitto.setColor(color);

        color.r = variables.colorDev[0];
        color.g = variables.colorDev[1];
        color.b = variables.colorDev[2];
        color.a = variables.colorDev[3];
        emitto.setColorDeviation(color);

        clearColor.r = variables.clearColor[0];
        clearColor.g = variables.clearColor[1];
        clearColor.b = variables.clearColor[2];

        window.clear(clearColor);

        window.draw(grid);
        renderTargets[0].clear(sf::Color::Transparent);
        for (size_t i = 0; i < LightQueue::get().getQueue().size(); i++)
        {
            Light* light = LightQueue::get().getQueue()[i];
            ShaderHandler::getShader(SHADER::lighting).setUniform("pos", light->pos);
            ShaderHandler::getShader(SHADER::lighting).setUniform("radius", light->radius);
            ShaderHandler::getShader(SHADER::lighting).setUniform("color", light->color);

            sf::RenderStates state;
            state.shader = &ShaderHandler::getShader(SHADER::lighting);
            state.blendMode = sf::BlendAdd;

            sf::RectangleShape sprite(sf::Vector2f(light->radius * 2, light->radius * 2));
            sprite.setPosition(light->pos - (sf::Vector2f(sprite.getSize() / 2.f)));
            renderTargets[0].draw(sprite, state);
        }
        renderTargets[0].display();

        fullscreenboi.setTexture(&renderTargets[0].getTexture());
       
        LightQueue::get().clear();

        for (int i = 0; i < 2; i++)
        {
            renderTargets[i + 1].clear(sf::Color::Transparent);
            renderTargets[i + 1].draw(fullscreenboi, &shaders[shaders.BLUR_PASS[i]]);
            renderTargets[i + 1].display();

            fullscreenboi.setTexture(&renderTargets[i + 1].getTexture());
        }

        window.draw(emitto);
        if (lightOn)
            window.draw(fullscreenboi, sf::BlendMultiply);

        ImGui::SFML::Render(window);
        window.display();

    }
    delete outFileName;
    ImGui::SFML::Shutdown();
    return 0;
}

void restart(Emitter* emitter)
{
    variables = { 30000, 3000, 50, 2, 500, 5, 0, 360,
        sf::Color::Green.r, sf::Color::Green.g, sf::Color::Green.b, sf::Color::Green.a,
        0, 0, 0, 0,
        0, 0, 0, sf::Vector2f(2, 5), false, 0, 1, 50 };

    sf::Color color(variables.color[0], variables.color[1], variables.color[2], variables.color[3]);


    *emitter = Emitter(sf::Vector2f(500, 500), variables.size, color, variables.spawnRate, variables.speed, variables.particleLife, variables.lifeSpan, variables.initailParticles, variables.pps);
    
}

sf::VertexArray generateGrid(sf::Color color, float size)
{
    sf::VertexArray grid(sf::PrimitiveType::Lines);

    for (int i = 0; i < WIDTH / size; i++)
    {
        sf::Vertex v(sf::Vector2f(i * size, 0), color);
        grid.append(v);
        v.position.y = HEIGHT;
        grid.append(v);
    }

    for (int i = 0; i < HEIGHT / size; i++)
    {
        sf::Vertex v(sf::Vector2f(0, i * size), color);
        grid.append(v);
        v.position.x = WIDTH;
        grid.append(v);
    }

    return grid;
}