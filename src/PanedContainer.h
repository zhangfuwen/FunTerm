//
// Created by zhangfuwen on 2022/4/21.
//

#ifndef FUNTERM_PANEDCONTAINER_H
#define FUNTERM_PANEDCONTAINER_H

#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/window.h>

#include <vte/vte.h>

#include "common.h"
#include "common_log.h"

#include "configor/json.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class TerminalSession;
class RootPanedContainer;
extern TerminalSession *lastFocusTerm;
class PanedContainer : public Gtk::Paned {
public:

    PanedContainer * NewPanedContainerAt(int i, Gtk::Orientation ori = Gtk::ORIENTATION_HORIZONTAL);

    void setRoot() { root = true; }
    void setId() {
        static int id = 0;
        id++;
        m_id = id;
    }
    void add1(TerminalSession *sess);
    void add2(TerminalSession *sess);
    void remove(TerminalSession *sess);
    void add1(PanedContainer *paned);
    void add2(PanedContainer *paned);
    void remove(PanedContainer *paned);

    // move second  child to the first place :)
    int Move221();
    void removeFromParent();

    static RootPanedContainer *GetRoot(PanedContainer *p);

protected:
    PanedContainer(Gtk::Orientation orient);
    int m_id = 0;
    bool root = false;
};

class RootPanedContainer : public PanedContainer {
public:
    RootPanedContainer() : PanedContainer(Gtk::ORIENTATION_HORIZONTAL) {
        root = true;
    }

    void AddTerminal(TerminalSession * sess);
    void RemoveTerminal(TerminalSession * sess);

    void TerminalNumChanged();

    using type_signal_terminal_num_changed = sigc::signal<void(int)>;
    type_signal_terminal_num_changed signal_terminal_num_changed() {
        return m_signal_terminal_num_changed;
    }
    void Dump();
    bool HasTerminalSession(TerminalSession * sess) {
        return m_terminalSessions.count(sess) != 0;
    }

protected:
    type_signal_terminal_num_changed m_signal_terminal_num_changed;

protected:
    std::set<TerminalSession*> m_terminalSessions;

};

#endif // FUNTERM_PANEDCONTAINER_H
