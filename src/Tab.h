//
// Created by zhangfuwen on 2022/4/21.
//

#ifndef FUNTERM_TAB_H
#define FUNTERM_TAB_H
#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/window.h>

#include <vte/vte.h>

#include "common.h"
#include "common_log.h"

#include "configor/json.hpp"

#include "PanedContainer.h"
#include "TitleEntry.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

struct MaxSess {
    PanedContainer  *savedRoot;
    bool             first_child;
    PanedContainer  *savedParent;
    TerminalSession *saved_sess;
};

class Tab;

class TabTitle : public Gtk::Box {
  public:
    TabTitle(const Glib::ustring &text, Tab *tab);

  private:
    Gtk::Button *m_closeButton = nullptr;
    TitleEntry  *m_titleTex    = nullptr;
    friend class Tab;
};

class Tab : public Gtk::Box {
  public:
    Tab(RootPanedContainer *paned, const Glib::ustring &label) {
        rootPaned  = paned;
        m_tabTitle = new TabTitle(label, this);
    }
    void AddToNotebook(Gtk::Notebook &notebook) {
        notebook.append_page(*this, *m_tabTitle);
        this->setNotebook(&notebook);
    }
    void Close() {
        if (m_notebook != nullptr) {
            m_notebook->remove_page(*this);
        }
    }
    void        Max(TerminalSession *sess);
    void        Restore();
    static Tab *GetTab(PanedContainer *p);
    bool        HasMaxSess();

    bool HasTerminalSession(TerminalSession *sess) { return rootPaned->HasTerminalSession(sess); }

  private:
    void setNotebook(Gtk::Notebook *notebook) { m_notebook = notebook; }

  private:
    MaxSess *maxSess = nullptr;

    RootPanedContainer *rootPaned  = nullptr;
    TabTitle           *m_tabTitle = nullptr;

    Gtk::Notebook *m_notebook = nullptr;
};

#endif // FUNTERM_TAB_H
