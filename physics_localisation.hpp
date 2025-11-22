#pragma once
#include "raylib.h"
#include "physics_variables.hpp"
#include <map>
#include <string>

static std::map<std::string, std::map<std::string, std::string>> STR = {

    // -------- Editor panel base labels --------
    { "editor.title.template", {
        {"en", "New Object Template"},
        {"ru", "Шаблон нового объекта"}
    }},
    { "editor.title.object", {
        {"en", "Edit Object"},
        {"ru", "Редактирование объекта"}
    }},

    { "editor.mass", {
        {"en", "Mass"},
        {"ru", "Масса"}
    }},
    { "editor.friction", {
        {"en", "Friction"},
        {"ru", "Коэфф. трения"}
    }},
    { "editor.elasticity", {
        {"en", "Elasticity"},
        {"ru", "Упругость"}
    }},
    { "editor.gravity", {
        {"en", "Gravity"},
        {"ru", "Гравитация"}
    }},
    { "editor.fixed", {
        {"en", "Fixed"},
        {"ru", "Статичность"}
    }},
    { "editor.speed_x", {
        {"en", "Speed X"},
        {"ru", "Скорость по X"}
    }},
    { "editor.speed_y", {
        {"en", "Speed Y"},
        {"ru", "Скорость по Y"}
    }},
    { "editor.trail", {
        {"en", "Trail"},
        {"ru", "След"}
    }},
    { "editor.radius", {
        {"en", "Radius"},
        {"ru", "Радиус"}
    }},
    { "editor.color_r", {
        {"en", "Red"},
        {"ru", "Красный"}
    }},
    { "editor.color_g", {
        {"en", "Green"},
        {"ru", "Зеленый"}
    }},
    { "editor.color_b", {
        {"en", "Blue"},
        {"ru", "Синий"}
    }},

    // -------- Buttons --------
    { "btn.close", {
        {"en", "Close"},
        {"ru", "Закрыть"}
    }},
    { "btn.delete", {
        {"en", "Delete"},
        {"ru", "Удалить"}
    }},
    { "btn.orbit", {
        {"en", "Orbit..."},
        {"ru", "Орбита..."}
    }},

    // -------- Small UI elements --------
    { "ui.scale", {
        {"en", "Scale: "},
        {"ru", "Масштаб: "}
    }},
    { "ui.time", {
        {"en", "Time: "},
        {"ru", "Время: "}
    }},
    { "ui.trail_lifetime", {
        {"en", "Trail lifetime: "},
        {"ru", "Время следа: "}
    }},
    { "ui.visual_scaling", {
        {"en", "Visual scaling: "},
        {"ru", "Визуальный масштаб: "}
    }},
    { "ui.madeby", {
        {"en", "Made by "},
        {"ru", "Создал "}
    }},

    { "creator.myname", {
        {"en", "Miron Samokhvalov"},
        {"ru", "Мирон Самохвалов"}
    }},
};
std::string L(const std::string& key, const std::string& l) {
    if (STR.count(key) && STR.at(key).count(l))
        return STR.at(key).at(l);
    return key;
}
std::string L(const std::string& key) {
    if (STR.count(key) && STR.at(key).count(lang))
        return STR.at(key).at(lang);
    return key+lang;
}
void LoadUIFont() {
    const int FONT_SIZE = 24;
    int codepoints[351];
    int count = 0;

    // Basic ASCII
    for (int c = 32; c <= 126; c++) codepoints[count++] = c;

    // Cyrillic (0x0400–0x04FF)
    for (int c = 0x0400; c <= 0x04FF; c++) codepoints[count++] = c;

    uiFont = LoadFontEx("UbuntuMono-Regular.ttf",
                        FONT_SIZE,
                        codepoints,
                        count);
}