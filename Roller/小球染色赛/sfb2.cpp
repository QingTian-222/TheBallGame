#include "sfb2.h"
#include <cmath>

sf::RenderWindow* mainWindow = nullptr;
const float pi = std::acos(-1.0f);


std::vector<MergeEffect> mergeEffects;

p3 get(p3 st,p3 ed){//插值
    p3 res;
    for(int i=0;i<3;i++)
        res.col[i] = (ed.col[i]-st.col[i]+1)/7+st.col[i];
    return res;
}

//生成粒子
void spawnMergeEffect(const sf::Vector2f& center,
                      const sf::Color&   baseColor,
                      float impactSpeed)
{
    MergeEffect ef;
    std::mt19937_64 rng(1);
    std::uniform_real_distribution<float> angleDist(0, 2*pi);
    std::uniform_real_distribution<float> lifeDist(0.3f, 0.8f);

    float baseRadius = impactSpeed * 0.05f;

    int count = 20;
    ef.particles.reserve(count);
    for (int i = 0; i < count; ++i) {
        float a  = angleDist(rng);
        float lt = lifeDist(rng);

        Particle p;
        p.pos      = center;
        float randRadius = baseRadius * (0.5f + 0.5f * std::cos(i));
        p.pos.x   += std::cos(a) * randRadius;
        p.pos.y   += std::sin(a) * randRadius;
        float speedFactor = 0.5f + 0.5f * (impactSpeed / 200.f);
        p.vel      = { std::cos(a)*impactSpeed*speedFactor*0.5f,
                       std::sin(a)*impactSpeed*speedFactor*0.5f };
        p.life     = lt;
        p.lifetime = lt;
        p.color    = baseColor;
        ef.particles.push_back(p);
    }

    mergeEffects.push_back(std::move(ef));
}
//更新粒子画面
void updateAndDrawMergeEffects(float dt) {
    for (auto it = mergeEffects.begin(); it != mergeEffects.end(); ) {
        bool aliveAny = false;
        for (auto& p : it->particles) {
            if (p.life <= 0) continue;
            p.life -= dt;// 更新
            p.pos += p.vel * dt;
            p.vel *= 0.95f;// 空气阻力衰减速度（可选）
            aliveAny = true;
        }

        // 绘制所有还活着的粒子
        for (auto& p : it->particles) {
            if (p.life <= 0) continue;
            float t = p.life / p.lifetime;  // 1→0
            sf::Uint8 alpha = static_cast<sf::Uint8>(255 * t);
            sf::CircleShape dot(3);
            dot.setOrigin(3,3);
            dot.setPosition(p.pos);
            dot.setFillColor({ p.color.r, p.color.g, p.color.b, alpha });
            mainWindow->draw(dot);
        }
        if (!aliveAny) {// 如果整组粒子都死亡，就移除该 effect
            it = mergeEffects.erase(it);
        } else {
            ++it;
        }
    }
}
