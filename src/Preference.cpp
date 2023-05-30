//
// Created by zhangfuwen on 2022/5/6.
//

#include "Preference.h"
#include "common_log.h"

void Preference::PreferenceFromDialog() {
    Glib::setenv("GSETTINGS_SCHEMA_DIR", "res", true);

    auto settings = Gio::Settings::create("fun.xjbcode.funterm", "/fun/xjbcode/funterm/");
    m_scrollLines = settings->get_int("scroll-lines");
    PreferenceDialog *matchDialog = new PreferenceDialog();

    auto builder = Gtk::Builder::create_from_file(RES_FILE_DIR "/preference_dialog.glade");
#include "preference_dialog.ui.h"

    std::vector<std::string> colorSchemeFileNames;
    if (handycpp::file::is_dir_exist(colorSchemeDir.c_str())) {
        handycpp::file::listFiles(colorSchemeDir, colorSchemeFileNames);
    }
    but_save->signal_clicked().connect([this]() { SavePrefs(); });

    textScrollLines->set_text(Glib::ustring::compose("%1", m_scrollLines));

    pref_custom_font_checked->set_active(this->font_desc != nullptr);
    if (this->font_desc != nullptr) {
        gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(pref_custom_font->gobj()), this->font_desc);
    }

    pref_custom_font->signal_font_set().connect([this, matchDialog, pref_custom_font, pref_custom_font_checked]() {
        matchDialog->m_changes.font_changed = true;
        //        Gtk::FontChooser chooser(*pref_custom_font);
        auto desc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(pref_custom_font->gobj()));
        auto size = gtk_font_chooser_get_font_size(GTK_FONT_CHOOSER(pref_custom_font->gobj()));
        if (pref_custom_font_checked->get_active()) {
            FUN_DEBUG("font desc %p ", desc);
            if (desc) {
                FUN_DEBUG("font desc str:%s", pango_font_description_to_string(desc));
            }
            this->font_desc = desc;
            this->font_size = size / 1000;
        }
        FUN_DEBUG("font %d", size);
    });

    for (auto file : colorSchemeFileNames) {
        cs_combo->append(file);
    }

    cs_combo->signal_changed().connect([matchDialog]() { matchDialog->m_changes.cs_changed = true; });
    matchDialog->get_content_area()->pack_start(*win1);
    matchDialog->set_title("Preference");
    matchDialog->show_all();
    matchDialog->signal_response().connect([this, matchDialog, cs_combo](int id) {
        FUN_DEBUG("dialog close %d", matchDialog->m_changes.cs_changed);
        if (matchDialog->m_changes.cs_changed) {
            auto fileName = cs_combo->get_active_text();
            m_currentColorScheme = Parse(fileName);
        }
        onChanged(*this, matchDialog->m_changes);
    });
}
void Preference::SavePrefs() {
    auto text = ToString();
    if (!handycpp::file::is_dir_exist(configDir.c_str())) {
        handycpp::file::create_dir(configDir.c_str(), true);
    }
    handycpp::file::saveFile(text.data(), text.size(), configDir + prefFile);
    Glib::setenv("GSETTINGS_SCHEMA_DIR", "res", true);
    auto settings = Gio::Settings::create("fun.xjbcode.funterm");
    settings->set_int("scroll-lines", m_scrollLines);
}

std::string Preference::ToString() {
    std::string s;
    s += pango_font_description_to_string(this->font_desc);
    return s;
}
void Preference::FromString(std::string fileContext) {
    this->font_desc = pango_font_description_from_string(fileContext.c_str());
}
