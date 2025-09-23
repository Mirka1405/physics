#pragma once
#include <random>
#include <cmath>
unsigned long long UUID = 0;
unsigned long long getUUID(){
  return UUID++;
}
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> normFloat(0.0f, 1.0f); // random number generation
std::uniform_real_distribution<float> negFloat(-1.0f, 1.0f);

float randFloat(){
  return normFloat(gen);
}
float randNegFloat(){
  return negFloat(gen);
}
Color randomColor(){
  Color c;
  c.r = randFloat()*255;
  c.g = randFloat()*255;
  c.b = randFloat()*255;
  c.a = 255;
  return c;
}

Vector2 Vector2Normalize(Vector2 v) {
    float length = sqrtf(v.x * v.x + v.y * v.y);
    if (length == 0) return (Vector2){0, 0};
    return (Vector2){v.x / length, v.y / length};
}
float Vector2DotProduct(Vector2 a, Vector2 b) {
    return (a.x * b.x) + (a.y * b.y);
}
Vector2 vector(double x,double y){
  Vector2 n;
  n.x = x;
  n.y = y;
  return n;
}
Vector2 vector(){
  return Vector2();
}
Vector2 operator+(const Vector2& a,const Vector2& b){
  return vector(a.x+b.x,a.y+b.y);
}
Vector2 operator-(const Vector2& a,const Vector2& b){
  return vector(a.x-b.x,a.y-b.y);
}
void operator+=(Vector2& a,const Vector2& b){
  a.x+=b.x;
  a.y+=b.y;
}
bool operator==(const Vector2& a,const Vector2& b){
  return a.x==b.x&&a.y==b.y;
}
void operator-=(Vector2& a,const Vector2& b){
  a.x-=b.x;
  a.y-=b.y;
}
void operator*=(Vector2& a,double b){
  a.x*=b;
  a.y*=b;
}
void operator/=(Vector2& a,double b){
  a.x/=b;
  a.y/=b;
}
Vector2 operator*(const Vector2& a,const Vector2& b){
  return vector(a.x*b.x,a.y*b.y);
}
Vector2 operator*(const Vector2& a,double b){
  return vector(a.x*b,a.y*b);
}
Vector2 operator/(const Vector2& a,double b){
  return vector(a.x/b,a.y/b);
}
double distance(const Vector2& a, const Vector2& b){
  double dx = a.x-b.x;
  double dy = a.y-b.y;
  return std::sqrt(dx*dx+dy*dy);
}
double distance(const Vector2& a){
  double dx = a.x;
  double dy = a.y;
  return std::sqrt(dx*dx+dy*dy);
}
