#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include "raylib.h"
#include "physics_variables.hpp"
#include "physics_localisation.hpp"
#include <list>

class UI{
  public:
  Vector2 pos;
  UI(int x=0,int y=0){
    this->pos = vector(x,y);
  }
  virtual void draw(){};
  virtual ~UI() = default;
  int getX(){
    if(this->pos.x<0) return GetScreenWidth()+this->pos.x;
    return this->pos.x;
  }
  int getY(){
    if(this->pos.y<0) return GetScreenHeight()+this->pos.y;
    return this->pos.y;
  }
};
class WindowScaleUI : public UI{
  public:
  using UI::UI;
  void draw(){
    std::ostringstream oss;
    oss << L("ui.scale");
    oss << std::fixed << std::setprecision(2) << 100/windowScale << "%";
    
    DrawTextEx(uiFont,oss.str().c_str(),vector(getX(),getY()),24,1.0f, WHITE);
  }
};
class TimeScaleUI : public UI{
  public:
  using UI::UI;
  void draw(){
    std::ostringstream oss;
    oss << L("ui.time");
    oss << std::fixed << std::setprecision(3) << timeScale << "x";
    DrawTextEx(uiFont,oss.str().c_str(),vector(getX(),getY()),24,1.0f, WHITE);
  }
};
class PauseUI : public UI{
  public:
  using UI::UI;
  void draw(){
    if(paused){
      DrawRectangle(getX(),getY(),6,24,WHITE);
      DrawRectangle(getX()+9,getY(),6,24,WHITE);
    }
  }
};
class OwnershipUI : public UI{
  public:
  using UI::UI;
  std::string text;
  OwnershipUI(const std::string& name,int x=0,int y=0):UI(x,y){
    this->text = L("ui.madeby")+name;
  }
  void draw(){
    DrawTextEx(uiFont,text.c_str(),vector(getX(),getY()),18,1.0f, WHITE);
  }
};
class VisualScaleUI : public UI{
  public:
  using UI::UI;
  std::string text;
  VisualScaleUI(int x=0,int y=0):UI(x,y){
    this->text = L("ui.visual_scaling");
  }
  void draw(){
    if(visualScaling) DrawTextEx(uiFont,(text+std::to_string(windowVisualScale)+"x").c_str(),vector(getX(),getY()),24,1.0f, WHITE);
  }
};
class TrailLifetimeUI : public UI{
  public:
  using UI::UI;
  std::string text;
  TrailLifetimeUI(int x=0,int y=0):UI(x,y){
    this->text = L("ui.trail_lifetime");
  }
  void draw(){
    if(editingTrailLifetime) DrawTextEx(uiFont,(text+std::to_string(trailLifetime)+"s").c_str(),vector(getX(),getY()),24,1.0f, WHITE);
  }
};

std::list<UI*> UIList;