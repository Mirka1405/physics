#pragma once
#include "raylib.h"
#include "physics_engine.hpp"
#include "sstream"

extern std::list<Object*> objectList;
extern double windowScale;
extern Vector2 windowPos;

extern Vector2 vector(double x, double y);
extern double distance(const Vector2& a, const Vector2& b);

namespace PhysEditor {

// Right-click opens editor. If right-clicked empty space, edit template for new objects.
// Left-click uses the template to create a new PhysicsCircularObject at the click position.

struct TemplateProps {
    // Only used when editing template (for new creations)
    double mass = 1e3;
    float  radius = 10.0f;
    double friction = 0.02;
    float  elasticity = 0.5f;
    bool   gravityAffected = false;
    Color  color = {200, 220, 255, 255};
    Vector2 speed = {0, 0};
};

struct State {
    bool visible = false;
    bool editingTemplate = true;    // true => editing template; false => editing selected object
    Object* selected = nullptr;     // selected object when editingTemplate == false
    TemplateProps tpl{};            // editable template for new objects
    // UI
    Rectangle panel{20, 60, 360, 360};
    bool dragging = false;
    Vector2 dragOffset{0,0};
};

static State& S() { static State s; return s; }

// ----------------- Small UI helpers -----------------
static bool PointInRect(Vector2 p, const Rectangle& r){
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height;
}

static bool DrawBtn(const Rectangle& r, const char* txt){
    DrawRectangleRec(r, (Color){40,40,40,255});
    DrawRectangleLinesEx(r, 1.0f, (Color){200,200,200,255});
    int tw = MeasureText(txt, 16);
    DrawText(txt, (int)(r.x + (r.width - tw)/2), (int)(r.y + 2), 16, WHITE);
    return (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), r));
}

static void DrawValueRowF(const char* name, float& val, float step, float x, float& y){
    DrawText(name, (int)x, (int)y, 18, WHITE);
    std::ostringstream oss; oss.setf(std::ios::fixed); oss.precision(4); oss << val;
    DrawText(oss.str().c_str(), (int)(x+160), (int)y, 18, (Color){180,220,255,255});
    Rectangle minusR = {x+260, y, 24, 24};
    Rectangle plusR  = {x+290, y, 24, 24};
    double keyscale = 1;
    if(IsKeyDown(KEY_LEFT_ALT))keyscale*=10000;
    if(IsKeyDown(KEY_LEFT_SHIFT))keyscale*=10;
    if(IsKeyDown(KEY_LEFT_CONTROL))keyscale*=100;
    if(IsKeyDown(KEY_RIGHT_CONTROL))keyscale*=0.01;
    if(IsKeyDown(KEY_LEFT_SHIFT))keyscale*=0.1;
    if(IsKeyDown(KEY_LEFT_ALT))keyscale*=0.0001;
    if (DrawBtn(minusR, "-")) val -= step*keyscale;
    if (DrawBtn(plusR,  "+")) val += step*keyscale;
    y += 28;
}

static void DrawValueRowD(const char* name, double& val, double step, float x, float& y){
    DrawText(name, (int)x, (int)y, 18, WHITE);
    std::ostringstream oss; oss.setf(std::ios::fixed); oss.precision(6); oss << val;
    DrawText(oss.str().c_str(), (int)(x+160), (int)y, 18, (Color){180,220,255,255});
    Rectangle minusR = {x+260, y, 24, 24};
    Rectangle plusR  = {x+290, y, 24, 24};
    if (DrawBtn(minusR, "-")) val -= step;
    if (DrawBtn(plusR,  "+")) val += step;
    y += 28;
}

static void DrawCheckRow(const char* name, bool& b, float x, float& y){
    DrawText(name, (int)x, (int)y, 18, WHITE);
    Rectangle r = {x+160, y, 24, 24};
    DrawRectangleRec(r, b ? (Color){80,160,80,255} : (Color){60,60,60,255});
    DrawRectangleLinesEx(r, 1, (Color){200,200,200,255});
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), r)) b = !b;
    y += 28;
}

static void DrawRGBRow(const char* name, unsigned char& ch, float x, float& y){
    float fx = (float)ch;
    DrawText(name, (int)x, (int)y, 18, WHITE);
    Rectangle bar = {x+160, y+8, 160, 8};
    DrawRectangleRec(bar, (Color){90,90,90,255});
    float knobX = bar.x + (fx/255.0f)*bar.width - 4;
    Rectangle knob = {knobX, bar.y-4, 8, 16};
    DrawRectangleRec(knob, (Color){200,200,200,255});
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), (Rectangle){bar.x-6, bar.y-6, bar.width+12, bar.height+12})){
        float t = (GetMousePosition().x - bar.x)/bar.width;
        t = std::clamp(t, 0.0f, 1.0f);
        ch = (unsigned char)std::round(t*255.0f);
    }
    y += 28;
}

// ----------------- Picking -----------------
static Object* PickObjectAtScreen(Vector2 mp){
    Vector2 world = mp*windowScale + windowPos;
    Object* best = nullptr;
    double bestKey = 1e300;
    for (auto* o : objectList){
        if (auto c = dynamic_cast<CircularObject*>(o)){
            double d = distance(world, c->pos);
            if (d <= c->radius){
                if (c->radius < bestKey){ bestKey = c->radius; best = o; }
            }
        } else if (auto r = dynamic_cast<RectangularObject*>(o)){
            if (CheckCollisionPointRec(world, r->rect())){
                double key = (double)r->sides.x * (double)r->sides.y; // smaller first
                if (key < bestKey){ bestKey = key; best = o; }
            }
        }
    }
    return best;
}

// ----------------- Public API -----------------
inline void Init(){ /* nothing yet */ }

inline void OnRightClick(Vector2 mouseScreen){
    State& st = S();
    Object* picked = PickObjectAtScreen(mouseScreen);
    if (picked){
        st.selected = picked;
        st.editingTemplate = false;
        st.visible = true;
    } else {
        st.selected = nullptr;
        st.editingTemplate = true;
        st.visible = true;
    }
}

// Call in your LMB handler to create from the template at mouse position
inline bool HandleLeftClickCreate(Vector2 mouseScreen){
    State& st = S();
    Vector2 world = mouseScreen*windowScale + windowPos;
    auto* obj = new PhysicsCircularObject(
        world,
        st.tpl.speed,
        st.tpl.color,
        st.tpl.mass,
        st.tpl.radius,
        st.tpl.friction
    );
    obj->elasticity = st.tpl.elasticity;
    obj->gravityAffected = st.tpl.gravityAffected;
    objectList.push_back(obj);
    return true;
}

// Draw header + handle dragging
static void DrawPanelFrame(State& st){
    Rectangle& panel = st.panel;
    DrawRectangleRec(panel, (Color){20,20,28,240});
    DrawRectangleLinesEx(panel, 2, (Color){140,140,160,255});

    Rectangle title = {panel.x, panel.y, panel.width, 28};
    DrawRectangleRec(title, (Color){30,30,42,255});
    const char* titleTxt = st.editingTemplate ? "New Object Template" : "Edit Object (live)";
    DrawText(titleTxt, (int)(panel.x+10), (int)(panel.y+6), 16, WHITE);

    // Dragging
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), title)){
        st.dragging = true; st.dragOffset = {GetMousePosition().x - panel.x, GetMousePosition().y - panel.y};
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) st.dragging = false;
    if (st.dragging){
        Vector2 mp = GetMousePosition();
        panel.x = mp.x - st.dragOffset.x;
        panel.y = mp.y - st.dragOffset.y;
    }
}

// Draw panel and edit either template or live object
inline void Draw(){
    State& st = S();
    if (!st.visible) return;

    DrawPanelFrame(st);

    float x = st.panel.x + 12;
    float y = st.panel.y + 40;

    // Close button
    Rectangle closeR = { st.panel.x + st.panel.width - 86, st.panel.y + st.panel.height - 36, 70, 26 };
    if (DrawBtn(closeR, "Close")) st.visible = false;

    if (st.editingTemplate){
        DrawValueRowD("Mass",     st.tpl.mass,     1e3, x, y);
        DrawValueRowD("Friction", st.tpl.friction, 0.01, x, y);
        DrawValueRowF("Elastic.", st.tpl.elasticity, 0.05f, x, y);
        DrawCheckRow ("Gravity",  st.tpl.gravityAffected, x, y);
        DrawValueRowF("Speed X",  st.tpl.speed.x, 10.0f, x, y);
        DrawValueRowF("Speed Y",  st.tpl.speed.y, 10.0f, x, y);
        DrawValueRowF("Radius",   st.tpl.radius,  1.0f, x, y);
        DrawRGBRow   ("Color R",  st.tpl.color.r, x, y);
        DrawRGBRow   ("Color G",  st.tpl.color.g, x, y);
        DrawRGBRow   ("Color B",  st.tpl.color.b, x, y);
        // preview
        DrawCircle(st.panel.x + st.panel.width - 40, st.panel.y + 40, 10, st.tpl.color);
    } else {
        // Live-bound editing of the actual object fields
        Object* o = st.selected;
        if (!o || o->shouldRemove) { st.visible = false; st.selected = nullptr; return; }

        DrawValueRowD("Mass",     o->mass,            1e3, x, y);
        DrawValueRowD("Friction", o->frictionFactor,  0.01, x, y);
        DrawValueRowF("Elastic.", o->elasticity,      0.05f, x, y);
        DrawCheckRow ("Gravity",  o->gravityAffected, x, y);
        DrawValueRowF("Speed X",  o->speed.x,         10.0f, x, y);
        DrawValueRowF("Speed Y",  o->speed.y,         10.0f, x, y);

        if (auto* c = dynamic_cast<CircularObject*>(o)){
            DrawValueRowD("Radius", c->radius, 1.0f, x, y);
        }
        DrawRGBRow("Color R", o->color.r, x, y);
        DrawRGBRow("Color G", o->color.g, x, y);
        DrawRGBRow("Color B", o->color.b, x, y);

        // Small preview dot using current color
        DrawCircle(st.panel.x + st.panel.width - 40, st.panel.y + 40, 10, o->color);

        Rectangle delR = { x, st.panel.y + st.panel.height - 36, 70, 26 };
        if (DrawBtn(delR, "Delete")) { o->shouldRemove = true; st.visible = false; st.selected = nullptr; }
    }
}

} // namespace PhysEditor
