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
  bool fixed = false;
  bool leaveTrail = false;
  Vector2 lastTrailPos;
  double timeAlive = 0;
  Object* lastCollision = nullptr;
  float elasticity = 0.5; // range: [0;1] || if any at 0, objects are combined at collision; in other cases momentum is transferred back
  Object(Vector2 pos, Vector2 init_speed, Color color, double mass, double frictionFactor, bool leaveTrail = false){
    this->pos = pos;
    this->speed = init_speed;
    this->frictionFactor = frictionFactor;
    this->color = color;
    this->mass = mass;
    this->uuid = getUUID();
    this->leaveTrail = leaveTrail;
    this->lastTrailPos = vector(pos.x,pos.y);
  }
  virtual void defaultRender(Color col) = 0;
  void tickTime(){
    auto ft = GetFrameTime()*timeScale;
    timeAlive+=ft;
  }
  virtual void tick(){
    tickTime();
    auto ft = GetFrameTime()*timeScale;
    this->pos+=this->speed*ft;
    this->speed*=1-frictionFactor*ft;
    if(this->gravityAffected)this->speed.y+=freeFallAcceleration*ft;
  };
  virtual void draw(){
    defaultRender(color);
  };
  void tickWithAttractionForce();
  bool tickLifeTime(double maxLifeTimeSeconds){ // returns true if this is the tick when the Object starts being marked as deleted
    tickTime();
    if(shouldRemove)return false;
    if(maxLifeTimeSeconds<=timeAlive)shouldRemove=true;
    return shouldRemove;
  }
  virtual bool checkCollision(Object* o, bool desperate=false) = 0; // desperate is the last search flag, to prevent recursion
  virtual double area() = 0;
  virtual void setArea(double a) = 0;
};
Object::~Object() {}
std::list<Object*> objectList;
class CircularObject : public Object{
  public:
  virtual ~CircularObject() = default;
  double radius;
  CircularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius=1, double frictionFactor=0.02, bool leaveTrail = false): Object(pos,init_speed,color,mass,frictionFactor, leaveTrail){
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
  void trail();
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
  }
};
class TrailParticle : public CircularObject {
  public:
  ~TrailParticle() = default;
  TrailParticle(Vector2 pos, Vector2 init_speed, Color color, double radius=1,double frictionFactor=0.02) : CircularObject(pos,init_speed,color,0,radius,frictionFactor){
  }
  Color calculateColor(){
    Color n(this->color);
    n.a = 255-(char)(255*(timeAlive/trailLifetime));
    return n;
  }
  void draw() override{
    if(shouldRemove)return;
    defaultRender(this->calculateColor());
  }
  void tick() override{
    tickTime();
    if(timeAlive>trailLifetime) shouldRemove=true;
  }
};

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

void CircularObject::trail(){
  if (distance(pos,lastTrailPos)>2*radius){
    objectList.push_back(new TrailParticle(lastTrailPos,vector(),color,radius*0.1));
    lastTrailPos.x=pos.x;lastTrailPos.y=pos.y;
  }
}


class PhysicsCircularObject : public CircularObject {
  public:
  PhysicsCircularObject(Vector2 pos, Vector2 init_speed, Color color, double mass, double radius = 1, double frictionFactor = 0.02, bool leaveTrail = false)
    : CircularObject(pos, init_speed, color, mass, radius, frictionFactor, leaveTrail) {}
  void tick() override{
    tickWithAttractionForce();
    if(leaveTrail)trail();
    tickTime();
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

void Object::tickWithAttractionForce() {
  if (lastCollision && !checkCollision(lastCollision)) lastCollision = nullptr;

  // time stepping (substep to avoid tunneling/creep)
  const double ft = GetFrameTime() * timeScale;
  const double maxDt = 1.0 / 240.0;                 // ~4.17 ms substeps
  int steps = (int)ceil(ft / maxDt);
  if (steps < 1) steps = 1;
  const double dt = ft / steps;

  for (int s = 0; s < steps; ++s) {
    Vector2 F = vector(0, 0);
    auto handleCircleRect = [&](CircularObject* circle, Object* circleObj,
                                RectangularObject* rect, Object* rectObj)
    {
      if (!circle || !rect) return;

      Rectangle r = rect->rect();
      float cx = circle->pos.x;
      float cy = circle->pos.y;
      float cr = (float)circle->radius;

      // Expand rectangle by circle radius (Minkowski sum)
      float ex = r.x - cr;
      float ey = r.y - cr;
      float ew = r.width  + 2.0f * cr;
      float eh = r.height + 2.0f * cr;

      // If center not inside expanded rect, nothing to do
      if (cx < ex || cx > ex + ew || cy < ey || cy > ey + eh) return;

      // Distances to each side of expanded rect
      float distLeft   = cx - ex;
      float distRight  = (ex + ew) - cx;
      float distTop    = cy - ey;
      float distBottom = (ey + eh) - cy;

      // Minimal translation to push circle center out
      float pen = distLeft;
      Vector2 n = { -1.0f, 0.0f }; // default: push left

      if (distRight < pen) {
        pen = distRight;
        n = { +1.0f, 0.0f };
      }
      if (distTop < pen) {
        pen = distTop;
        n = { 0.0f, -1.0f };
      }
      if (distBottom < pen) {
        pen = distBottom;
        n = { 0.0f, +1.0f };
      }

      // Full positional correction
      if (pen > 0.0f) {
        circle->pos += n * pen;
      }

      float vN = Vector2DotProduct(circleObj->speed, n);
      if (vN < 0.0f) {  // only if moving into the rect
        float e = std::clamp((float)circleObj->elasticity, 0.0f, 1.0f);
        float new_vN = -vN * e;
        circleObj->speed = circleObj->speed + n * (new_vN - vN);
      }

      circleObj->lastCollision = rectObj;
      rectObj->lastCollision   = circleObj;
    };
    for (auto i = objectList.begin(); i != objectList.end(); ) {
      if ((*i)->mass == 0 || uuid == (*i)->uuid) { ++i; continue; }

      if (checkCollision(*i)) {
        auto selfC  = dynamic_cast<CircularObject*>(this);
        auto otherC = dynamic_cast<CircularObject*>(*i);
        auto selfR  = dynamic_cast<RectangularObject*>(this);
        auto otherR = dynamic_cast<RectangularObject*>(*i);

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

            // Fixed objects don't move under position correction
            if (fixed)         im1 = 0.0;
            if ((*i)->fixed)   im2 = 0.0;

            double sum = im1 + im2; 
            if (sum == 0.0) sum = 1.0;
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
              float new_v1 = v1;
              float new_v2 = v2;

              if (fixed && !(*i)->fixed) {
                  new_v1 = 0.0f;
                  new_v2 = -v2 * avgE;
              } else if (!fixed && (*i)->fixed) {
                  new_v1 = -v1 * avgE;
                  new_v2 = 0.0f;
              } else {
                  new_v1 = ((float)(mass - (*i)->mass) * v1 + 2.0f * (float)(*i)->mass * v2) / (float)(mass + (*i)->mass);
                  new_v2 = (2.0f * (float)mass * v1 + (float)((*i)->mass - mass) * v2) / (float)(mass + (*i)->mass);
                  new_v1 *= avgE;
                  new_v2 *= avgE;
              }

              if (!fixed)
                  speed = speed - n * v1 + n * new_v1;
              if (!(*i)->fixed)
                  (*i)->speed = (*i)->speed - n * v2 + n * new_v2;

              if (lastCollision != *i && distance(speed) + distance((*i)->speed) > 20) {
                  // approximate contact point between spheres
                  Vector2 hitPos;
                  double totalR = selfC->radius + otherC->radius;
                  if (totalR > 0.0) {
                      float t = (float)(selfC->radius / totalR);
                      hitPos = selfC->pos * t + otherC->pos * (1.0f - t);
                  } else {
                      hitPos = (selfC->pos + otherC->pos) * 0.5f;
                  }

                  auto otherArea = otherC->area();
                  explosion(hitPos,
                            color,
                            std::sqrt(otherArea) * 0.2,
                            distance((*i)->speed) * 1.5,
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
        } 
        else if (selfC && otherR) {
          // circle = this, rect = *i
          handleCircleRect(selfC, this, otherR, *i);
          ++i;
          continue;
        } else if (selfR && otherC) {
          // circle = *i, rect = this
          handleCircleRect(otherC, *i, selfR, this);
          ++i;
          continue;
        } else if (selfR && otherR) {
          RectangularObject* A = selfR;
          RectangularObject* B = otherR;

          float ax = A->pos.x;
          float ay = A->pos.y;
          float bx = B->pos.x;
          float by = B->pos.y;

          float ahx = A->sides.x * 0.5f;
          float ahy = A->sides.y * 0.5f;
          float bhx = B->sides.x * 0.5f;
          float bhy = B->sides.y * 0.5f;

          // delta between centers
          float dx = bx - ax;
          float dy = by - ay;

          float px = (ahx + bhx) - std::fabs(dx);
          float py = (ahy + bhy) - std::fabs(dy);

          if (px > 0.0f && py > 0.0f) {
              Vector2 n;
              float  pen;

              if (px < py) {
                  pen = px;
                  n = { dx >= 0.0f ? 1.0f : -1.0f, 0.0f };
              } else {
                  pen = py;
                  n = { 0.0f, dy >= 0.0f ? 1.0f : -1.0f };
              }

              double imA = (mass       > 0.0) ? 1.0 / mass       : 0.0;
              double imB = ((*i)->mass > 0.0) ? 1.0 / (*i)->mass : 0.0;
              double sum = imA + imB;
              if (sum == 0.0) sum = 1.0;

              A->pos -= n * (float)(pen * (imA / sum));
              B->pos += n * (float)(pen * (imB / sum));

              float vA = Vector2DotProduct(speed,        n);
              float vB = Vector2DotProduct((*i)->speed,  n);

              if (vA - vB > 0.0f) {
                  float e = std::clamp(
                      (float)((elasticity + (*i)->elasticity) * 0.5),
                      0.0f, 1.0f
                  );

                  double mA = mass;
                  double mB = (*i)->mass;
                  if (mA + mB <= 0.0) {
                      float new_vA = -vA * e;
                      speed = speed - n * vA + n * new_vA;
                  } else {
                      float new_vA = ((float)(mA - mB) * vA + 2.0f * (float)mB * vB) / (float)(mA + mB);
                      float new_vB = (2.0f * (float)mA * vA + (float)(mB - mA) * vB) / (float)(mA + mB);
                      new_vA *= e;
                      new_vB *= e;

                      speed        = speed        - n * vA + n * new_vA;
                      (*i)->speed  = (*i)->speed  - n * vB + n * new_vB;
                  }
              }
          }

          (*i)->lastCollision = this;
          lastCollision       = *i;
          ++i;
          continue;
        } else {
          (*i)->lastCollision = this;
          lastCollision       = *i;
          ++i;
          continue;
        }
      }

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
    if (!fixed) {
      speed *= (float)std::exp(-frictionFactor * dt);
      if (gravityAffected) speed.y += (float)(freeFallAcceleration * dt);
      if (mass > 0.0) {
        Vector2 a = F / mass;
        speed += a * (float)dt;
      }
      pos += speed * (float)dt;
    }

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
