#pragma once
#include "SFML/Graphics.hpp"

struct Particle
{
    sf::Vector2f velocity;
    sf::Color color;

    float lifespan;
};