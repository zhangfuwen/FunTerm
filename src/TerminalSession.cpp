//
// Created by zhangfuwen on 2022/4/20.
//
#define PCRE2_CODE_UNIT_WIDTH 0
#include "TerminalSession.h"
#include <pcre2.h>
#include <vte/vte.h>
TerminalSession::TerminalSession(Tab *tab) : Gtk::Paned(Gtk::ORIENTATION_VERTICAL){
    m_tab = tab;
    static int id = 0;
    id++;
    m_id = id;
    Gtk::Widget::set_name("Sess-" + std::to_string(id));
    FUN_INFO("creating session %d", id);

    InitTerminal();
    InitTitleBox();
    InitSearchBox();
    InitMatchBox();

    // setup topbar
    m_topBar = new Gtk::Box();
    m_topBar->pack_start(*titleBox);
    Gtk::Paned::add1(*m_topBar);
    Gtk::Paned::set_position(40);
    set_orientation(Gtk::ORIENTATION_VERTICAL);
    show_all();

    // setup content area
    m_bottomBar = new Gtk::Box();
    scroll = new Gtk::VScrollbar();
    m_bottomBar->add(*vte);
    m_bottomBar->add(*scroll);
    add2(*m_bottomBar);

    Gtk::Paned::set_focus_on_click(true);
    Gtk::Paned::set_can_focus(true);
    Gtk::Paned::set_visible(true);
    Gtk::Paned::set_child_visible(true);
    Gtk::Paned::signal_focus_in_event().connect([](bool v) {
        FUN_INFO("panned focus in");
        return true;
    });
    set_focus_child(*m_bottomBar);
    m_bottomBar->set_focus_child(*vte);
    signal_size_allocate().connect([this](Gdk::Rectangle &rec)  {
        this->set_position(40);
    });
    Gtk::Paned::set_property("max-position", 40);
}

void TerminalSession::InitMatchBox() {
    m_matchBox = new Gtk::Box();
    auto checkMatch = new Gtk::CheckButton();
    checkMatch->set_tooltip_text("toggle highlight matches");
    checkMatch->signal_toggled().connect([this, checkMatch]() {
        FUN_DEBUG("match toggle %d", checkMatch->get_active());
        m_highlightMatch = checkMatch->get_active();
        RefreshMatch();
    });
    auto label = new Gtk::Label("Highlight");
    label->set_margin_left(8);

    auto butConfig = new Gtk::Button();
    butConfig->set_image_from_icon_name("emblem-system-symbolic", Gtk::ICON_SIZE_MENU);
    butConfig->set_tooltip_text("Manage highlight words");
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
        vte_terminal_match_remove_all(terminal);
        vte_terminal_highlight_clear(terminal);
        for (auto &m : matchRegexes) {
            if (m.pattern == nullptr) {
                m.CompileForMatch();
            }
            FUN_DEBUG("add_regex %s, %p", m.text.c_str(), m.pattern);
            vte_terminal_match_add_regex(terminal, m.pattern, 0);
            AddHighlight(m);
        }

    } else {
        vte_terminal_match_remove_all(terminal);
    }
}
void TerminalSession::AddHighlight(const RegexMatch &m) const {
    HighlightStyle  style{0};
    style.backmask = 0xffffffff;
    style.back = 11;

    std::string s = m.text;
    HighlightPattern  pat;
    if (!m.caseSensitive) {
        pat.regex_flags |= PCRE2_CASELESS;
    } else {
        pat.regex_flags = 0;
    }
    if (!m.regex) { // not regex search
                s = g_regex_escape_string(m.text.c_str(), m.text.size());
    }
    if (m.wholeWord) {
                using namespace std::string_literals;
                s = "\\b" + s + "\\b";
    }
    pat.pattern = s.c_str();
    pat.style = style;
    pat.regex = true;
    vte_terminal_highlight_add_pattern(terminal, pat);
}

void TerminalSession::InitSearchBox() {
    searchBox = new Gtk::Box();
    auto search = new Gtk::SearchEntry();
    auto caseSensitiveSearch = new Gtk::ToggleButton("Cc");
    auto regexSearch = new Gtk::ToggleButton(".*");
    auto wholeWord = new Gtk::ToggleButton("W");
    caseSensitiveSearch->get_style_context()->add_class("circular");
    regexSearch->get_style_context()->add_class("circular");
    wholeWord->get_style_context()->add_class("circular");
    caseSensitiveSearch->set_relief(Gtk::RELIEF_NONE);
    regexSearch->set_relief(Gtk::RELIEF_NONE);
    wholeWord->set_relief(Gtk::RELIEF_NONE);
    caseSensitiveSearch->set_tooltip_text("Case-sensitive Search");
    regexSearch->set_tooltip_text("Search with regular expresion");
    wholeWord->set_tooltip_text("Search with whole word");
    auto next = new Gtk::Button();
    auto prev = new Gtk::Button();
    auto close = new Gtk::Button();

    next->set_image_from_icon_name("go-down-symbolic", Gtk::ICON_SIZE_MENU);
    prev->set_image_from_icon_name("go-up-symbolic", Gtk::ICON_SIZE_MENU);
    close->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_MENU);
    next->set_relief(Gtk::RELIEF_NONE);
    prev->set_relief(Gtk::RELIEF_NONE);
    close->set_relief(Gtk::RELIEF_NONE);
    next->set_halign(Gtk::ALIGN_END);
    prev->set_halign(Gtk::ALIGN_END);
    close->set_halign(Gtk::ALIGN_END);
    searchBox->pack_start(*search);
    searchBox->pack_start(*caseSensitiveSearch);
    searchBox->pack_start(*wholeWord);
    searchBox->pack_start(*regexSearch);
    searchBox->pack_end(*close);
    searchBox->pack_end(*next);
    searchBox->pack_end(*prev);
    searchBox->set_halign(Gtk::ALIGN_CENTER);
    searchBox->set_valign(Gtk::ALIGN_CENTER);
    search->signal_next_match().connect([search, this, wholeWord, caseSensitiveSearch, regexSearch]() {
        auto text = search->get_text();
        auto regex = vte_regex_new_for_search(text.c_str(), text.length() + 1, 0, nullptr);
        vte_terminal_search_set_regex(terminal, regex, 0);
          RegexMatch m(text);
          m.caseSensitive = caseSensitiveSearch->get_active();
          m.regex = regexSearch->get_active();
          m.wholeWord = wholeWord->get_active();
          AddHighlight(m);
    });
    //    search->signal_insert_text().connect([](){
    //      FUN_DEBUG("");
    //      return true;
    //    });
    search->signal_search_changed().connect([this, search, caseSensitiveSearch, regexSearch, wholeWord]() {
        GError *error = nullptr;
        auto text = search->get_text();
        guint32 compile_flags;
        compile_flags = PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE;
        if (!caseSensitiveSearch->get_active()) {
            compile_flags |= PCRE2_CASELESS;
        }
        if (!regexSearch->get_active()) { // not regex search
            text = g_regex_escape_string(text.c_str(), text.size());
        }
        if (wholeWord->get_active()) {
            text = "\\b" + text + "\\b";
        }

        if (searchRegex != nullptr) {
            vte_regex_unref(searchRegex);
        }
        searchRegex = vte_regex_new_for_search(text.c_str(), text.size(), compile_flags, &error);
        if (searchRegex != nullptr) {
            vte_terminal_search_set_regex(terminal, searchRegex, 0);
            if (!vte_terminal_search_find_next(terminal)) {
                vte_terminal_search_find_previous(terminal);
            }
        } else {
            FUN_ERROR("regex error: %s", vte_regex_error_quark());
        }
        FUN_DEBUG("search changed: %s", text.c_str());
    });
    next->signal_clicked().connect([this]() {
        FUN_DEBUG("search next");
        vte_terminal_search_find_next(terminal);
    });
    next->set_can_focus(true);
    prev->signal_clicked().connect([this]() {
        FUN_DEBUG("search prev");
        vte_terminal_search_find_previous(terminal);
        //      return true;
    });

    close->signal_clicked().connect([this]() { this->ToggleSearch(); });
}

void TerminalSession::ToggleMatch() {
    if (m_topBar->get_children().size() > 0 && m_topBar->get_children()[0] == titleBox) {
        m_topBar->remove(*titleBox);
        m_topBar->pack_start(*m_matchBox);
        if(!m_showTitle && get_child1() == nullptr) {
            pack1(*m_topBar);
        }
    } else {
        if(!m_showTitle && get_child1() != nullptr) {
            remove(*m_topBar);
        }
        m_topBar->remove(*m_matchBox);
        m_topBar->pack_start(*titleBox);
    }
    m_topBar->show_all();
}

void TerminalSession::ToggleSearch() {
    static Gtk::Widget * save = nullptr;
    if (m_topBar->get_children().size() > 0 && m_topBar->get_children()[0] == titleBox) {
        m_topBar->remove(*titleBox);
        m_topBar->pack_start(*searchBox);
        if(!m_showTitle && get_child1() == nullptr) {
            pack1(*m_topBar);
        }
    } else {
        if(!m_showTitle && get_child1() != nullptr) {
            remove(*m_topBar);
        }
        m_topBar->remove(*searchBox);
        m_topBar->pack_start(*titleBox);
    }
    m_topBar->show_all();
}
static void grid_set_header(Gtk::Grid *grid) {
    Gtk::Label *label1 = new Gtk::Label(_("pattern"));
    Gtk::Label *label2 = new Gtk::Label(_("Case Sensitive"));
    Gtk::Label *label3 = new Gtk::Label(_("Whole word"));
    Gtk::Label *label4 = new Gtk::Label(_("Regex"));
    Gtk::Label *label5 = new Gtk::Label(_("delete"));
    grid->attach(*label1, 0, 0);
    grid->attach(*label2, 1, 0);
    grid->attach(*label3, 2, 0);
    grid->attach(*label4, 3, 0);
    grid->attach(*label5, 4, 0);
    grid->show_all_children(true);
}

static void
grid_add_row(Gtk::Grid *grid, int row_index, std::string key, bool caseSensitive, bool wholeWord, bool regex) {
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
    const int i = row_index;
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
        text = g_regex_escape_string(text.c_str(), text.size());
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
        pattern_text = g_regex_escape_string(pattern_text.c_str(), pattern_text.size());
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

    but_new->signal_clicked().connect([=]() {
        FUN_INFO("button_clicked");
        auto num_rows = grid_get_num_rows(grid);
        FUN_DEBUG("num rows %d", num_rows);
        grid_add_row(grid, num_rows, "", false, false, false);
    });

    but_clear->signal_clicked().connect([=]() {
        auto num_rows = grid_get_num_rows(grid);
        for (int i = 1; i < num_rows; i++) {
            // row 0 contains labels that should be kept
            // row 2 becomes row 1 when row 1 is deleted, so always remove row 1
            grid->remove_row(1);
        }
        // add an empty row for input
        grid_add_row(grid, 1, "", false, false, false);
    });

    but_save->signal_clicked().connect([=]() {
        matchRegexes.clear();
        for (int i = 1; i < 1000; i++) {
            if (grid->get_child_at(0, i) == nullptr) {
                FUN_ERROR("child \"%d\" does not exist", i);
                break;
            }
            auto keyEntry = (Gtk::Entry *)grid->get_child_at(0, i);
            if (!keyEntry) {
                FUN_DEBUG("failed to get entry");
            }
            auto checkCase = (Gtk::CheckButton *)grid->get_child_at(1, i);
            auto checkWholeWord = (Gtk::CheckButton *)grid->get_child_at(2, i);
            auto checkRegex = (Gtk::CheckButton *)grid->get_child_at(3, i);
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
    for (auto match : matchRegexes) {
        auto num_rows = grid_get_num_rows(grid);
        grid_add_row(grid, num_rows, match.text, match.caseSensitive, match.wholeWord, match.regex);
    }

    Gtk::Dialog *matchDialog = new Gtk::Dialog();
    matchDialog->get_content_area()->pack_start(*box);
    matchDialog->set_title("Match");
    matchDialog->show_all();
    matchDialog->signal_response().connect([this](int id) { RefreshMatch(); });
}

gboolean key_press_cb(GtkWidget *self, GdkEventKey *event, gpointer user_data) {
//    FUN_INFO("event %c, %s", (char)event->keyval, event->string);
    if ((event->state & GDK_SHIFT_MASK) && (event->state & GDK_CONTROL_MASK) && ((char)event->keyval == 'F')) {
        TerminalSession *sess = (TerminalSession *)user_data;
        sess->ToggleSearch();
        return true;
    } else if ((event->state & GDK_SHIFT_MASK) && (event->state & GDK_CONTROL_MASK) && ((char)event->keyval == 'M')) {
        TerminalSession *sess = (TerminalSession *)user_data;
        sess->ToggleMatch();
        return true;
    }
    return false;
}
void TerminalSession::InitTerminal() {
    auto termWidget = vte_terminal_new();
    char *startterm[2] = {0, 0};
    startterm[0] = vte_get_user_shell();

    VteTerminal *term = VTE_TERMINAL(termWidget);
    vte_terminal_spawn_sync(
        term,
        VTE_PTY_DEFAULT,
        nullptr,
        startterm,
        nullptr,
        G_SPAWN_SEARCH_PATH,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    vte_terminal_set_font_scale(term, 1.0f);
    PangoFontDescription *fontDesc = pango_font_description_new();
    pango_font_description_set_family(fontDesc, "FiraCode");
    pango_font_description_set_stretch(fontDesc, PangoStretch::PANGO_STRETCH_CONDENSED);
    pango_font_description_set_gravity(fontDesc, PangoGravity::PANGO_GRAVITY_WEST);
    vte_terminal_set_font(term, fontDesc);
    GdkRGBA *backgroundColor = new GdkRGBA;
    gdk_rgba_parse(backgroundColor, "#fdfdf6f6e3e3");
    vte_terminal_set_color_background(term, backgroundColor);
    gdk_rgba_parse(backgroundColor, "#00002B2B3636");
    vte_terminal_set_color_foreground(term, backgroundColor);
    terminal = term;
    g_signal_connect(terminal, "key-press-event", G_CALLBACK(key_press_cb), this);

    gtk_widget_set_vexpand(termWidget, true);
    gtk_widget_set_halign(termWidget, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(termWidget, true);
    gtk_widget_set_valign(termWidget, GTK_ALIGN_FILL);

    vte = Glib::wrap(termWidget);
    vte->set_can_focus(true);
    vte->set_focus_on_click(true);
    vte->signal_focus().connect([this](bool v) {
        FUN_INFO("lastFocusTerm %p", this);
        lastFocusTerm = this;
        return true;
    });
    vte->signal_focus_in_event().connect([this](bool v) {
        FUN_INFO("vte focus in");
        FUN_INFO("lastFocusTerm %p", this);
        lastFocusTerm = this;
        return true;
    });

    vte->signal_button_press_event().connect([this](GdkEventButton *ev) {
        FUN_INFO("button press");
        Gtk::Menu menu;
        Gtk::MenuItem item;
        item.set_label("close");
        item.signal_select().connect([this]() { GetParent()->remove(this); });
        menu.add(item);
        menu.show_all();
        menu.popup_at_pointer((GdkEvent *)ev);

        return true;
    });

    vte->set_data("self", this);
    //        vte->set_focus_on_click(true);
}

bool TerminalSession::OnTitleDoubleClicked(GdkEventButton *ev, TitleEntry * label) {
    if(ev->type == GDK_2BUTTON_PRESS) {
        FUN_INFO("double click");
        label->set_can_focus(true);
        label->set_has_frame(true);
        label->set_editable(true);
    }
    return true;
}
void TerminalSession::InitTitleBox() {
    titleBox = new Gtk::Box();
    auto label = new TitleEntry("Sess-" + std::to_string(m_id));

    auto buttonClose = new Gtk::Button();
    auto buttonMax = new Gtk::Button();
    buttonClose->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
    buttonMax->set_image_from_icon_name("window-maximize-symbolic", Gtk::ICON_SIZE_MENU);
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
    titleBox->set_border_width(5);
    titleBox->pack_start(*label);
    titleBox->pack_end(*buttonClose, false, false, 0);
    titleBox->pack_end(*buttonMax, false, false, 0);
    titleBox->set_halign(Gtk::ALIGN_FILL);
    auto color = buttonMax->get_style_context()->get_color();
    auto bgcolor = buttonMax->get_style_context()->get_background_color();
    auto color2 = buttonMax->get_style_context()->get_border_color();

    FUN_DEBUG("color %s", color.to_string().c_str());
    FUN_DEBUG("bgcolor %s", bgcolor.to_string().c_str());
    for (auto clazz : buttonMax->get_style_context()->list_classes()) {
        FUN_DEBUG("%s ", clazz.c_str());
    }
    titleBox->override_background_color(color2);
    buttonMax->override_background_color(bgcolor);
    titleBox->set_halign(Gtk::ALIGN_FILL);
}
void TerminalSession::HideTitle() {
    remove(*m_topBar);
    m_showTitle = false;
}
void TerminalSession::ShowTitle() {
    pack1(*m_topBar);
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
