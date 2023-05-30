//
// Created by zhangfuwen on 2022/4/21.
//
#include "Tab.h"
#include "TerminalSession.h"

TabTitle::TabTitle(const Glib::ustring &text, Tab *tab) {
    m_titleTex = new TitleEntry(text);
    m_closeButton = new Gtk::Button;
    m_closeButton->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_MENU);

    m_closeButton->signal_clicked().connect([this, tab]() { tab->Close(); });
    m_closeButton->set_relief(Gtk::RELIEF_NONE);

    m_titleTex->set_has_frame(false);

    pack_start(*m_titleTex);
    pack_end(*m_closeButton);
    show_all();
}

void Tab::Max(TerminalSession *sess) {
    maxSess = new MaxSess;
    auto parent = sess->GetParent();
    auto rootNode = PanedContainer::GetRoot(parent);
    FUN_DEBUG("%p %p %p %p",
              sess,
              sess->GetParent()->get_child1(),
              sess->GetParent()->get_child2(),
              dynamic_cast<TerminalSession *>(sess->GetParent()->get_child1()));
    if (sess == (TerminalSession *) sess->GetParent()->get_child1()) {
        maxSess->first_child = true;
        parent->remove(sess);

    } else if (sess == (TerminalSession *) (sess->GetParent()->get_child2())) {
        maxSess->first_child = false;
        parent->remove(sess);
    }
    maxSess->savedParent = parent;
    maxSess->saved_sess = sess;

    remove(*rootPaned);
    add((Gtk::Widget &) *sess);
}
void Tab::Restore() {
    if (maxSess->first_child) {
        remove((Gtk::Widget &) *maxSess->saved_sess);
        maxSess->savedParent->add1(maxSess->saved_sess);
    } else {
        remove((Gtk::Widget &) *maxSess->saved_sess);
        maxSess->savedParent->add2(maxSess->saved_sess);
    }
    add(*rootPaned);
    maxSess = nullptr;
}

Tab *Tab::GetTab(PanedContainer *p) {
    auto root = PanedContainer::GetRoot(p);
    auto c = root->get_parent();
    return dynamic_cast<Tab *>(c);
}

bool Tab::HasMaxSess() { return maxSess != nullptr; }
