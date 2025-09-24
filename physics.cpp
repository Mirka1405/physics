#include "raylib.h"
#include "physics_engine.hpp"
#include "physics_ui.hpp"
#include "physics_functions.hpp"
#include "physics_editor.hpp"

extern int scrollSpeed;
extern float timeScale;
extern bool paused;
extern int screenWidth;
extern int screenHeight;
extern double freeFallAcceleration;
extern double gravitationalConstant;
extern Vector2 windowPos;
extern double windowScale;
extern double windowVisualScale;
extern bool visualScaling;
extern double mouseWheelScaleFactor;
/*std::ostream& operator<<(std::ostream& out,const Vector2& v){
  out<<"("<<v.x<<", "<<v.y<<")";
  return out;
}*/
extern std::list<Object*> objectList;
auto lastVisitedObject = objectList.begin();
extern std::list<UI*> UIList;
void debugPreInit(){
  // ---- PHYSICAL CONSTANTS (SI) ----
  const double G_SI    = 6.67430e-11;         // m^3 / (kg s^2)
  const double AU_m    = 1.495978707e11;      // meters
  const double R_SUN_m = 6.9634e8;            // meters
  const double M_SUN   = 1.98847e30;          // kg

  // ---- ENGINE UNIT SCALING ----
  // We want Sun radius = 100 engine units -> 1 engine unit = R_SUN_m / 100
  const double UNIT_m = R_SUN_m / 100.0;      // meters per engine unit
  // Rescale G for engine units (distance in "UNIT_m", time in seconds):
  // a' [unit/s^2] = (G_SI / UNIT_m^3) * M / r'^2
  gravitationalConstant = G_SI / (UNIT_m*UNIT_m*UNIT_m);

  // (optional) keep local gravity for "gravityAffected" objects negligible here
  freeFallAcceleration = 0.0;

  // Helper to add a body
  auto addBody = [](Vector2 p, Vector2 v, Color c, double mass, double radius, float elasticity = 1.0f){
    auto* obj = new PhysicsCircularObject(p, v, c, mass, radius, /*frictionFactor=*/0.0);
    obj->elasticity = elasticity;
    objectList.push_back(obj);
    return obj;
  };

  // Compute circular orbital speed in engine units: v' = sqrt(G' * M_central / r')
  auto circSpeed = [](double Gp, double Mcentral, double r_units){
    return std::sqrt(Gp * Mcentral / r_units);
  };

  // ---- SUN ----
  const double R_SUN_units = 100.0;           // by requirement
  auto* sun = addBody(vector(0,0), vector(0,0), {253, 184, 19, 255}, M_SUN, R_SUN_units); // yellowish

  // ---- PLANET DATA (approx mean values) ----
  struct PlanetDef {
    const char* name;
    double a_AU;     // semi-major axis in AU (we'll do circular approx at 'a')
    double radius_m; // mean radius in meters
    double mass_kg;  // mass in kg
    Color color;
  };

  PlanetDef planets[] = {
    {"Mercury", 0.387,  2.4397e6,   3.3011e23, {169,169,169,255}}, // gray
    {"Venus",   0.723,  6.0518e6,   4.8675e24, {218,165, 32,255}}, // golden
    {"Earth",   1.000,  6.3710e6,   5.97237e24,{ 70,130,180,255}}, // steel blue
    {"Mars",    1.524,  3.3895e6,   6.4171e23, {205, 92,  92,255}},// indian red
    {"Jupiter", 5.203,  6.9911e7,   1.8982e27, {222,184,135,255}}, // burlywood
    {"Saturn",  9.537,  5.8232e7,   5.6834e26, {210,180,140,255}}, // tan
    {"Uranus", 19.191,  2.5362e7,   8.6810e25, {175,238,238,255}}, // pale turquoise
    {"Neptune",30.070,  2.4622e7,   1.02413e26,{ 65,105,225,255}}, // royal blue
  };

  // Place planets on +X axis; give +Y velocity for counterclockwise orbits.
  for (const auto& P : planets) {
    const double r_units   = (P.a_AU * AU_m) / UNIT_m;      // distance in engine units
    const double R_units   =  P.radius_m / UNIT_m;          // radius in engine units (to scale!)
    const double v_units   = circSpeed(gravitationalConstant, M_SUN, r_units);

    // Position and velocity (perpendicular)
    Vector2 pos = vector((float)r_units, 0.0f);
    Vector2 vel = vector(0.0f, (float)v_units);

    auto* body = addBody(pos, vel, P.color, P.mass_kg, R_units, /*elasticity=*/1.0f);
    (void)body;
  }
  windowPos = sun->pos - vector(screenWidth, screenHeight)*windowScale/2.0;
}

void UIPreInit(){
  UIList.push_back(new WindowScaleUI());
  UIList.push_back(new TimeScaleUI(0,24));
  UIList.push_back(new PauseUI(6,-32));
  UIList.push_back(new OwnershipUI("Miron Samokhvalov",-150,0));
  UIList.push_back(new VisualScaleUI(0,48));
}

int main(){
  //debugPreInit();
  UIPreInit();
  StarsInit();
  
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, "physics!");
  SetTargetFPS(60);
  
  PhysEditor::Init();
  
  while(!WindowShouldClose()){
    if(IsWindowResized()){
      screenWidth = GetScreenWidth();
      screenHeight = GetScreenHeight();
      SetWindowSize(screenWidth,screenHeight);
      StarsInit();
    }
    double keyscale = getKeyScale();
    
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!PhysEditor::S().visible){
      PhysEditor::HandleLeftClickCreate(GetMousePosition());
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
      PhysEditor::OnRightClick(GetMousePosition());
    }
    if(IsKeyPressed(KEY_SPACE)){
      windowPos=vector();
    }
    if(IsKeyPressed(KEY_TAB)){
      paused=!paused;
    }
    if(IsKeyPressed(KEY_PERIOD)){
      if(objectList.size()>0){
        if(lastVisitedObject==objectList.end()) lastVisitedObject=objectList.begin();
        windowPos=(*lastVisitedObject)->pos-vector(screenWidth,screenHeight)*windowScale/2;
        do{
          lastVisitedObject++;
          if(lastVisitedObject==objectList.end()) lastVisitedObject=objectList.begin();
        }while((*lastVisitedObject)->mass==0);
      }
    }
    if(IsKeyDown(KEY_W)){
      windowPos.y-=scrollSpeed*windowScale*keyscale;
    }
    if(IsKeyDown(KEY_S)){
      windowPos.y+=scrollSpeed*windowScale*keyscale;
    }
    if(IsKeyDown(KEY_A)){
      windowPos.x-=scrollSpeed*windowScale*keyscale;
    }
    if(IsKeyDown(KEY_D)){
      windowPos.x+=scrollSpeed*windowScale*keyscale;
    }
    if(IsKeyPressed(KEY_EQUAL)){
      if(visualScaling) windowVisualScale+=0.1*keyscale;
      else timeScale+=0.1*keyscale;
    }
    if(IsKeyPressed(KEY_MINUS)){
      if(visualScaling) windowVisualScale-=0.1*keyscale;
      else timeScale-=0.1*keyscale;
    }
    if(IsKeyPressed(KEY_C))visualScaling=!visualScaling;
    auto mw_mv = -GetMouseWheelMove()*mouseWheelScaleFactor*keyscale;
    
    windowScale+=mw_mv;
    if(windowScale<0.1)windowScale=0.1;
    else if(mw_mv>0)windowPos-=vector(screenWidth,screenHeight)*mw_mv/2;
    else if(mw_mv<0)windowPos-=vector(screenWidth,screenHeight)*mw_mv/2;
    BeginDrawing();
    
    ClearBackground(BLACK);
    for(const auto& s : gStars){
      DrawCircleV(s.pos,s.size,s.color);
    }
    auto it = objectList.begin();
    while (it != objectList.end()) {
        if ((*it)->shouldRemove) {
            delete *it;
            it = objectList.erase(it);
        } else {
            if (!paused) {
                (*it)->tick();
            }
            (*it)->draw();
            ++it;
        }
    }
    for(auto it=UIList.begin();it!=UIList.end();it++){
      (*it)->draw();
    }
    PhysEditor::Draw();
    EndDrawing();
  }
  CloseWindow();
  for (auto obj : objectList) delete obj;
  for (auto UI : UIList) delete UI;
}