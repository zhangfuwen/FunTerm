//
// Created by zhangfuwen on 2022/4/20.
//
#define PCRE2_CODE_UNIT_WIDTH 0
#include "TerminalSession.h"
#include <glib-object.h>
#include <pcre2.h>
#include <vte/vte.h>

#include <utility>

TerminalSession::TerminalSession(Tab *tab, std::string wd)
    : Gtk::Paned(Gtk::ORIENTATION_VERTICAL)
    , workingDir(std::move(wd))
    , m_tab(tab) {
    static int id = 0;
    id++;
    m_id = id;
    Gtk::Widget::set_name(_("Sess-") + std::to_string(id));
    FUN_INFO("creating session %d", id);

    InitTerminal();
    InitTitleBox();
    InitSearchBox();
    InitMatchBox();

    // setup topbar
    m_topBar = Glib::RefPtr<Gtk::Box>(Gtk::make_managed<Gtk::Box>());
    m_topBar->pack_start(*titleBox.get());
    m_topBar->get_style_context()->add_class("top_bar");
    Gtk::Paned::add1(*m_topBar.get());
    Gtk::Paned::set_position(30);
    set_orientation(Gtk::ORIENTATION_VERTICAL);
    show_all();

    // setup content area
    m_bottomBar = Glib::RefPtr<Gtk::Box>(Gtk::make_managed<Gtk::Box>());
    scroll = Glib::RefPtr(Gtk::make_managed<Gtk::VScrollbar>());
    m_bottomBar->add(*vte);
    m_bottomBar->add(*scroll.get());
    add2(*m_bottomBar.get());
    m_bottomBar->signal_button_press_event().connect([](GdkEventButton *ev) { return true; });

    Gtk::Paned::set_focus_on_click(true);
    Gtk::Paned::set_can_focus(true);
    Gtk::Paned::set_visible(true);
    Gtk::Paned::set_child_visible(true);
    Gtk::Paned::signal_focus_in_event().connect([](bool) {
        FUN_INFO("panned focus in");
        return true;
    });
    set_focus_child(*m_bottomBar.get());
    m_bottomBar->set_focus_child(*vte);
    signal_size_allocate().connect([this](const Gdk::Rectangle &) { this->set_position(30); });

    m_pref = std::make_unique<Preference>(
        [this](const Preference &pref, Changes changes) { this->UpdatePreference(pref, changes); });
    if (handycpp::file::is_file_exist((configDir + prefFile).c_str())) {
        FUN_INFO("read preference file %s", (configDir + prefFile).c_str());
        std::string text = handycpp::file::readText(configDir + prefFile);
        m_pref->FromString(text);
        Changes changes;
        changes.cs_changed = false;
        changes.font_changed = true;
        UpdatePreference(*m_pref, changes);
    }
}

void TerminalSession::InitMatchBox() {
    m_matchBox = Glib::RefPtr<Gtk::Box>(Gtk::make_managed<Gtk::Box>());
    auto checkMatch = Gtk::make_managed<Gtk::CheckButton>();
    checkMatch->set_tooltip_text(_("toggle highlight matches"));
    checkMatch->signal_toggled().connect([this, checkMatch]() {
        FUN_DEBUG("match toggle %d", checkMatch->get_active());
        m_highlightMatch = checkMatch->get_active();
        RefreshMatch();
    });
    auto label = Gtk::make_managed<Gtk::Label>("Highlight");
    label->set_margin_left(8);

    auto butConfig = Gtk::make_managed<Gtk::Button>();
    butConfig->set_image_from_icon_name("emblem-system-symbolic", Gtk::ICON_SIZE_MENU);
    butConfig->set_tooltip_text(_("Manage highlight words"));
    butConfig->signal_clicked().connect([this]() { ShowMatchDialog(); });
    butConfig->set_relief(Gtk::RELIEF_NONE);

    m_matchBox->pack_start(*checkMatch);
    m_matchBox->pack_start(*label);
    m_matchBox->pack_end(*butConfig);
    m_matchBox->show_all();
    m_matchBox->set_halign(Gtk::ALIGN_CENTER);
}
void TerminalSession::RefreshMatch() {
    FUN_DEBUG("highlight: %d", m_highlightMatch);
    if (m_highlightMatch) {
        vte_terminal_match_remove_all(m_terminal);
        vte_terminal_highlight_clear(m_terminal);
        for (auto &m : matchRegexes) {
            m.CompileForMatch();
            FUN_DEBUG("add_regex %s, %p", m.text.c_str(), m.pattern);
            vte_terminal_match_add_regex(m_terminal, m.pattern, 0);
            AddHighlight(m);
        }

    } else {
        vte_terminal_match_remove_all(m_terminal);
    }
}
void TerminalSession::AddHighlight(const RegexMatch &m, const HighlightStyle &style) const {

    std::string s = m.text;
    HighlightPattern pat = {};
    if (!m.caseSensitive) {
        pat.regex_flags |= PCRE2_CASELESS;
    } else {
        pat.regex_flags = 0;
    }
    if (!m.regex) { // not regex search
        s = g_regex_escape_string(m.text.c_str(), (gint) m.text.size());
    }
    if (m.wholeWord) {
        using namespace std::string_literals;
        s = "\\b" + s + "\\b";
    }
    pat.pattern = s.c_str();
    pat.style = style;
    pat.regex = true;
    vte_terminal_highlight_add_pattern(m_terminal, pat);
}

void TerminalSession::InitSearchBox() {
    searchBox = std::make_unique<SearchBox>();
    searchBox->signal_next_match().connect([this]() {
        auto status = searchBox->GetStatus();
        auto const &text = status.text;
        auto regex = vte_regex_new_for_search(text.c_str(), (gssize) text.length() + 1, 0, nullptr);
        vte_terminal_search_set_regex(m_terminal, regex, 0);
        RegexMatch m(text);
        m.caseSensitive = status.caseSensitive;
        m.regex = status.regexSearch;
        m.wholeWord = status.wholeWord;
        AddHighlight(m);
    });
    searchBox->signal_search_changed().connect([this]() {
        auto status = searchBox->GetStatus();
        GError *error = nullptr;
        auto text = status.text;
        guint32 compile_flags;
        compile_flags = PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE;
        if (!status.caseSensitive) {
            compile_flags |= PCRE2_CASELESS;
        }
        if (!status.regexSearch) { // not regex search
            text = g_regex_escape_string(text.c_str(), (gint) text.size());
        }
        if (status.wholeWord) {
            text = "\\b" + text + "\\b";
        }

        if (searchRegex != nullptr) {
            vte_regex_unref(searchRegex);
        }
        searchRegex = vte_regex_new_for_search(text.c_str(), text.size(), compile_flags, &error);
        if (searchRegex != nullptr) {
            vte_terminal_search_set_regex(m_terminal, searchRegex, 0);
            if (!vte_terminal_search_find_next(m_terminal)) {
                vte_terminal_search_find_previous(m_terminal);
            }
        } else {
            FUN_ERROR("regex error: %s", vte_regex_error_quark());
        }
        FUN_DEBUG("search changed: %s", text.c_str());
    });
    searchBox->signal_next_clicked().connect([this]() {
        FUN_DEBUG("search next");
        vte_terminal_search_find_next(m_terminal);
    });
    searchBox->signal_prev_clicked().connect([this]() {
        FUN_DEBUG("search prev");
        vte_terminal_search_find_previous(m_terminal);
    });

    searchBox->signal_close_clicked().connect([this]() { this->ToggleSearch(); });
}

void TerminalSession::ToggleMatch([[maybe_unused]] const Glib::ustring &text, [[maybe_unused]] bool showOnly) {
    if (!m_topBar->get_children().empty() && m_topBar->get_children()[0] != m_matchBox.get()) {
        m_topBar->remove(*titleBox.get());
        m_topBar->pack_start(*m_matchBox.get());
        if (!m_showTitle && get_child1() == nullptr) {
            pack1(*m_topBar.get());
        }
    } else {
        if (!m_showTitle && get_child1() != nullptr) {
            remove(*m_topBar.get());
        }
        m_topBar->remove(*m_matchBox.get());
        m_topBar->pack_start(*titleBox.get());
    }
    m_topBar->show_all();
}

void TerminalSession::AddHighlight(const Glib::ustring &text) {
    RegexMatch m(text);
    m.caseSensitive = true;
    m.regex = false;
    m.wholeWord = false;
    m_highlightedTexts.insert(text);
    AddHighlight(m, getRandomStyle());
}

void TerminalSession::ToggleSearch(const Glib::ustring &text, bool showOnly) {
    if (!m_topBar->get_children().empty() && m_topBar->get_children()[0] != &(Gtk::Box &) *searchBox) {
        // currently not showing
        m_topBar->remove(*m_topBar->get_children()[0]);
        m_topBar->pack_start((Gtk::Box &) *searchBox);
        if (!text.empty()) {
            searchBox->SetText(text);
        }
        if (!m_showTitle && get_child1() == nullptr) {
            pack1(*m_topBar.get());
        }
    } else {
        // currently showing
        if (showOnly) {
            // not hiding
            if (!text.empty()) {
                searchBox->SetText(text);
            }
        } else {
            // hide
            if (!m_showTitle && get_child1() != nullptr) {
                remove(*m_topBar.get());
            }
            m_topBar->remove((Gtk::Box &) *searchBox);
            m_topBar->pack_start(*titleBox.get());
        }
    }
    m_topBar->show_all();
}
static void grid_set_header(Gtk::Grid *grid) {
    auto label1 = Gtk::make_managed<Gtk::Label>(_("pattern"));
    auto label2 = Gtk::make_managed<Gtk::Label>(_("Case Sensitive"));
    auto label3 = Gtk::make_managed<Gtk::Label>(_("Whole word"));
    auto label4 = Gtk::make_managed<Gtk::Label>(_("Regex"));
    auto label5 = Gtk::make_managed<Gtk::Label>(_("delete"));
    grid->attach(*label1, 0, 0);
    grid->attach(*label2, 1, 0);
    grid->attach(*label3, 2, 0);
    grid->attach(*label4, 3, 0);
    grid->attach(*label5, 4, 0);
    grid->show_all_children(true);
}

static void grid_add_row(
    Gtk::Grid *grid, int row_index, std::string const &key, bool caseSensitive, bool wholeWord, bool regex) {
    FUN_INFO("add row %d, %s, %d %d %d", row_index, key.c_str(), caseSensitive, wholeWord, regex);
    auto patternText1 = Gtk::manage(new Gtk::Entry());
    patternText1->set_editable();
    patternText1->set_text(key.c_str());
    patternText1->set_halign(Gtk::Align::ALIGN_CENTER);
    patternText1->set_valign(Gtk::Align::ALIGN_CENTER);
    grid->attach(*patternText1, 0, row_index, 1, 1);
    patternText1->show();

    auto checkCase = Gtk::manage(new Gtk::CheckButton());
    checkCase->set_active(caseSensitive);
    checkCase->set_halign(Gtk::Align::ALIGN_CENTER);
    checkCase->set_valign(Gtk::Align::ALIGN_CENTER);
    grid->attach(*checkCase, 1, row_index, 1, 1);
    checkCase->show();

    auto checkWholeWord = Gtk::manage(new Gtk::CheckButton());
    grid->attach(*checkWholeWord, 2, row_index, 1, 1);
    checkWholeWord->set_halign(Gtk::Align::ALIGN_CENTER);
    checkWholeWord->set_valign(Gtk::Align::ALIGN_CENTER);
    checkWholeWord->set_active(wholeWord);
    checkWholeWord->show_all();

    auto checkRegex = Gtk::manage(new Gtk::CheckButton());
    grid->attach(*checkRegex, 3, row_index, 1, 1);
    checkRegex->set_active(regex);
    checkRegex->set_halign(Gtk::Align::ALIGN_CENTER);
    checkRegex->set_valign(Gtk::Align::ALIGN_CENTER);
    checkRegex->show_all();

    auto delete_button = Gtk::manage(new Gtk::Button(_("delete")));
    grid->attach(*delete_button, 4, row_index);
    delete_button->set_halign(Gtk::Align::ALIGN_CENTER);
    delete_button->set_valign(Gtk::Align::ALIGN_START);
    delete_button->signal_clicked().connect(
        [patternText1, checkCase, checkRegex, checkWholeWord, delete_button, grid]() {
            grid->remove(*patternText1);
            grid->remove(*checkCase);
            grid->remove(*checkWholeWord);
            grid->remove(*checkRegex);
            grid->remove(*delete_button);
        },
        delete_button);

    delete_button->show();
}
static int grid_get_num_rows(Gtk::Grid *grid) {
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            return i;
        }
    }
    return 0;
}

void RegexMatch::CompileForSearch() {
    GError *error = nullptr;
    guint32 compile_flags;
    compile_flags = PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE;
    if (!caseSensitive) {
        compile_flags |= PCRE2_CASELESS;
    }
    if (!regex) { // not regex search
        text = g_regex_escape_string(text.c_str(), (gint) text.size());
    }
    if (wholeWord) {
        text = "\\b" + text + "\\b";
    }
    auto searchRegex = vte_regex_new_for_search(text.c_str(), text.size(), compile_flags, &error);
    if (searchRegex != nullptr) {
        pattern = searchRegex;
    }
}

void RegexMatch::CompileForMatch() {
    GError *error = nullptr;
    guint32 compile_flags;
    compile_flags = PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE;
    auto pattern_text = text;
    if (!caseSensitive) {
        compile_flags |= PCRE2_CASELESS;
    }
    if (!regex) { // not regex search
        pattern_text = g_regex_escape_string(pattern_text.c_str(), (gint) pattern_text.size());
    }
    if (wholeWord) {
        pattern_text = "\\b" + pattern_text + "\\b";
    }
    auto searchRegex = vte_regex_new_for_match(pattern_text.c_str(), pattern_text.size(), compile_flags, &error);
    if (searchRegex != nullptr) {
        pattern = searchRegex;
        vte_regex_ref(pattern);
    } else {
        FUN_DEBUG("failed to compile %s", pattern_text.c_str());
    }
}

void TerminalSession::ShowMatchDialog() {
    auto builder = Gtk::Builder::create_from_file(RES_FILE_DIR "/match_dialog.glade");
    Gtk::Box *box;
    builder->get_widget<Gtk::Box>("win1", box);

    Gtk::Grid *grid;
    builder->get_widget<Gtk::Grid>("match_dialog_grid", grid);
    Gtk::Button *but_new;
    builder->get_widget<Gtk::Button>("match_dialog_new_button", but_new);
    Gtk::Button *but_clear;
    builder->get_widget<Gtk::Button>("match_dialog_clear_button", but_clear);
    Gtk::Button *but_save;
    builder->get_widget<Gtk::Button>("match_dialog_save_button", but_save);
    Gtk::Button *but_highlight;
    builder->get_widget<Gtk::Button>("match_dialog_save_button", but_highlight);

    but_new->signal_clicked().connect([grid]() {
        FUN_INFO("button_clicked");
        auto num_rows = grid_get_num_rows(grid);
        FUN_DEBUG("num rows %d", num_rows);
        grid_add_row(grid, num_rows, "", false, false, false);
    });

    but_clear->signal_clicked().connect([grid]() {
        auto num_rows = grid_get_num_rows(grid);
        for (int i = 1; i < num_rows; i++) {
            // row 0 contains labels that should be kept
            // row 2 becomes row 1 when row 1 is deleted, so always remove row 1
            grid->remove_row(1);
        }
        // add an empty row for input
        grid_add_row(grid, 1, "", false, false, false);
    });

    but_save->signal_clicked().connect([this, grid]() {
        matchRegexes.clear();
        for (int i = 1; i < 1000; i++) {
            if (grid->get_child_at(0, i) == nullptr) {
                FUN_ERROR("child \"%d\" does not exist", i);
                break;
            }
            auto keyEntry = (Gtk::Entry *) grid->get_child_at(0, i);
            if (!keyEntry) {
                FUN_DEBUG("failed to get entry");
            }
            auto checkCase = (Gtk::CheckButton *) grid->get_child_at(1, i);
            auto checkWholeWord = (Gtk::CheckButton *) grid->get_child_at(2, i);
            auto checkRegex = (Gtk::CheckButton *) grid->get_child_at(3, i);
            auto keyString = keyEntry ? keyEntry->get_text() : "";
            if (!keyString.empty()) {
                RegexMatch regexMatch(keyString);
                regexMatch.caseSensitive = checkCase->get_active();
                FUN_DEBUG("caseSensitive %d", regexMatch.caseSensitive);
                regexMatch.wholeWord = checkWholeWord->get_active();
                regexMatch.regex = checkRegex->get_active();
                matchRegexes.emplace_back(regexMatch);
            }
        }
    });

    grid_set_header(grid);
    for (const auto &match : matchRegexes) {
        auto num_rows = grid_get_num_rows(grid);
        grid_add_row(grid, num_rows, match.text, match.caseSensitive, match.wholeWord, match.regex);
    }

    auto matchDialog = Gtk::make_managed<Gtk::Dialog>();
    matchDialog->get_content_area()->pack_start(*box);
    matchDialog->set_title("Match");
    matchDialog->show_all();
    matchDialog->signal_response().connect([this]([[maybe_unused]] int id) { RefreshMatch(); });
}
gboolean scroll_cb([[maybe_unused]] GtkWidget *self, GdkEventScroll *event, gpointer user_data) {
    FUN_INFO("scroll cb %08X, %08X", event->state, event->type);
    auto sess = (TerminalSession *) user_data;
    auto dir = event->direction;
    if (event->direction == GDK_SCROLL_SMOOTH) {
        dir = (event->delta_y <= 0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
    }
    if ((event->state & GdkModifierType::GDK_CONTROL_MASK) && (dir == GDK_SCROLL_UP)) {
        sess->FontZoomUp();
        return true;
    } else if ((event->state & GdkModifierType::GDK_CONTROL_MASK) && (dir == GDK_SCROLL_DOWN)) {
        sess->FontZoomDown();
        return true;
    }

    return false;
}
gboolean button_press_cb(GtkWidget *self, GdkEventButton *but, gpointer user_data) {
    TerminalSession *sess = (TerminalSession *) user_data;
    if (but->state & GDK_SHIFT_MASK && but->button == GDK_BUTTON_SECONDARY) {
        sess->ShowContextMenu(but);
        return true;
    }
    return false;
}
gboolean key_press_cb([[maybe_unused]] GtkWidget *self, [[maybe_unused]] GdkEventKey *event, gpointer user_data) {
    FUN_INFO("event %c, %s", (char) event->keyval, event->string);
    if ((event->state & GDK_SHIFT_MASK) && (event->state & GDK_CONTROL_MASK) && ((char) event->keyval == 'F')) {
        auto sess = (TerminalSession *) user_data;
        sess->ToggleSearch();
        return true;
    } else if ((event->state & GDK_SHIFT_MASK) && (event->state & GDK_CONTROL_MASK) && ((char) event->keyval == 'M')) {
        auto sess = (TerminalSession *) user_data;
        sess->ToggleMatch();
        return true;
    }
    if ((event->state & GDK_SHIFT_MASK) && (event->state & GDK_CONTROL_MASK)) {
        if (event->keyval == GDK_KEY_C) {
            TerminalSession *sess = (TerminalSession *) user_data;
            vte_terminal_copy_clipboard_format(sess->m_terminal, VteFormat::VTE_FORMAT_TEXT);
            return true;
        } else if (event->keyval == GDK_KEY_V) {
            TerminalSession *sess = (TerminalSession *) user_data;
            vte_terminal_paste_clipboard(sess->m_terminal);
            return true;
        }
    }
    return false;
}

void selection_changed(VteTerminal *term, TerminalSession *sess) {
    vte_terminal_copy_primary(term);
    auto clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    Glib::ustring text = gtk_clipboard_wait_for_text(clipboard);
    sess->m_selectionText = text;
}

void notification_received([[maybe_unused]] VteTerminal *term, const gchar *summary, const gchar *body) {
    FUN_INFO("summary:%s, body:%s", summary, body);
}
void TerminalSession::InitTerminal() {
    auto termWidget = vte_terminal_new();
    std::array<char *, 2> startterm = {nullptr, nullptr};
    startterm[0] = vte_get_user_shell();

    auto term = VTE_TERMINAL(termWidget);
    vte_terminal_spawn_sync(term,
                            VTE_PTY_DEFAULT,
                            workingDir.empty() ? nullptr : workingDir.c_str(),
                            startterm.data(),
                            nullptr,
                            G_SPAWN_SEARCH_PATH,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr);
    vte_terminal_set_scrollback_lines(term, 10000);
    vte_terminal_set_font_scale(term, 1.0f);
    PangoFontDescription *fontDesc = pango_font_description_new();
    pango_font_description_set_family(fontDesc, "FiraCode");
    pango_font_description_set_stretch(fontDesc, PangoStretch::PANGO_STRETCH_CONDENSED);
    pango_font_description_set_gravity(fontDesc, PangoGravity::PANGO_GRAVITY_WEST);
    FUN_INFO("vte_terminal set_font %s", pango_font_description_to_string(fontDesc));
    vte_terminal_set_font(term, fontDesc);
    GdkRGBA backgroundColor;
    gdk_rgba_parse(&backgroundColor, "#fdfdf6f6e3e3");
    vte_terminal_set_color_background(term, &backgroundColor);
    gdk_rgba_parse(&backgroundColor, "#00002B2B3636");
    vte_terminal_set_color_foreground(term, &backgroundColor);
    m_terminal = term;
    g_signal_connect(m_terminal, "key-press-event", G_CALLBACK(key_press_cb), this);
    g_signal_connect(m_terminal, "button-press-event", G_CALLBACK(button_press_cb), this);
    g_signal_connect(m_terminal, "scroll-event", G_CALLBACK(scroll_cb), this);
    g_signal_connect(m_terminal, "selection-changed", G_CALLBACK(selection_changed), this);
    g_signal_connect(m_terminal, "notification-received", G_CALLBACK(notification_received), this);

    gtk_widget_set_vexpand(termWidget, true);
    gtk_widget_set_halign(termWidget, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(termWidget, true);
    gtk_widget_set_valign(termWidget, GTK_ALIGN_FILL);

    vte = Glib::wrap(termWidget);
    vte->set_can_focus(true);
    vte->set_focus_on_click(true);
    vte->signal_focus().connect([this]([[maybe_unused]] bool v) {
        FUN_INFO("lastFocusTerm %p", this);
        lastFocusTerm = this;
        return true;
    });
    vte->signal_focus_in_event().connect([this]([[maybe_unused]] bool v) {
        FUN_INFO("vte focus in");
        FUN_INFO("lastFocusTerm %p", this);
        lastFocusTerm = this;
        return true;
    });

    vte->signal_button_press_event().connect([this](GdkEventButton *ev) {
        FUN_INFO("button press");

        if (ev->button == GDK_BUTTON_SECONDARY) {
            ShowContextMenu(ev);

            FUN_INFO("right button");
            return true;
        }

        return true;
    });

    vte->set_data("self", this);
}

void TerminalSession::UpdatePreference(const Preference &pref, Changes changes) {
    FUN_DEBUG("UpdatePreference %d", changes.cs_changed);
    if (changes.cs_changed) {
        auto cs = pref.GetColorscheme();
        if (cs.has_value()) {
            FUN_DEBUG("cs.has_value");
            if (cs->numColor != 0) {
                vte_terminal_set_colors(m_terminal, &cs->fg, &cs->bg, cs->palette, cs->numColor);
            }
            if (cs->hasBg) {
                vte_terminal_set_color_background(m_terminal, &cs->bg);
            }
            if (cs->hasFg) {
                vte_terminal_set_color_foreground(m_terminal, &cs->fg);
            }
            if (cs->hasCursor) {
                vte_terminal_set_color_cursor(m_terminal, &cs->cursor);
            }
        }
    }
    if (changes.font_changed) {
        FUN_INFO("vte_terminal set_font %s", pango_font_description_to_string(pref.font_desc));
        vte_terminal_set_font(m_terminal, pref.font_desc);
    }
}
void TerminalSession::ShowContextMenu(GdkEventButton *event) {
    m_popupMenu.Clear();

    auto itemCopy = std::make_unique<Gtk::MenuItem>(_("Copy"), true);
    itemCopy->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        CopyText();
        return true;
    });
    auto itemPaste = std::make_unique<Gtk::MenuItem>(_("Paste"), true);
    itemPaste->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        PasteText();
        return true;
    });

    auto itemSearch = std::make_unique<Gtk::MenuItem>(_("Search"), false);
    itemSearch->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        ToggleSearch();
        return true;
    });
    auto itemHighlight = std::make_unique<Gtk::MenuItem>(_("Highlight Management"), false);
    itemHighlight->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        ToggleMatch();
        return true;
    });

    auto itemSearchSelected = std::make_unique<Gtk::MenuItem>(_("Search Selected"), false);
    itemSearchSelected->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        ToggleSearch(m_selectionText, true);
        return true;
    });
    auto itemHighlightSelected = std::make_unique<Gtk::MenuItem>(_("Highlight Selected"), false);
    itemHighlightSelected->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        AddHighlight(m_selectionText);
        return true;
    });

    auto itemCopyPath = std::make_unique<Gtk::MenuItem>(_("Copy Path"), false);
    itemCopyPath->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        auto dirPath = vte_terminal_get_current_directory_uri(m_terminal);
        auto filePath = vte_terminal_get_current_file_uri(m_terminal);
        FUN_INFO("dirPath %s, filePath:%s", dirPath, filePath);
        if (dirPath != nullptr) {
            auto clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_clipboard_set_text(clipboard, dirPath, (gint) strlen(dirPath));
        }

        return true;
    });

    auto itemOpenPath = std::make_unique<Gtk::MenuItem>(_("Open Path"), false);
    itemOpenPath->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        auto dirPath = vte_terminal_get_current_directory_uri(m_terminal);
        auto filePath = vte_terminal_get_current_file_uri(m_terminal);
        FUN_INFO("dirPath %s, filePath:%s", dirPath, filePath);
        if (dirPath != nullptr) {
            g_app_info_launch_default_for_uri(dirPath, nullptr, nullptr);
        }
        return true;
    });

    auto itemPreference = std::make_unique<Gtk::MenuItem>(_("Preference"), true);
    itemPreference->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
        if (m_pref != nullptr) {
            m_pref->PreferenceFromDialog();
        }
        return true;
    });
    if (vte_terminal_get_has_selection(m_terminal) && m_highlightedTexts.count(m_selectionText)) {
        auto itemRemoveHighlight = std::make_unique<Gtk::MenuItem>(_("Remove Highlight"), true);
        itemRemoveHighlight->signal_button_press_event().connect([this]([[maybe_unused]] GdkEventButton const *ev) {
            m_highlightedTexts.erase(m_selectionText);
            vte_terminal_highlight_clear(m_terminal);
            for (const auto &text : m_highlightedTexts) {
                AddHighlight(text);
            }
            return true;
        });
        m_popupMenu.Add(std::move(itemRemoveHighlight));
    }

    m_popupMenu.Add(std::move(itemCopy));
    m_popupMenu.Add(std::move(itemPaste));
    m_popupMenu.Add(std::make_unique<Gtk::SeparatorMenuItem>());

    m_popupMenu.Add(std::move(itemSearchSelected));
    m_popupMenu.Add(std::move(itemHighlightSelected));
    m_popupMenu.Add(std::make_unique<Gtk::SeparatorMenuItem>());

    m_popupMenu.Add(std::move(itemCopyPath));
    m_popupMenu.Add(std::move(itemOpenPath));

    m_popupMenu.Add(std::make_unique<Gtk::SeparatorMenuItem>());
    m_popupMenu.Add(std::move(itemSearch));
    m_popupMenu.Add(std::move(itemHighlight));
    m_popupMenu.Add(std::move(itemPreference));
    m_popupMenu.Show((GdkEvent *) event);
}

void TerminalSession::CopyText() {
    if (m_terminal == nullptr) {
        return;
    }
    if (vte_terminal_get_has_selection(m_terminal)) {
        vte_terminal_copy_clipboard_format(m_terminal, VTE_FORMAT_TEXT);
    }
}

void TerminalSession::PasteText() {
    if (m_terminal == nullptr) {
        return;
    }
    vte_terminal_paste_clipboard(m_terminal);
}

bool TerminalSession::OnTitleDoubleClicked(GdkEventButton *ev, TitleEntry *label) const {
    if (ev->type == GDK_2BUTTON_PRESS) {
        FUN_INFO("double click");
        label->set_can_focus(true);
        label->set_has_frame(true);
        label->set_editable(true);
    }
    return true;
}
void TerminalSession::InitTitleBox() {
    titleBox = Glib::RefPtr<Gtk::Box>(Gtk::make_managed<Gtk::Box>());
    auto label = Gtk::make_managed<TitleEntry>("Sess-" + std::to_string(m_id));

    auto buttonClose = Gtk::make_managed<Gtk::Button>();
    auto buttonMax = Gtk::make_managed<Gtk::Button>();
    buttonClose->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_BUTTON);
    buttonMax->set_image_from_icon_name("window-maximize-symbolic", Gtk::ICON_SIZE_BUTTON);
    buttonMax->set_relief(Gtk::RELIEF_NONE);
    buttonClose->set_relief(Gtk::RELIEF_NONE);
    buttonMax->get_style_context()->add_class("circular");
    buttonClose->get_style_context()->add_class("circular");

    buttonMax->signal_clicked().connect([this, buttonMax]() {
        if (!m_tab->HasMaxSess()) {
            m_tab->Max(this);
            buttonMax->set_image_from_icon_name("window-restore-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        } else {
            m_tab->Restore();
            buttonMax->set_image_from_icon_name("window-maximize-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        }
    });
    buttonClose->signal_clicked().connect([this]() {
        auto parent = GetParent();
        if (GetParent()->get_child2() == this) {
            if (GetParent()->get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
                GetParent()->set_position(GetParent()->get_width());
            } else {
                GetParent()->set_position(GetParent()->get_height());
            }
            GetParent()->remove(this);
            parent->set_focus_child(*parent->get_child1());
        } else if (GetParent()->get_child1() == this) {
            if (auto child2 = parent->get_child2(); child2 != nullptr) { // make child2 the first one and resize
                parent->remove(this);
                parent->Move221();
                if (parent->get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
                    parent->set_position(parent->get_width());
                } else {
                    parent->set_position(parent->get_height());
                }
                parent->set_focus_child(*parent->get_child1());
            } else { // only child deleted
                parent->remove(this);
                parent->removeFromParent();
            }
        }
    });
    titleBox->set_halign(Gtk::ALIGN_FILL);
    titleBox->set_border_width(0);

    titleBox->pack_start(*label);
    titleBox->pack_end(*buttonClose, false, false, 0);
    titleBox->pack_end(*buttonMax, false, false, 0);
    titleBox->set_halign(Gtk::ALIGN_FILL);
    //    titleBox->get_style_context()->add_class("title_box");
    auto color = buttonMax->get_style_context()->get_color();
    auto bgcolor = buttonMax->get_style_context()->get_background_color();
    auto color2 = buttonMax->get_style_context()->get_border_color();

    FUN_DEBUG("color %s", color.to_string().c_str());
    FUN_DEBUG("bgcolor %s", bgcolor.to_string().c_str());
    for (const auto &clazz : buttonMax->get_style_context()->list_classes()) {
        FUN_DEBUG("%s ", clazz.c_str());
    }
    titleBox->set_halign(Gtk::ALIGN_FILL);
}
void TerminalSession::HideTitle() {
    remove(*m_topBar.get());
    m_showTitle = false;
}
void TerminalSession::ShowTitle() {
    pack1(*m_topBar.get());
    m_showTitle = true;
}
void TerminalSession::Split(TerminalSession *new_sess, Gtk::Orientation orient) {
    FUN_DEBUG("this:%p, parent:%p ", this, this->get_parent());

    auto parent_paned = GetParent();

    if (parent_paned->get_child1() == this) {        // first child
        if (parent_paned->get_child2() == nullptr) { // second child is empty
            parent_paned->set_orientation(orient);
            parent_paned->add2(new_sess);
            new_sess->set_parent(*parent_paned);
            parent_paned->show_all();
            dualSplit(*parent_paned);
        } else { // second child is not empty
            auto w = this->get_width();
            auto h = this->get_height();
            parent_paned->remove(this);
            auto new_panned = parent_paned->NewPanedContainerAt(1, orient);
            new_panned->setId();
            parent_paned->add1(new_panned);
            new_panned->add1(this);
            new_panned->add2(new_sess);
            this->reparent(*new_panned);
            new_sess->set_parent(*new_panned);
            new_panned->show_all();
            FUN_INFO("new paned size %d, %d", new_panned->get_width(), new_panned->get_height());
            FUN_DEBUG("this:%p, parent:%p ", this, this->get_parent());
            dualSplit(*new_panned, w, h);
        }
    } else if (parent_paned->get_child2() == this) {
        auto w = this->get_width();
        auto h = this->get_height();

        parent_paned->remove(this);
        auto new_panned = parent_paned->NewPanedContainerAt(2, orient);

        new_panned->add1(this);
        new_panned->add2(new_sess);
        this->reparent(*new_panned);
        new_sess->set_parent(*new_panned);
        new_panned->show_all();
        FUN_INFO("new paned size %d, %d", new_panned->get_width(), new_panned->get_height());
        dualSplit(*new_panned, w, h);
    }
}

// static
void TerminalSession::dualSplit(Gtk::Paned &paned, int w, int h) {
    if (w == -1) {
        w = paned.get_width();
    }
    if (h == -1) {
        h = paned.get_height();
    }
    if (paned.get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        FUN_INFO("horizontal %d", w);
        w /= 2;
        paned.set_position(w);
    } else {
        FUN_INFO("vertical %d", h);
        h /= 2;
        paned.set_position(h);
    }
}

void TerminalSession::FontZoomUp() {
    auto desc = vte_terminal_get_font(m_terminal);
    auto newDesc = pango_font_description_copy(desc);
    auto size = pango_font_description_get_size(desc);
    FUN_INFO("%d", size);
    size += 1000;
    pango_font_description_set_size(newDesc, size);
    vte_terminal_set_font(m_terminal, newDesc);
    pango_font_description_free(newDesc);
}
void TerminalSession::FontZoomDown() {
    FUN_INFO("");
    auto desc = vte_terminal_get_font(m_terminal);
    auto newDesc = pango_font_description_copy(desc);
    auto size = pango_font_description_get_size(desc);
    size -= 1000;
    if (size < 0) {
        size = 1000;
    }
    pango_font_description_set_size(newDesc, size);
    vte_terminal_set_font(m_terminal, newDesc);
    pango_font_description_free(newDesc);
}
