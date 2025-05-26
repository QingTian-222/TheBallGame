#ifndef SFB2_H_INCLUDED
#define SFB2_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include <vector>
#include <random>

// 前置声明
extern sf::RenderWindow* mainWindow; // 声明而非定义
extern const float pi;               // 声明常量
extern const float SCALE;

// 结构体定义
struct p3 {
    int col[3];
    sf::Color toColor() { return sf::Color(col[0], col[1], col[2]); }
};

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float life;
    float lifetime;
    sf::Color color;
};

struct MergeEffect {
    std::vector<Particle> particles;
};

p3 get(p3 st,p3 ed);

void spawnMergeEffect(const sf::Vector2f& center,
                     const sf::Color& baseColor,
                     float impactSpeed);

void updateAndDrawMergeEffects(float dt);


extern std::vector<MergeEffect> mergeEffects;

#endif // SFB2_H_INCLUDED
