//
// Created by zhangfuwen on 2022/5/6.
//

#ifndef FUNTERM_PREFERENCE_H
#define FUNTERM_PREFERENCE_H

#include <gtkmm.h>
#include <handycpp/file.h>
#include <handycpp/string.h>

#include <utility>
#include "common_log.h"

struct ColorScheme {
    GdkRGBA bg, fg, cursor;
    bool hasBg = false, hasFg = false, hasCursor = false;
    GdkRGBA palette[16];
    int numColor = 0;
};

inline std::optional<ColorScheme> Parse(std::string filePath) {
    ColorScheme cs;
    std::unordered_map<std::string, std::string> m;
    FUN_DEBUG("filepath %s", filePath.c_str());
    handycpp::file::for_each_line(filePath, [&m](int n, std::string line) {
        FUN_DEBUG("%s", line.c_str());
        if(handycpp::string::Contains(line, "=")) {
            using namespace handycpp::string::pipe_operator;
            auto tokens = line | splitby("=");
            if(tokens.size() != 2) {
                return 0;
            } else {
                m[tokens[0]] = tokens[1];
            }
        }
        return 0;
    });
    for(const auto & [a, b] : m) {
        FUN_DEBUG("%s:%s", a.c_str(), b.c_str());
    }
    for(int i = 0; i<8;i++) {
        auto key = "ColorPalette" + std::to_string(i+1);
        if(!m.count(key)) {
            FUN_ERROR("error");
            return {};
        }
        if(auto ret = gdk_rgba_parse(&cs.palette[i], m[key].c_str()); !ret) {
            FUN_ERROR("error");
            return {};
        }
    }
    cs.numColor = 8;

    // optionally parse 16 colors
    for(int i = 8; i<16;i++) {
        auto key = "ColorPalette" + std::to_string(i+1);
        if(!m.count(key)) {
            break;
        }
        if(auto ret = gdk_rgba_parse(&cs.palette[i], m[key].c_str()); !ret) {
            break;
        }
        if(i==15) {
            cs.numColor = 16;
        }
    }

    // optional colors
    if(m.count("ColorCursor") && gdk_rgba_parse(&cs.cursor, m["ColorCurosr"].c_str())) {
        cs.hasCursor = true;
    }

    if(m.count("ColorBackground") && gdk_rgba_parse(&cs.bg, m["ColorBackground"].c_str())) {
        cs.hasBg = true;
    }
    if(m.count("ColorForeground") && gdk_rgba_parse(&cs.fg, m["ColorForeground"].c_str())) {
        cs.hasFg = true;
    }
    return cs;
}

struct Changes {
    bool cs_changed = false;
    bool font_changed = false;
};

class PreferenceDialog : public Gtk::Dialog {
public:
    Changes m_changes;
};

class Preference {
public:
    Preference(std::function<void(const Preference&, Changes changes)> onChanged) {
        this->onChanged = std::move(onChanged);
    }
    void PreferenceFromDialog();
    std::optional<ColorScheme> GetColorscheme() const {
        return m_currentColorScheme;
    }
    std::string ToString();
    void FromString(std::string fileContext);

    std::optional<ColorScheme> m_currentColorScheme{};
    PangoFontDescription *font_name = nullptr;
    int font_size;
    std::function<void(const Preference&, Changes)> onChanged;

    const std::string colorschemeDir = RES_FILE_DIR "/colorschemes/";
    const std::string configDir = "~/.config/funterm/";
    const std::string prefFile = "pref.txt";
};

#endif // FUNTERM_PREFERENCE_H
