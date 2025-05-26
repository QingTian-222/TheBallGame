#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include <bits/stdc++.h>
#include "sfb2.h"


using namespace sf;

const float DEG  =  57.29577f;
const float SCALE = 30.f;
b2Vec2 Gravity(0.0f, 0);
b2World World(Gravity);

std::unordered_map<b2Body*,int> mp;//userdata
std::unordered_map<b2Body*, std::deque<sf::Vector2f>> trails;
const int MAX_TRAIL_LENGTH = 7;
const int Groups = 4;
int num[Groups];
int all_cnt=0;


std::unordered_map<b2Body*,p3> mp_c,mp_cn;

class MyContactListener : public b2ContactListener {
public:
    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override {

        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();


        b2Body* b1 = fixtureA->GetBody();
        b2Body* b2 = fixtureB->GetBody();
        int m1=mp[b1],m2=mp[b2];
        if(m1<0 || m2<0) return;


        if(m1==m2){//取消碰撞
            //contact->SetEnabled(false);
            return ;
        }
        float v1=b1->GetLinearVelocity().Length();
        float v2=b2->GetLinearVelocity().Length();

        bool didMerge = 0;
        b2Body* mergedBody = nullptr;

        if(v1>2*v2) {
            mp[b2]=m1;
            num[m2]--,num[m1]++;
            mp_c[b2]=mp_c[b1];
            mp_cn[b2]={255,255,255};
            didMerge = 1; mergedBody = b2;
        }else if(v2>2*v1){
            mp[b1]=m2;
            num[m1]--,num[m2]++;
            mp_c[b1]=mp_c[b2];
            mp_cn[b2]={255,255,255};
            didMerge = 1; mergedBody = b1;
        }

        if (didMerge) {
            b2WorldManifold wm;
            contact->GetWorldManifold(&wm);
            sf::Vector2f center{ wm.points[0].x*SCALE, wm.points[0].y*SCALE };
            spawnMergeEffect(center, mp_c[mergedBody].toColor() ,std::max(v1, v2) * SCALE);
        }

    }
}contactListener;



sf::Color getColor(b2Body* bd){
    sf::Color color=sf::Color::Red;
    if(mp[bd]<0) color = sf::Color(220,220,220);

    else{
        p3 tp=get(mp_cn[bd],mp_c[bd]);
        mp_cn[bd]=tp;
        color = tp.toColor();

    }

    return color;
}

void drawbody(b2Body* body) {
    if (!body) return;

    b2Vec2 position = body->GetPosition();
    float angle = body->GetAngle();
    sf::Color color = getColor(body);

    // 高速光环渲染
    sf::Vector2f pos(position.x * SCALE, position.y * SCALE);
    float speed = body->GetLinearVelocity().Length();
    float normalizedSpeed = std::min(1.0f, speed / 35.0f);

    if (speed > 25.0f && mp[body] >= 0) {
        float haloSize = 25 * (normalizedSpeed * 1.5f);
        for(int i = 3; i >= 1; i--) {
            float radius = haloSize * (0.3f + 0.2f * i);
            sf::CircleShape halo(radius);
            halo.setOrigin(radius, radius);
            halo.setPosition(pos);
            halo.setFillColor(sf::Color(255, 255, 255, 255 / i));
            mainWindow->draw(halo);
        }
    }

    // 拖尾渲染
    if (mp[body] >= 0) {
        auto& trail = trails[body];
        trail.push_back(pos);
        if (trail.size() > MAX_TRAIL_LENGTH) trail.pop_front();
    }
    if (mp[body] >= 0 && trails.count(body)) {
        const auto& trail = trails[body];
        for (size_t i = 0; i < trail.size(); ++i) {
            float alpha = 255.f * i / trail.size() * 0.6f;
            sf::CircleShape tail(18);
            tail.setOrigin(18, 18);
            tail.setPosition(trail[i]);
            tail.setFillColor(sf::Color(color.r, color.g, color.b, static_cast<sf::Uint8>(alpha)));
            mainWindow->draw(tail);
        }
    }

    // 原始渲染
    for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
        b2Shape* shape = fixture->GetShape();

        switch (shape->GetType()) {
            case b2Shape::e_circle: {
                b2CircleShape* circleShape = static_cast<b2CircleShape*>(shape);
                sf::CircleShape circle(circleShape->m_radius * SCALE);
                circle.setOrigin(circleShape->m_radius * SCALE, circleShape->m_radius * SCALE);
                b2Vec2 rot = {circleShape->m_p.x*cos(angle)-circleShape->m_p.y*sin(angle),
                            circleShape->m_p.x*sin(angle)+circleShape->m_p.y*cos(angle)};
                b2Vec2 ve = position+rot;
                circle.setPosition( ve.x * SCALE, ve.y * SCALE);
                circle.setRotation(angle * DEG);
                circle.setFillColor(color);
                mainWindow->draw(circle);
                break;
            }

            case b2Shape::e_polygon: {
                b2PolygonShape* polygonShape = static_cast<b2PolygonShape*>(shape);
                sf::ConvexShape polygon;
                int32 vertexCount = polygonShape->m_count;
                polygon.setPointCount(vertexCount);
                for (int32 i = 0; i < vertexCount; i++) {
                    b2Vec2 vertex = polygonShape->m_vertices[i];
                    polygon.setPoint(i, sf::Vector2f((vertex.x * cos(angle) - vertex.y * sin(angle) + position.x) * SCALE,
                                                     (vertex.x * sin(angle) + vertex.y * cos(angle) + position.y) * SCALE));
                }
                polygon.setFillColor(color);
                mainWindow->draw(polygon);
                break;
            }
            default:
                break;
        }
    }
}

void setWall(int x,int y,int w,int h,int idx){
    w/=2,h/=2;
    b2PolygonShape gr;
    gr.SetAsBox(w/SCALE,h/SCALE);

    b2BodyDef bdef;
    bdef.position.Set(x/SCALE, y/SCALE);

    b2Body *b_ground = World.CreateBody(&bdef);
    b_ground->CreateFixture(&gr,1);
    mp[b_ground]=idx;
}

b2Body* setball(b2Vec2 pos,int siz,int idx){
    b2BodyDef bdef;
    bdef.position.Set(pos.x/SCALE,pos.y/SCALE);

    b2CircleShape circle;
    circle.m_radius=siz/SCALE;
    b2Body *b = World.CreateBody(&bdef);

    b2FixtureDef fdef;
    fdef.shape=&circle;
    fdef.restitution=1;
    b->CreateFixture(&fdef);

    mp[b]=idx;
}
b2Body* set_dball(b2Vec2 pos,int idx){
    b2BodyDef bdef;
    bdef.type=b2_dynamicBody;
    bdef.position.Set(pos.x/SCALE,pos.y/SCALE);

    b2CircleShape circle;
    circle.m_radius=18/SCALE;
    b2Body *b = World.CreateBody(&bdef);


    b2FixtureDef fdef;
    fdef.shape=&circle;
    fdef.restitution=1.1;
    fdef.density=2;

    b->CreateFixture(&fdef);
    mp[b]=idx;
    return b;
}
int seed;
std::mt19937 rd;
int random(int l,int r){
	std::uniform_int_distribution<int> d(l,r);
	return d(rd);
}
double fandom(double l,double r){
	std::uniform_real_distribution<double> d(l,r);
	return d(rd);
}


p3 bas[]={{20,255,60},{255,20,20},{80,20,255},{255,201,14}};

void create(int color){
    all_cnt++;
    num[color]++;
    b2Vec2 rd = {fandom(0,1400),fandom(0,1400)};
    auto tp = set_dball(rd,color);
    float tt=10,sita=fandom(0,2*pi);
    tp->SetLinearVelocity({tt*cos(sita),tt*sin(sita)});

    mp_c[tp]=mp_cn[tp]=bas[color];
}




float tim = 0.0f;

void init(){
    World.SetContactListener(&contactListener);

    setWall(0, 700, 60, 2000, -1);
    setWall(1400, 700, 60, 2000, -1);
    setWall(700, 0, 2000, 60, -1);
    setWall(700, 1400, 2000, 60, -1);

    setball({350,350},80,-2);
    setball({1400-350,350},80,-2);
    setball({350,1050},80,-2);
    setball({1400-350,1050},80,-2);

    setball({0,700},90,-2);
    setball({1400,700},90,-2);
    setball({700,0},90,-2);
    setball({700,1400},90,-2);

    setball({700,700},80,-2);

    for(int i=0;i<5;i++){
        for(int j=0;j<Groups;j++){
            create(j);
        }
    }
}


int u_time=0;
int min_cnt[]={10000,10000,10000,10000};
int min_num[]={0,0,0,0};
int all_exist=0;

bool ed(){
    bool ed=0;
    for(int i=0;i<Groups;i++) if(num[i]==all_cnt) ed=1;
    return ed;
}

void update(){
        u_time++;
        tim += 1.0f / 60.0f;
        for (b2Body* it = World.GetBodyList(); it != 0; it = it->GetNext()){
            int gp=mp[it];
            if(gp>=0){
                int cnt = num[gp];
                float rest = (cnt < 3)? 1.12f: 1.1f;
                for (b2Fixture* f = it->GetFixtureList(); f; f = f->GetNext()) {
                    f->SetRestitution(rest);
                }
            }
        }

        for(int i=0;i<Groups;i++) min_cnt[i]=std::min(min_cnt[i],num[i]),min_num[i]+=(num[i]==1);
        if(num[0]&&num[1]&&num[2]&&num[3]) all_exist++;

        if(!(ed() || u_time>60*120) ){
            for(int _=0;_<2;_++) World.Step(1/60.f, 8, 3);
        }else{
            std::cout<<u_time/60<<'\n';
            for(int i=0;i<Groups;i++){
                if(num[i]==all_cnt) std::cout<<min_cnt[i]<<'\n'<<min_num[i]<<'\n';
            }

            std::cout<<all_exist<<'\n'<<seed<<'\n';
            exit(0);
        }
}
int get_now(){
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int res=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    return res;
}
sf::Clock deltaClock;

int main() {
    int mode = 0;
    if(mode){
        seed = get_now();
        rd=std::mt19937(seed);
        init();while(1) update();
    }


    seed = 131317134;
    rd=std::mt19937(seed);



    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    RenderWindow window(VideoMode(1400, 1400), "test", sf::Style::Default, settings);
    mainWindow = &window;
    window.setFramerateLimit(60);

    init();

    while (window.isOpen()) {


        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
        }

        tim += 1.0f / 60.0f;
        for (b2Body* it = World.GetBodyList(); it != 0; it = it->GetNext()){
            int gp=mp[it];
            if(gp>=0){
                int cnt = num[gp];
                float rest = (cnt < 3)? 1.12f: 1.1f;
                for (b2Fixture* f = it->GetFixtureList(); f; f = f->GetNext()) {
                    f->SetRestitution(rest);
                }
            }
        }

        for(int _=0;_<2;_++) World.Step(1/60.f, 8, 3);

        window.clear(sf::Color(10, 10, 10));

        for (b2Body* it = World.GetBodyList(); it != 0; it = it->GetNext()) drawbody(it);
        float dt = deltaClock.restart().asSeconds();
        updateAndDrawMergeEffects(dt);
        window.display();
    }

    return 0;
}
