#pragma once
#include "physics_functions.hpp"
#include "raylib.h"
int scrollSpeed = 2;
float timeScale = 1;
bool paused = true;
int screenWidth = 1200;
int screenHeight = 900;
double freeFallAcceleration = 9.81;
double gravitationalConstant = 6.67430e-11;
auto windowPos = vector(0,0);
double windowScale = 1;
double windowVisualScale = 1;
double trailLifetime = 20;
bool visualScaling = false;
bool editingTrailLifetime = false;
double mouseWheelScaleFactor = 0.1;
double visualScale(){
  if(visualScaling)return windowVisualScale;
  return 1;
}
const char* lang = "en";
Font uiFont;