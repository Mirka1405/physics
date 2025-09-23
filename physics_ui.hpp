#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include "raylib.h"
#include "physics_variables.hpp"
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
    oss << "Scale: ";
    oss << std::fixed << std::setprecision(2) << 100/windowScale << "%";
    
    DrawText(oss.str().c_str(),getX(),getY(),24,WHITE);
  }
};
class TimeScaleUI : public UI{
  public:
  using UI::UI;
  void draw(){
    std::ostringstream oss;
    oss << "Time: ";
    oss << std::fixed << std::setprecision(3) << timeScale << "x";
    DrawText(oss.str().c_str(),getX(),getY(),24,WHITE);
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
    this->text = "Made by "+name;
  }
  void draw(){
    DrawText(text.c_str(),getX(),getY(),6,WHITE);
  }
};
class VisualScaleUI : public UI{
  public:
  using UI::UI;
  std::string text;
  VisualScaleUI(int x=0,int y=0):UI(x,y){
    this->text = "Visual scaling: ";
  }
  void draw(){
    if(visualScaling) DrawText((text+std::to_string(windowVisualScale)+"x").c_str(),getX(),getY(),24,WHITE);
  }
};

std::list<UI*> UIList;