#include "raylib.h"
#include "physics_engine.hpp"
#include "physics_ui.hpp"
#include "physics_functions.hpp"
#include "physics_editor.hpp"
#include "physics_localisation.hpp"

#include <string>
#include <fstream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

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
extern bool editingTrailLifetime;
extern double trailLifetime;
extern double mouseWheelScaleFactor;

extern std::list<Object*> objectList;
auto lastVisitedObject = objectList.begin();
extern std::list<UI*> UIList;

// ---------------- Web helpers (WASM) ----------------
#ifdef __EMSCRIPTEN__

EM_JS(void, Web_OpenFileDialog, (), {
    if (!Module.rayFileInput) {
        Module.rayFileInput = document.createElement('input');
        Module.rayFileInput.type = 'file';
        Module.rayFileInput.style.display = 'none';
        document.body.appendChild(Module.rayFileInput);
    }
    Module.rayFileInput.value = "";

    Module.rayFileInput.onchange = function(e) {
        var file = e.target.files[0];
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function(ev) {
            var data = new Uint8Array(ev.target.result);
            var FS_ = (typeof FS !== 'undefined') ? FS : (Module.FS || null);
            if (!FS_) {
                console.error("FS not available");
                return;
            }
            try {
                if (FS_.analyzePath('/opened.csv').exists) {
                    FS_.unlink('/opened.csv');
                }
            } catch(e) {}
            FS_.writeFile('/opened.csv', data);
            Module.rayFileReady = 1;
        };
        reader.readAsArrayBuffer(file);
    };

    if (typeof Module.rayFileReady === 'undefined') Module.rayFileReady = 0;
    Module.rayFileInput.click();
});

EM_JS(int, Web_IsFileReady, (), {
    return (Module.rayFileReady|0);
});

EM_JS(void, Web_ConsumeFileReady, (), {
    Module.rayFileReady = 0;
});

EM_JS(void, Web_DownloadFile, (const char* path), {
    var p = UTF8ToString(path);
    try {
        var data = FS.readFile(p); // Uint8Array
        var blob = new Blob([data], {type: 'text/csv'});
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = p.split('/').pop();
        document.body.appendChild(a);
        a.click();
        a.remove();
        URL.revokeObjectURL(url);
    } catch(e) {
        console.error('Download failed', e);
    }
});

#else

static void Web_OpenFileDialog() {}
static int  Web_IsFileReady() { return 0; }
static void Web_ConsumeFileReady() {}
static void Web_DownloadFile(const char*) {}

#endif // __EMSCRIPTEN__

// ---------------- CSV scene save/load ----------------
void ClearScene() {
    for (auto* o : objectList) delete o;
    objectList.clear();
}

void SaveSceneCSV(const char* path) {
    std::ofstream ofs(path);
    if (!ofs) return;

    // Header:
    ofs << "Width,Height,Pos_X,Pos_Y,Speed_X,Speed_Y,Mass,Friction,Elasticity,"
           "GravityAffected,LeaveTrail,Fixed,Color_R,Color_G,Color_B\n";

    for (auto* o : objectList) {
        if (dynamic_cast<TrailParticle*>(o)) {continue;}
        if (dynamic_cast<Particle*>(o)) {continue;}
        double width = 0.0;
        double height = 0.0;

        if (auto* c = dynamic_cast<CircularObject*>(o)) {
            width  = 0.0;                 // circle flag
            height = c->radius * 2.0;     // radius = Height/2
        } else if (auto* r = dynamic_cast<RectangularObject*>(o)) {
            width  = r->sides.x;
            height = r->sides.y;
        } else {
            continue; // unknown type – skip
        }

        ofs << width << ','
            << height << ','
            << o->pos.x << ','
            << o->pos.y << ','
            << o->speed.x << ','
            << o->speed.y << ','
            << o->mass << ','
            << o->frictionFactor << ','
            << o->elasticity << ','
            << (o->gravityAffected ? 1 : 0) << ','
            << (o->leaveTrail ? 1 : 0) << ','
            << (o->fixed ? 1 : 0) << ','
            << (int)o->color.r << ','
            << (int)o->color.g << ','
            << (int)o->color.b << '\n';
    }
}

void LoadSceneCSV(const char* path) {
    std::ifstream ifs(path);
    if (!ifs) return;

    ClearScene();

    std::string line;
    if (!std::getline(ifs, line)) return; // skip header

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;

        auto readDouble = [&]() -> double {
            if (!std::getline(ss, cell, ',')) return 0.0;
            if (cell.empty()) return 0.0;
            return std::stod(cell);
        };
        auto readInt = [&]() -> int {
            if (!std::getline(ss, cell, ',')) return 0;
            if (cell.empty()) return 0;
            return std::stoi(cell);
        };

        double width   = readDouble();
        double height  = readDouble();
        double posx    = readDouble();
        double posy    = readDouble();
        double speedx  = readDouble();
        double speedy  = readDouble();
        double mass    = readDouble();
        double friction= readDouble();
        double elast   = readDouble();
        int grav       = readInt();
        int trail      = readInt();
        int fixedFlag  = readInt();
        int cr         = readInt();
        int cg         = readInt();
        int cb         = readInt();

        Color color = {(unsigned char)cr, (unsigned char)cg, (unsigned char)cb, 255};
        Vector2 pos = vector(posx, posy);
        Vector2 vel = vector(speedx, speedy);

        Object* o = nullptr;

        if (width == 0.0) {
            double radius = height * 0.5;
            o = new PhysicsCircularObject(pos, vel, color, mass, radius, friction, trail != 0);
        } else {
            Vector2 sides = vector(width, height);
            o = new PhysicsRectangularObject(pos, vel, color, mass, sides, friction);
            o->leaveTrail = (trail != 0);
        }

        o->elasticity      = (float)elast;
        o->gravityAffected = (grav != 0);
        o->fixed           = (fixedFlag != 0);

        objectList.push_back(o);
    }

    lastVisitedObject = objectList.begin();
}

void OnFileLoaded(const char* path) {
    LoadSceneCSV(path);
}

// ---------------- UI & main ----------------

void debugPreInit(){
}

void UIPreInit(){
  UIList.push_back(new WindowScaleUI());
  UIList.push_back(new TimeScaleUI(0,24));
  UIList.push_back(new PauseUI(6,-32));
  UIList.push_back(new OwnershipUI(L("creator.myname"),-260,0));
  UIList.push_back(new VisualScaleUI(0,48));
  UIList.push_back(new TrailLifetimeUI(0,48));
}

int main(int argc, char** argv){
  lang = (argc > 1) ? argv[1] : lang;
  
  //debugPreInit();
  UIPreInit();
  StarsInit();
  
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, "Physics 10A");
  LoadUIFont();
  SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
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
    
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
      if(!PhysEditor::S().visible)
        PhysEditor::HandleLeftClickCreate(GetMousePosition());
      else PhysEditor::TryPickOrbitTarget(GetMousePosition());
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

    // --- CSV load/save ---
    if(IsKeyPressed(KEY_O)){
#ifdef __EMSCRIPTEN__
      // Web: open file picker, selected CSV goes to /opened.csv
      Web_OpenFileDialog();
#else
      // Desktop: load scene from scene.csv in working directory
      OnFileLoaded("scene.csv");
#endif
    }

#ifdef __EMSCRIPTEN__
    // Poll for CSV selected in browser
    if (Web_IsFileReady()) {
        Web_ConsumeFileReady();
        OnFileLoaded("/opened.csv");
    }
#endif

    // I = save scene to CSV
    if(IsKeyPressed(KEY_I)){
      const char* path = "scene.csv";
      SaveSceneCSV(path);
#ifdef __EMSCRIPTEN__
      Web_DownloadFile(path);
#endif
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
      else if(editingTrailLifetime) trailLifetime+=keyscale;
      else timeScale+=0.1*keyscale;
    }
    if(IsKeyPressed(KEY_MINUS)){
      if(visualScaling) windowVisualScale-=0.1*keyscale;
      else if(editingTrailLifetime) trailLifetime-=keyscale;
      else timeScale-=0.1*keyscale;
    }
    if(IsKeyPressed(KEY_C)){
      visualScaling=!visualScaling;
      if(visualScaling)editingTrailLifetime=false;
    }
    if(IsKeyPressed(KEY_T)){
      editingTrailLifetime=!editingTrailLifetime;
      if(editingTrailLifetime)visualScaling=false;
    }
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
  UnloadFont(uiFont);
}
