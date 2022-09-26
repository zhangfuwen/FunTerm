//
// Created by zhangfuwen on 2022/4/21.
//

#include "PanedContainer.h"

#include "TerminalSession.h"

PanedContainer::PanedContainer(Gtk::Orientation orient)
    : Gtk::Paned(orient) {
    //    signal_add().connect([this](Gtk::Widget *w) {
    //      auto root = GetRoot(this);
    //      auto sess = dynamic_cast<TerminalSession*>(w);
    //      root->AddTerminal(sess);
    //    });
    //    signal_remove().connect([this](Gtk::Widget *w) {
    //      auto root = GetRoot(this);
    //      auto sess = dynamic_cast<TerminalSession*>(w);
    //      root->AddTerminal(sess);
    //    });
}
RootPanedContainer *PanedContainer::GetRoot(PanedContainer *p) {
    while (!p->root) {
        p = dynamic_cast<PanedContainer *>(p->get_parent());
    }
    return dynamic_cast<RootPanedContainer *>(p);
}

PanedContainer *PanedContainer::NewPanedContainerAt(int i, Gtk::Orientation ori) {
    if (i == 1) {
        if (auto c = get_child1(); c == nullptr) {
            auto paned = new PanedContainer(ori);
            add1(paned);
            return paned;
        } else {
            return nullptr;
        }
    } else if (i == 2) {
        if (auto c = get_child2(); c == nullptr) {
            auto paned = new PanedContainer(ori);
            add2(paned);
            return paned;
        } else {
            return nullptr;
        }

    } else {
        return nullptr;
    }
}
int PanedContainer::Move221() {
    if (Gtk::Paned::get_child1() != nullptr) {
        return -1;
    }
    auto c2 = Gtk::Paned::get_child2();
    Gtk::Paned::remove(*c2);
    Gtk::Paned::add1(*c2);
    return 0;
}

void PanedContainer::removeFromParent() {
    if (!root) {
        auto parent = dynamic_cast<PanedContainer *>(this->get_parent());
        if (parent->get_child1() == this) {
            if (parent->get_child2() != nullptr) { // make second child first
                auto child2 = parent->get_child2();
                parent->remove(this);
                parent->Move221();
                child2->reparent(*parent);
                if (parent->get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
                    parent->set_position(parent->get_width());
                } else {
                    parent->set_position(parent->get_height());
                }

            } else {
                parent->remove(this);
                parent->removeFromParent();
            }
        } else if (parent->get_child2() == this) {
            parent->remove(this);
        }

    } else {
        this->get_parent()->remove(*this);
        lastFocusTerm = nullptr;
    }
}
void PanedContainer::add1(PanedContainer *paned) { Gtk::Paned::add1(*paned); }
void PanedContainer::add2(PanedContainer *paned) { Gtk::Paned::add2(*paned); }
void PanedContainer::remove(PanedContainer *paned) { Gtk::Paned::remove(*paned); }

void PanedContainer::add1(TerminalSession *sess) {
    Gtk::Paned::add1((Gtk::Widget &)*sess);
    GetRoot(this)->AddTerminal(sess);
}
void PanedContainer::add2(TerminalSession *sess) {
    Gtk::Paned::add2((Gtk::Widget &)*sess);
    GetRoot(this)->AddTerminal(sess);
}
void PanedContainer::remove(TerminalSession *sess) {
    Gtk::Paned::remove((Gtk::Widget &)*sess);
    GetRoot(this)->RemoveTerminal(sess);
}

void RootPanedContainer::AddTerminal(TerminalSession *sess) {
    FUN_DEBUG("add terminal %p", sess);
    m_terminalSessions.insert(sess);
    TerminalNumChanged();
}
void RootPanedContainer::RemoveTerminal(TerminalSession *sess) {
    FUN_DEBUG("remove terminal %p", sess);
    m_terminalSessions.erase(sess);
    TerminalNumChanged();
}
void RootPanedContainer::TerminalNumChanged() {
    Dump();
    m_signal_terminal_num_changed.emit(m_terminalSessions.size());
    FUN_DEBUG("terminal num %d", m_terminalSessions.size());
    if (m_terminalSessions.size() == 1) {
        TerminalSession *sess = *m_terminalSessions.begin();
        sess->HideTitle();
    } else if (m_terminalSessions.size() == 2) { // maybe from 1 to 2
        for (const auto &sess : m_terminalSessions) {
            sess->ShowTitle();
        }
    }
}

void RootPanedContainer::Dump() {
    std::queue<void *> q;
    q.emplace(this);
    while (!q.empty()) {
        int siz = (int)q.size();
        for (int i = 0; i < siz; i++) {
            auto x = q.front();
            q.pop();
            if (m_terminalSessions.count((TerminalSession *)x)) {
                printf("%p(term)->%p ", x, ((TerminalSession *)x)->get_parent());
            } else {
                printf("%p ", x);
            }
            if (x != nullptr && !m_terminalSessions.count((TerminalSession *)x)) {
                auto paned = (PanedContainer *)x;
                q.push(paned->get_child1());
                q.push(paned->get_child2());
            }
        }
        printf("\n");
    }
}
