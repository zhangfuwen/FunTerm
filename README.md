

# FunTerm

A Linux Terminal Emulator

# About

This project is inspired by [tilix](https://github.com/gnunn1/tilix). Tilix is written with dlang.


# Features

[] 1. Search text with highlighting
      Implemented
[] 2. Highlight custom text
      Implemented with flaws, you have to trigger a refresh(press enter, etc.) to see text highlighted.
[] 3. Support tabs and split panes
      Implemented
[] 4. Quick command notebook
      Not started yet.
      
      
# Dependencies     

This project depends on a slightly modified version of libvte, source code can be found [here](https://github.com/zhangfuwen/vte).

Modifications are meant to support text highlighting (which is officially supported by vte for now and I copied a patch from [here](https://gitlab.gnome.org/GNOME/gnome-terminal/-/issues/7771#note_1175694).

# Contribution

Feel free to send me a pull request, whether it being a refactor(maybe huge, since current code is mess), a feature enhancement or fixing a spelling error.
            
