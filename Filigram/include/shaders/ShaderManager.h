#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <SFML/Graphics/Texture.hpp>

class ShaderManager {
public:
    ShaderManager(const std::string& fragmentShaderSource) {
        if (!shader.loadFromMemory(fragmentShaderSource, sf::Shader::Type::Fragment)) {
            throw std::runtime_error("Error loading shader");
        }
        
        texture.resize({ 800, 600 }, true);
        shader.setUniform("resolution", sf::Glsl::Vec2(800, 600));
        sprite = std::make_shared<sf::Sprite> (texture);
    }

    void handleResize(int width, int height, sf::RenderWindow& window) {
        if (!isActive)return;
        if (width <= 0 || height <=0)
        {
            return;
        }
        texture.resize(sf::Vector2u( width, height ), true);
        texture.update(window);
        sprite->setTexture(texture);
        shader.setUniform("resolution", sf::Glsl::Vec2(width, height));
    }

    void update(float time) {
        if (!isActive)return;
        shaderTime += time;
        shader.setUniform("time", shaderTime);
        shader.setUniform("stoppingTime", stopingTime);
        if (stoping)
        {
            stopingTime += time*10.f;
            if (stopingTime >=1.f)
            {
                stopingTime = 1.f;
                stoping = false;
                isActive = false;
            }
        }
        if (starting)
        {
            stopingTime -= time * 10.f;
            if (stopingTime <= 0)
            {
                stopingTime = 0.f;
                starting = false;
            }
        }
        
    }

    void draw(sf::RenderWindow& window) {
        if (!isActive)return;
        
        window.draw(*sprite, &shader);
    }
    void start()
    {
        isActive = true;
        starting = true;
        stoping = false;
        

    }
    void stop()
    {
        if (isActive)
        {
            starting = false;
            stoping = true;
        }

    }
    void cleanup() {
        texture = sf::Texture();
    }
private:
    bool isActive = false;
    bool stoping = false;
    bool starting;
    float shaderTime{0};
    float stopingTime{1.f};
    sf::Shader shader;
    sf::Texture texture;
    std::shared_ptr<sf::Sprite> sprite;
};