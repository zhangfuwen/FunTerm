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
#include "Preference.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class TerminalSession;
extern TerminalSession *lastFocusTerm;

const HighlightStyle defaultStyle = {
    .back= 11,
    .backmask = 0xffffffff,
};
const std::vector<HighlightStyle> styles = {
    HighlightStyle {
        .back= 11,
        .backmask = 0xffffffff,

    },
    HighlightStyle {
        .back= 04,
        .backmask = 0xffffffff,

    },
    HighlightStyle {
        .back= 0x04,
        .backmask = 0xffffffff,

    },
    HighlightStyle {
        .back= 0x03,
        .backmask = 0xffffffff,

    },
    HighlightStyle {
        .back= 0x09,
        .backmask = 0xffffffff,

    },
    HighlightStyle {
        .back= 0x05,
        .backmask = 0xffffffff,
    },
};

inline HighlightStyle getRandomStyle() {
    static int i = 0;
    i++;
    if(i>= styles.size()) {
        i=0;
    }
    return styles[i];
}


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

class ContextMenu : private Gtk::Menu {
public:
    void Add(std::unique_ptr<Gtk::MenuItem> && item) {
        this->add(*item);
        m_items.push_back(std::move(item));
    }

    void Clear() {
        m_items.clear();
    }

    void Show(GdkEvent * ev) {
        this->show_all();
        this->popup_at_pointer((GdkEvent*)ev);
    }
    bool Empty() {
        return m_items.empty();
    }

private:
    std::vector<std::unique_ptr<Gtk::MenuItem>> m_items;
};

class SearchBox : private Gtk::Box {
public:
    struct SearchStatus {
        Glib::ustring text;
        bool caseSensitive;
        bool wholeWord;
        bool regexSearch;
    };

    operator Gtk::Box&() {
        return *this;
    }

    SearchStatus GetStatus() {
        SearchStatus status;
        status.wholeWord = wholeWord.get_active();
        status.regexSearch = regexSearch.get_active();
        status.caseSensitive = caseSensitiveSearch.get_active();
        status.text = search.get_text();
        return status;
    }

    auto signal_next_match() {
        return search.signal_next_match();
    }
    auto signal_search_changed() {
        return search.signal_search_changed();
    }
    auto signal_next_clicked() { return next.signal_clicked(); }
    auto signal_prev_clicked() { return prev.signal_clicked(); }
    auto signal_close_clicked() { return close.signal_clicked(); }

private:
    Gtk::SearchEntry search;
    Gtk::ToggleButton caseSensitiveSearch;
    Gtk::ToggleButton regexSearch;
    Gtk::ToggleButton wholeWord;
    Gtk::Button next;
    Gtk::Button prev;
    Gtk::Button close;

public:
    void SetText(const Glib::ustring& text) {
        search.set_text(text);
    }
    SearchBox() {
        caseSensitiveSearch.set_label("Cc");
        regexSearch.set_label(".*");
        wholeWord.set_label("W");
        caseSensitiveSearch.get_style_context()->add_class("circular");
        regexSearch.get_style_context()->add_class("circular");
        wholeWord.get_style_context()->add_class("circular");
        caseSensitiveSearch.set_relief(Gtk::RELIEF_NONE);
        regexSearch.set_relief(Gtk::RELIEF_NONE);
        wholeWord.set_relief(Gtk::RELIEF_NONE);
        caseSensitiveSearch.set_tooltip_text("Case-sensitive Search");
        regexSearch.set_tooltip_text("Search with regular expresion");
        wholeWord.set_tooltip_text("Search with whole word");

        next.set_image_from_icon_name("go-down-symbolic", Gtk::ICON_SIZE_MENU);
        prev.set_image_from_icon_name("go-up-symbolic", Gtk::ICON_SIZE_MENU);
        close.set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_MENU);
        next.set_relief(Gtk::RELIEF_NONE);
        prev.set_relief(Gtk::RELIEF_NONE);
        close.set_relief(Gtk::RELIEF_NONE);
        next.set_halign(Gtk::ALIGN_END);
        prev.set_halign(Gtk::ALIGN_END);
        close.set_halign(Gtk::ALIGN_END);
        pack_start(search);
        pack_start(caseSensitiveSearch);
        pack_start(wholeWord);
        pack_start(regexSearch);
        pack_end(close);
        pack_end(next);
        pack_end(prev);
        set_halign(Gtk::ALIGN_CENTER);
        set_valign(Gtk::ALIGN_CENTER);
        next.set_can_focus(true);
    }

};
class TerminalSession : public Gtk::Paned {
public:
    PanedContainer *GetParent() { return dynamic_cast<PanedContainer *>(this->get_parent()); }
    Tab *GetTab() { return m_tab; }
    explicit TerminalSession(Tab *tab, std::string workingDir = "");
    void InitTitleBox();
    void InitTerminal();
    void InitSearchBox();
    void InitMatchBox();
    void ShowMatchDialog();

    static void dualSplit(Gtk::Paned &paned, int w = -1, int h = -1);

    void Split(TerminalSession *new_sess, Gtk::Orientation orient = Gtk::ORIENTATION_HORIZONTAL);
    void ToggleSearch(const Glib::ustring &text = "", bool showOnly = false);
    void ToggleMatch(const Glib::ustring &text = "", bool showOnly = false);
    bool OnTitleDoubleClicked(GdkEventButton *ev, TitleEntry * label);
    void HideTitle();
    void ShowTitle();
    void CopyText();
    void PasteText();
    void UpdatePreference(const Preference & pref, Changes changes);

    ~TerminalSession() override {}

private:
    int m_id = 0;
    std::string workingDir = "";

    // root tab
    Tab *m_tab = nullptr;

    // topbar
    bool m_showTitle = true;
    Gtk::Box *m_topBar = nullptr; // place holder
    Gtk::Box *titleBox = nullptr;
    std::unique_ptr<SearchBox> searchBox = nullptr;
    Gtk::Box *m_topBarOldBox = nullptr;
    Gtk::Box *m_matchBox = nullptr;
    bool m_highlightMatch = true;
    VteRegex  * searchRegex = nullptr;
    std::vector<RegexMatch> matchRegexes;

    // bottombar
    Gtk::Box *m_bottomBar = nullptr;
    Gtk::Widget *vte = nullptr;
    VteTerminal  *m_terminal = nullptr;
    Gtk::VScrollbar * scroll = nullptr;

    friend gboolean key_press_cb ( GtkWidget* self,
        GdkEventKey *event,
        gpointer user_data
        );
    friend void selection_changed(VteTerminal *term, TerminalSession *sess);

    void RefreshMatch();
    void AddHighlight(const RegexMatch &m, const HighlightStyle & style = defaultStyle) const;
    void AddHighlight(const Glib::ustring &text);
    std::unordered_set<std::string> m_highlightedTexts;

    Glib::ustring m_selectionText;
    ContextMenu  m_popupMenu;

    std::unique_ptr<Preference> m_pref=nullptr;

    const std::string configDir = "~/.config/funterm/";
    const std::string prefFile = "pref.txt";


    void ShowContextMenu(const GdkEventButton *ev);
};

#endif // FUNTERM_TERMINALSESSION_H
