//
// Created by zhangfuwen on 2022/1/18.
//
#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/window.h>

#include "PanedContainer.h"

#include <vte/vte.h>

#include "common.h"
#include "common_log.h"

#include "configor/json.hpp"
#include "TerminalSession.h"
#include "Tab.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

Glib::RefPtr<Gtk::Application> app;
Glib::RefPtr<Gtk::Builder> builder;
Gtk::ApplicationWindow *win;

void save_fast_input_config(Gtk::Grid *grid, std::string filepath = "") {
    if (filepath.empty()) {
        auto user_dir = get_ibus_fun_user_data_dir();
        if (!std::filesystem::is_directory(user_dir) && !std::filesystem::create_directory(user_dir)) {
            FUN_ERROR("directory \"%s\" does not exist and create failed", user_dir.c_str());
            return;
        }

        filepath = user_dir + "fast_input.json";
    }
    if (std::filesystem::exists(filepath) && !std::filesystem::remove(filepath)) {
        FUN_ERROR("file \"%s\" does exist and remove failed", filepath.c_str());
        return;
    }

    configor::json j;
    for (int i = 0; i < 1000; i++) {
        if (grid->get_child_at(0, i) == nullptr) {
            FUN_ERROR("child \"%d\" does not exist", i);
            break;
        }
        auto keyEntry = (Gtk::Entry *)grid->get_child_at(0, i);
        auto cmdEntry = (Gtk::Entry *)grid->get_child_at(1, i);
        auto keyString = keyEntry ? keyEntry->get_text() : "";
        auto cmdString = cmdEntry ? cmdEntry->get_text() : "";
        if (keyString.empty() || cmdString.empty()) {
            FUN_ERROR("key or cmd is empty");
            continue;
        }
        if (keyString.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != std::string::npos) {
            FUN_ERROR("key is invalid %s", keyString.c_str());
            continue;
        }
        j[keyString] = cmdString;
        FUN_INFO("save key to %s", keyString.c_str());
        FUN_INFO("save cmd to %s", cmdString.c_str());
    }
    auto s = j.dump();
    FUN_INFO("writing config file %s, content:%s", filepath.c_str(), s.c_str());
    int fd = open(filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        FUN_ERROR("failed to open file %s, %s", filepath.c_str(), strerror(errno));
    } else {
        FUN_INFO("file opened ,fd %d", fd);
        int ret = write(fd, s.c_str(), s.size());
        if (ret != s.size()) {
            FUN_ERROR("failed to write to file %d:%d", ret, s.size());
        }
        write(fd, "\n", 1);
        close(fd);
    }
}


TerminalSession * lastFocusTerm = nullptr;
Gtk::Notebook * notebook;

static int SplitTerm(Gtk::Orientation ori) {
    if(notebook == nullptr) {
        return -1;
    }
    Tab * tab = (Tab *)notebook->get_nth_page(notebook->get_current_page());
    if(tab == nullptr) {
        return -1;
    }
    if(lastFocusTerm == nullptr || !tab->HasTerminalSession(lastFocusTerm)) {
        return -1;
    }
    lastFocusTerm->Split(new TerminalSession(tab), ori);
    return 0;

}

int main(int argc, char *argv[]) {
    signal(SIGSEGV, signal_handler);
    // funterm
    Gio::init();
    app = Gtk::Application::create("fun.xjbcode.funterm", Gio::APPLICATION_HANDLES_COMMAND_LINE);
    app->add_main_option_entry(Gio::Application::OPTION_TYPE_STRING, "hori", 'h', "file system uri");
    app->add_main_option_entry(Gio::Application::OPTION_TYPE_STRING, "vert", 'v', "file system uri");
    app->add_main_option_entry(Gio::Application::OPTION_TYPE_STRING, "tab", 't', "file system uri");

    builder = Gtk::Builder::create_from_file( RES_FILE_DIR "/funterm.glade");
    std::cout << RES_FILE_DIR << std::endl;

    // headerbar
    Gtk::HeaderBar * headerbar ;
    auto builder2 = Gtk::Builder::create_from_file(RES_FILE_DIR "/headerbar.glade");
    builder2->get_widget<Gtk::HeaderBar>("headerbar", headerbar);
    Gtk::Button *but_new_tab;
    builder2->get_widget<Gtk::Button>("but_new_tab", but_new_tab);
    but_new_tab->set_image_from_icon_name("tab-new-symbolic", Gtk::ICON_SIZE_MENU);
    Gtk::Button *but_right_split;
    builder2->get_widget<Gtk::Button>("but_right_split", but_right_split);
    but_right_split->set_image_from_icon_name("go-last-symbolic", Gtk::ICON_SIZE_MENU);
    Gtk::Button *but_below_split;
    builder2->get_widget<Gtk::Button>("but_below_split", but_below_split);
    but_below_split->set_image_from_icon_name("go-bottom-symbolic", Gtk::ICON_SIZE_MENU);

    builder->get_widget<Gtk::ApplicationWindow>("win1", win);

    builder->get_widget<Gtk::Notebook>("notebook", notebook);
    notebook->signal_page_added().connect([](Gtk::Widget *w, guint page_num) {
        if(notebook->get_n_pages() == 1) {
            notebook->set_show_tabs(false);
        } else {
            notebook->set_show_tabs(true);
        }
    });
    notebook->signal_page_removed().connect([](Gtk::Widget *w, guint page_num) {
      if(notebook->get_n_pages() == 1) {
          notebook->set_show_tabs(false);
      } else {
          notebook->set_show_tabs(true);
      }
    });


    auto new_tab = [](Gtk::Notebook * notebook, std::string wd = "") {
        static int tabId = notebook->get_n_pages();
      auto *panned = new RootPanedContainer();
      auto tab = new Tab(panned, "Tab-" + std::to_string(tabId));
      panned->setId();
      panned->setRoot();
      auto termSess = new TerminalSession(tab, wd);
      panned->add1(termSess);
      tab->pack_start(*panned);
      tab->AddToNotebook(*notebook);
      panned->signal_terminal_num_changed().connect([tab](int num) {
        if(num == 0) {
            tab->Close();
        }
      });
      panned->show_all_children(true);
      notebook->show_all();
      notebook->set_focus_child(*tab);
      notebook->set_current_page(tabId);
      tabId ++;
    };

    but_new_tab->signal_clicked().connect([new_tab]() {
        new_tab(notebook);
    });
    new_tab(notebook);


    but_right_split->signal_clicked().connect([]() { SplitTerm(Gtk::ORIENTATION_HORIZONTAL); });
    but_below_split->signal_clicked().connect([]() { SplitTerm(Gtk::ORIENTATION_VERTICAL); });


    app->signal_command_line().connect([&](const Glib::RefPtr<Gio::ApplicationCommandLine> & cmd) -> int{
      const auto options = cmd->get_options_dict();
      if(!options) {
          FUN_INFO("not options found");
          return 0;
      }

      //Parse command-line arguments that were passed either to the primary (first) instance
      //or to subsequent instances.
      //Note that this parsing is happening in the primary (not local) instance.
      bool foo_value = false;
      Glib::ustring uri;
      if(auto ret = options->lookup_value("tab", uri); ret) {
          FUN_INFO("uri %s", uri.substr(7).c_str());
          new_tab(notebook, uri.substr(7));
      } else {
          FUN_INFO("uri not found");
      }
      app->activate();

      //The local instance will eventually exit with this status code:
      return EXIT_SUCCESS;

    }, false);

    //    headerbar = new Gtk::HeaderBar();
    win->set_titlebar(*headerbar);
    win->show_all();
    win->show_all_children();

    win->set_title("Fun Terminal");


    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new ();
    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER   (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const gchar* path_to_css = RES_FILE_DIR "/style.css";
    FUN_INFO("loading css file %s", path_to_css);
    GError *error = 0;
    gtk_css_provider_load_from_file(provider, g_file_new_for_path(path_to_css), &error);

    // return app->run(*win1, argc, argv);
    return app->run(*win, argc, argv);
}