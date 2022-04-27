//
// Created by zhangfuwen on 2022/4/20.
//

#ifndef FUNTERM_TERMINALSESSION_H
#define FUNTERM_TERMINALSESSION_H

#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/window.h>

#include <vte/vte.h>

#include "common.h"
#include "common_log.h"

#include "configor/json.hpp"

#include "PanedContainer.h"
#include "Tab.h"
#include "TitleEntry.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class TerminalSession;
extern TerminalSession *lastFocusTerm;

gboolean key_press_cb (
    GtkWidget* self,
    GdkEventKey *event,
    gpointer user_data
);

struct RegexMatch {
    RegexMatch(std::string t) : text(t) {}
    ~RegexMatch() {
        if(pattern) {
            vte_regex_unref(pattern);
        }
    }
    void CompileForMatch();
    void CompileForSearch();
    std::string text = "";
    bool caseSensitive = false;
    bool regex = false;
    bool wholeWord = false;
    VteRegex * pattern = nullptr;
};

class TerminalSession : public Gtk::Paned {
public:
    PanedContainer *GetParent() { return dynamic_cast<PanedContainer *>(this->get_parent()); }
    Tab *GetTab() { return m_tab; }
    explicit TerminalSession(Tab *tab);
    void InitTitleBox();
    void InitTerminal();
    void InitSearchBox();
    void InitMatchBox();
    void ShowMatchDialog();

    static void dualSplit(Gtk::Paned &paned, int w = -1, int h = -1);

    void Split(TerminalSession *new_sess, Gtk::Orientation orient = Gtk::ORIENTATION_HORIZONTAL);
    void ToggleSearch();
    void ToggleMatch();
    bool OnTitleDoubleClicked(GdkEventButton *ev, TitleEntry * label);
    void HideTitle();
    void ShowTitle();

    ~TerminalSession() override {}

private:
    int m_id = 0;

    // root tab
    Tab *m_tab = nullptr;

    // topbar
    bool m_showTitle = true;
    Gtk::Box *m_topBar = nullptr; // place holder
    Gtk::Box *titleBox = nullptr;
    Gtk::Box *searchBox = nullptr;
    Gtk::Box *m_matchBox = nullptr;
    bool m_highlightMatch = true;
    VteRegex  * searchRegex = nullptr;
    std::vector<RegexMatch> matchRegexes;

    // bottombar
    Gtk::Box *m_bottomBar = nullptr;
    Gtk::Widget *vte = nullptr;
    VteTerminal  * terminal = nullptr;
    Gtk::VScrollbar * scroll = nullptr;

    friend gboolean key_press_cb ( GtkWidget* self,
        GdkEventKey *event,
        gpointer user_data
        );
    void RefreshMatch();
    void AddHighlight(const RegexMatch &m) const;
};

#endif // FUNTERM_TERMINALSESSION_H
