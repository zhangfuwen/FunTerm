//
// Created by zhangfuwen on 2022/4/25.
//

#ifndef FUNTERM_TITLEENTRY_H
#define FUNTERM_TITLEENTRY_H
#include <gtkmm.h>
class TitleEntry : public Gtk::Entry {
  public:
    TitleEntry() { Disable(); }

    explicit TitleEntry(const Glib::ustring &str) {
        Disable();
        set_text(str);
    }

  protected:
    void on_editing_done() override {
        Disable();
        CellEditable::on_editing_done();
    }
    bool on_button_press_event(GdkEventButton *button_event) override {
        if (button_event->type == GDK_2BUTTON_PRESS) {
            Enable();
            return true;
        }
        if (this->get_editable() == false) { // when not editable, send event to container
            return false;
        }
        return Widget::on_button_press_event(button_event);
    }
    bool on_focus_out_event(GdkEventFocus *gdk_event) override {
        Disable();
        return Widget::on_focus_out_event(gdk_event);
    }
    bool on_key_release_event(GdkEventKey *key_event) override {
        if (key_event->keyval == GDK_KEY_Escape) {
            Disable();
        }
        return Widget::on_key_release_event(key_event);
    }

  public:
    void Enable() {
        set_can_focus(true);
        set_has_frame(true);
        set_editable(true);
        grab_focus(); // immediately editable
    }
    void Disable() {
        set_editable(false);
        set_can_focus(false);
        set_has_frame(false);
    }
};
#endif // FUNTERM_TITLEENTRY_H
