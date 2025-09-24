#pragma once
#include "physics_variables.hpp"
#include "raylib.h"
#include <algorithm>
#include "physics_functions.hpp"
#include <list>

class Object{
  public:
  virtual ~Object();
  Vector2 pos;
  Vector2 speed;
  Color color;
  double frictionFactor;
  double mass;
  unsigned long long uuid;
  bool shouldRemove = false;
  bool gravityAffected = false;
  double timeAlive = 0;
  Object* lastCollision = nullptr;
  float elasticity = 0.5; // range: [0;1] || if any at 0, objects are combined at collision; in other cases momentum is transferred back
  Object(Vector2 pos, Vector2 init_speed, Color color, double mass, double frictionFactor){
    this->pos = pos;
    this->speed = init_speed;
    this->frictionFactor = frictionFactor;
    this->color = color;
    this->mass = mass;
    this->uuid = getUUID();
  }
  virtual void defaultRender(Color col) = 0;
  virtual void tick(){
    auto ft = GetFrameTime()*timeScale;
    timeAlive+=ft;
    this->pos+=this->speed*ft;
    this->speed*=1-frictionFactor*ft;
    if(this->gravityAffected)this->speed.y+=freeFallAcceleration*ft;
  };
  virtual void draw(){
    defaultRender(color);
  };
  void tickWithAttractionForce();
  bool tickLifeTime(double maxLifeTimeSeconds){ // returns true if this is the tick when the Object starts being marked as deleted
    if(shouldRemove)return false;
    if(maxLifeTimeSeconds<=timeAlive)shouldRemove=true;
    return shouldRemove;
  }
  virtual bool checkCollision(Object* o, bool desperate=false) = 0; // desperate is the last search flag, to prevent recursion
  virtual double area() = 0;
  virtual void setArea(double a) = 0;
};
Object::~Object() {}
class CircularObject : public Object{
  public:
  virtual ~CircularObject() = default;
  double radius;
  CircularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius=1, double frictionFactor=0.02): Object(pos,init_speed,color,mass,frictionFactor){
    this->radius = radius;
  }
  void defaultRender (Color col){
    DrawCircleV((this->pos-windowPos)/windowScale,this->radius/windowScale*visualScale(),col);
  }
  double area(){
    return radius*radius*M_PI;
  }
  void setArea(double a){
    radius = sqrt(a/M_PI);
  }
  bool checkCollision(Object* o, bool desperate=false){
    auto* c = dynamic_cast<CircularObject*>(o);
    if (c) {
      return distance(pos,c->pos)<radius+c->radius;
    }
    if(!desperate) return o->checkCollision(this,true);
    return false;
  }
};
class Particle : public CircularObject{
  public:
  ~Particle() = default;
  double fadeSeconds;
  Particle(Vector2 pos, Vector2 init_speed, Color color, double radius=1, double fadeSeconds=3, double frictionFactor=0.02) : CircularObject(pos,init_speed,color,0,radius,frictionFactor){
    this->fadeSeconds = fadeSeconds;
  }
  Color calculateColor(){
    Color n(this->color);
    n.a = 255-(char)(255*(timeAlive/fadeSeconds));
    return n;
  }
  void draw() override{
    if(shouldRemove)return;
    defaultRender(this->calculateColor());
  }
  void tick() override{
    CircularObject::tick();
    CircularObject::tickLifeTime(fadeSeconds);
  };
};

std::list<Object*> objectList;
void explosion(Vector2 pos,Color color,double maxSize,double speed=1,int maxParticles=30,int minParticles=0){
  for(int _=0;_<minParticles+randFloat()*(maxParticles-minParticles);_++){
    objectList.push_back(new Particle(pos,vector(randNegFloat(),randNegFloat())*speed,color,maxSize*randFloat()));
  }
}
void explosion(Vector2 pos,Color color,double maxSize,Vector2 speed,int maxParticles=30,int minParticles=0){
  for(int _=0;_<minParticles+randFloat()*(maxParticles-minParticles);_++){
    objectList.push_back(new Particle(pos,vector(randNegFloat(),randNegFloat())*speed,color,maxSize*randFloat()));
  }
}
/*
void Object::tickWithAttractionForce(){ // if called, you don't need to call Object::tick().
  if(lastCollision&&!checkCollision(lastCollision)){lastCollision=nullptr;}
  // F = mg
  // F = frictionFactor * mg
  // F = ma
  // F = G * m1 * m2 / dist / dist
  Vector2 F = vector(0,0);
  for(auto i=objectList.begin();i!=objectList.end();){
    if((*i)->mass==0||uuid==(*i)->uuid){i++;continue;}
    if(checkCollision((*i))){
      auto ownArea = area();
      auto otherArea = (*i)->area();
      if (elasticity != 0 && (*i)->elasticity != 0) {
        if(distance(speed)<1 && distance((*i)->speed)<1){i++;continue;}
        auto avgElasticity = (elasticity+(*i)->elasticity)/2;
        Vector2 collision_dir = (*i)->pos-pos;
        Vector2 collision_normal = Vector2Normalize(collision_dir);
        if (auto selfC = dynamic_cast<CircularObject*>(this)) {
            if (auto otherC = dynamic_cast<CircularObject*>(*i)) {
                double dx = otherC->pos.x - selfC->pos.x;
                double dy = otherC->pos.y - selfC->pos.y;
                double d  = std::sqrt(dx*dx + dy*dy);
                double target = selfC->radius + otherC->radius;
                double pen = target - d;
                if (pen > 0.0 && d > 0.0) {
                    // Baumgarte-style positional correction with slop to reduce jitter
                    const double slop = 0.1;     // allowed tiny penetration (engine units)
                    const double percent = 0.8;  // resolve 80% per frame
                    double corr = std::max(0.0, pen - slop) * percent;

                    Vector2 n = { (float)(dx / d), (float)(dy / d) };
                    double im1 = (mass > 0.0)      ? 1.0 / mass        : 0.0;
                    double im2 = ((*i)->mass > 0.) ? 1.0 / (*i)->mass  : 0.0;
                    double sum = im1 + im2; if (sum == 0.0) sum = 1.0;

                    selfC->pos  -= n * (float)(corr * (im1 / sum));
                    otherC->pos += n * (float)(corr * (im2 / sum));
                }
            }
        }
        float v1 = Vector2DotProduct(speed, collision_normal);
        float v2 = Vector2DotProduct((*i)->speed, collision_normal);
        
        if (v1 - v2 <= 0) {
          i++;
          continue;
        }

        float new_v1 = ((mass - (*i)->mass) * v1 + 2 * (*i)->mass * v2) / (mass + (*i)->mass);
        float new_v2 = (2 * mass * v1 + ((*i)->mass - mass) * v2) / (mass + (*i)->mass);
        
        new_v1 *= avgElasticity;
        new_v2 *= avgElasticity;

        speed = speed-collision_normal*v1+collision_normal*new_v1;
        (*i)->speed = (*i)->speed-collision_normal*v2+collision_normal*new_v2;

        if(distance(speed)+distance((*i)->speed)>20)explosion((*i)->pos, color, sqrt(otherArea)*0.2, distance((*i)->speed)*1.5, (int)(sqrt(distance(speed)+distance((*i)->speed))));
        (*i)->lastCollision = this;
        lastCollision = *i;
        i++;
        continue;
      }
      if(otherArea>ownArea){
        auto oldPos = pos;
        pos=(*i)->pos;
        (*i)->pos=oldPos; // for correct particle spawning
      }
      explosion((*i)->pos,(*i)->color,sqrt(otherArea)*0.5,distance((*i)->speed)*0.5);
      setArea(ownArea+otherArea);
      auto areaSum = ownArea+otherArea;
      ownArea/=areaSum; // 0 to 1
      otherArea/=areaSum; // 0 to 1
      //v = (m1*v1 + m2*v2)/m
      auto sumMass = mass+(*i)->mass;
      speed = (speed*mass+(*i)->speed*(*i)->mass)/sumMass;
      mass=sumMass;
      color.r = static_cast<unsigned char>(std::clamp(color.r*ownArea + (*i)->color.r*otherArea,0.0,255.0)); // weighted avg
      color.g = static_cast<unsigned char>(std::clamp(color.g*ownArea + (*i)->color.g*otherArea,0.0,255.0)); 
      color.b = static_cast<unsigned char>(std::clamp(color.b*ownArea + (*i)->color.b*otherArea,0.0,255.0));
      delete *i;
      i=objectList.erase(i);
      continue;
    }
    double d = distance(this->pos,(*i)->pos);
    double scalarF = gravitationalConstant*this->mass*(*i)->mass/d/d;
    F+=vector((*i)->pos.x-this->pos.x,(*i)->pos.y-this->pos.y)/d*scalarF;
    i++;
  }
  Object::tick();
  this->speed+=F/this->mass*GetFrameTime()*timeScale;
}
*/
void Object::tickWithAttractionForce() { // replaces your current implementation
  if (lastCollision && !checkCollision(lastCollision)) lastCollision = nullptr;

  // time stepping (substep to avoid tunneling/creep)
  const double ft = GetFrameTime() * timeScale;
  const double maxDt = 1.0 / 240.0;                 // ~4.17 ms substeps
  int steps = (int)ceil(ft / maxDt);
  if (steps < 1) steps = 1;
  const double dt = ft / steps;

  for (int s = 0; s < steps; ++s) {
    Vector2 F = vector(0, 0);

    for (auto i = objectList.begin(); i != objectList.end(); ) {
      if ((*i)->mass == 0 || uuid == (*i)->uuid) { ++i; continue; }

      // --- Collision handling (circles) ---
      if (checkCollision(*i)) {
        auto selfC  = dynamic_cast<CircularObject*>(this);
        auto otherC = dynamic_cast<CircularObject*>(*i);

        // Always do positional correction if overlapping (prevents slow sinking)
        if (selfC && otherC) {
          double dx = otherC->pos.x - selfC->pos.x;
          double dy = otherC->pos.y - selfC->pos.y;
          double d  = std::sqrt(dx*dx + dy*dy);
          double target = selfC->radius + otherC->radius;
          double pen = target - d;
          if (pen > 0.0 && d > 0.0) {
            const double slop = 0.1;      // allow tiny overlap
            const double percent = 0.8;   // resolve 80% each frame
            double corr = std::max(0.0, pen - slop) * percent;

            Vector2 n = { (float)(dx / d), (float)(dy / d) };
            double im1 = (mass > 0.0)       ? 1.0 / mass       : 0.0;
            double im2 = ((*i)->mass > 0.0) ? 1.0 / (*i)->mass : 0.0;
            double sum = im1 + im2; if (sum == 0.0) sum = 1.0;

            selfC->pos  -= n * (float)(corr * (im1 / sum));
            otherC->pos += n * (float)(corr * (im2 / sum));
          }

          // Elastic vs merge
          if (elasticity != 0 && (*i)->elasticity != 0) {
            Vector2 n = Vector2Normalize(otherC->pos - selfC->pos);
            float v1 = Vector2DotProduct(speed, n);
            float v2 = Vector2DotProduct((*i)->speed, n);
            bool approaching = (v1 - v2) > 0.0f;

            if (approaching) {
              float avgE = std::clamp((elasticity + (*i)->elasticity) * 0.5f, 0.0f, 1.0f);
              float new_v1 = ((float)(mass - (*i)->mass) * v1 + 2.0f * (float)(*i)->mass * v2) / (float)(mass + (*i)->mass);
              float new_v2 = (2.0f * (float)mass * v1 + (float)((*i)->mass - mass) * v2) / (float)(mass + (*i)->mass);
              new_v1 *= avgE; new_v2 *= avgE;

              speed       = speed        - n * v1 + n * new_v1;
              (*i)->speed = (*i)->speed  - n * v2 + n * new_v2;

              // optional fx
              if (distance(speed) + distance((*i)->speed) > 20) {
                auto otherArea = otherC->area();
                explosion((*i)->pos, color, std::sqrt(otherArea) * 0.2, distance((*i)->speed) * 1.5,
                          (int)std::sqrt(distance(speed) + distance((*i)->speed)));
              }
            }

            (*i)->lastCollision = this;
            lastCollision = *i;
            ++i;
            continue;
          } else {
            // Merge branch (any elasticity == 0)
            auto ownArea   = selfC->area();
            auto otherArea = otherC->area();

            if (otherArea > ownArea) {
              auto oldPos = pos;
              pos = (*i)->pos;
              (*i)->pos = oldPos; // for correct particle spawning
            }

            explosion((*i)->pos, (*i)->color, std::sqrt(otherArea) * 0.5, distance((*i)->speed) * 0.5);
            selfC->setArea(ownArea + otherArea);

            double areaSum = ownArea + otherArea;
            ownArea   /= areaSum;
            otherArea /= areaSum;

            double sumMass = mass + (*i)->mass;
            speed = (speed * mass + (*i)->speed * (*i)->mass) / sumMass;
            mass  = sumMass;

            color.r = (unsigned char)std::clamp(color.r * ownArea + (*i)->color.r * otherArea, 0.0, 255.0);
            color.g = (unsigned char)std::clamp(color.g * ownArea + (*i)->color.g * otherArea, 0.0, 255.0);
            color.b = (unsigned char)std::clamp(color.b * ownArea + (*i)->color.b * otherArea, 0.0, 255.0);

            delete *i;
            i = objectList.erase(i);
            continue;
          }
        } else {
          // non-circular collisions: mark contact; no special resolution here
          (*i)->lastCollision = this;
          lastCollision = *i;
          ++i;
          continue;
        }
      }

      // --- Gravity accumulation with softening (avoids spikes) ---
      double dx = (*i)->pos.x - pos.x;
      double dy = (*i)->pos.y - pos.y;
      const double soft2 = 1e-4;                 // tune in engine units^2
      double r2 = dx*dx + dy*dy + soft2;
      double invR = 1.0 / std::sqrt(r2);
      double scalarF = gravitationalConstant * mass * (*i)->mass / r2;
      F += vector(dx * invR, dy * invR) * scalarF;

      ++i;
    } // objects loop

    // friction as exponential decay for stability
    speed *= (float)std::exp(-frictionFactor * dt);
    if (gravityAffected) speed.y += (float)(freeFallAcceleration * dt);
    if (mass > 0.0) {
      Vector2 a = F / mass;
      speed += a * (float)dt;
    }
    pos += speed * (float)dt;

    // hygiene
    if (!std::isfinite(speed.x)) speed.x = 0;
    if (!std::isfinite(speed.y)) speed.y = 0;
    if (!std::isfinite(pos.x))   pos.x   = 0;
    if (!std::isfinite(pos.y))   pos.y   = 0;
    const double VMAX = 1e7;
    double vlen = distance(speed);
    if (vlen > VMAX) speed *= (float)(VMAX / vlen);
  }
}

class PhysicsCircularObject : public CircularObject {
  public:
  PhysicsCircularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius = 1, double frictionFactor = 0.02)
    : CircularObject(pos, init_speed, color, mass, radius, frictionFactor) {}
  void tick() override{
    tickWithAttractionForce();
  }
};

class Rocket : public PhysicsCircularObject{
  public:
  ~Rocket() = default;
  double fireworkAccelerationFactor = 0.2;
  Rocket(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius=1, double frictionFactor=0.02) : PhysicsCircularObject(pos,init_speed,color,mass,radius,frictionFactor){}
  void tick(){
    PhysicsCircularObject::tick();
    this->speed+=speed*fireworkAccelerationFactor*GetFrameTime()*timeScale;
    explosion(pos,color,radius*0.3,vector(10,10),1);
  };
};
class Firework : public Rocket{
  public:
  ~Firework() = default;
  double lifeTime;
  Firework(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius=1, double frictionFactor=0.02, double lifeTime=10) : Rocket(pos,init_speed,color,mass,radius,frictionFactor){
    this->lifeTime = lifeTime;
  }
  void tick(){
    Rocket::tick();
    if(tickLifeTime(lifeTime)) explosion(pos,color,radius*0.7,50,300,100);
  }
};

class RectangularObject : public Object{
  public:
  virtual ~RectangularObject() = default;
  Vector2 sides;
  RectangularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, Vector2 sides, double frictionFactor=0.02): Object(pos,init_speed,color,mass,frictionFactor){
    this->sides = sides;
  }
  void defaultRender (Color col){
    DrawRectangleRec(rectOnScreen(), color);
  }
  double area(){
    return sides.x*sides.y;
  }
  void setArea(double a){
    sides*=a/area();
  }
  Rectangle rect(){
    Rectangle r;
    r.width = sides.x;
    r.height = sides.y;
    r.x = pos.x-r.width/2;
    r.y = pos.y-r.height/2;
    return r;
  }
  Rectangle rectOnScreen(){
    Rectangle r;
    r.width = sides.x/windowScale;
    r.height = sides.y/windowScale;
    r.x = (pos.x-windowPos.x)/windowScale*visualScale()-r.width/2;
    r.y = (pos.y-windowPos.y)/windowScale*visualScale()-r.height/2;
    return r;
  }
  bool checkCollision(Object* o, bool desperate=false){ 
    auto* c = dynamic_cast<RectangularObject*>(o);
    if (c) {
      return CheckCollisionRecs(rect(), c->rect());
    }
    auto* r = dynamic_cast<CircularObject*>(o);
    if (r) {
      return CheckCollisionCircleRec(r->pos, r->radius, rect());
    }
    if(!desperate) return o->checkCollision(this,true);
    return false;
  }
};
class PhysicsRectangularObject : public RectangularObject {
  public:
  PhysicsRectangularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, Vector2 sides, double frictionFactor = 0.02)
    : RectangularObject(pos, init_speed, color, mass, sides, frictionFactor) {}
  void tick() override{
    tickWithAttractionForce();
  }
};

struct Star{
  Vector2 pos;
  float size;
  Color color;
};
std::vector<Star> gStars;
void StarsInit() {
    gStars.clear();
    for (int i = 0; i < 500; i++) {
        Star s;
        s.pos = vector(randFloat() * screenWidth, randFloat() * screenHeight);
        s.size = 1 + randFloat(); // 1–3 pixels
        unsigned char brightness = 200 + (unsigned char)(55 * randFloat());
        s.color = {brightness, brightness, brightness, 255};
        gStars.push_back(s);
    }
}
