

# FunTerm

A Linux Terminal Emulator

# About

This project is inspired by [tilix](https://github.com/gnunn1/tilix). Tilix is written with dlang.

# Download

deb packages for Ubuntu 22.04 and Ubuntu 20.04 can be downloaded at [github actions page](https://github.com/zhangfuwen/FunTerm/actions).


# Features

1. Search text with highlighting

      Implemented
      
2. Highlight custom text

      Implemented with flaws, you have to trigger a refresh(press enter, etc.) to see text highlighted.
      
3. Support tabs and split panes

      Implemented
      
4. Quick command notebook
      Not started yet.
      
      
# Dependencies     

This project depends on a slightly modified version of libvte, source code can be found [here](https://github.com/zhangfuwen/vte).

Modifications are meant to support text highlighting (which is officially supported by vte for now and I copied a patch from [here](https://gitlab.gnome.org/GNOME/gnome-terminal/-/issues/7771#note_1175694).

# Contribution

Feel free to send me a pull request, whether it being a refactor(maybe huge, since current code is mess), a feature enhancement or fixing a spelling error.

# 中文Readme

注意： 这不是一个严肃的项目。

这是一个Linux Terminal。主要是现有terminal还有一些用着不太方便的地方，自己尝试去改发现很难。

我个人比较关注的特性：

1. 支持多tab, 窗口切分
2. 支持文字高亮，目前gnome系的terminal好像都不支持，因为libvte不支持。我在gitlab上看它libvte的作者提了一个patch，但没有合到主线，我就拿来改了改。
3. 支持快捷命令菜单，有些命令不常用，但又比较长，不想每次各个地方找，直接集成在terminal上会比较方便，这个功能还没做。

目前项目代码比较乱，很多地方有已知的内存泄漏（主要是gtk做的ui那里，我也不会gtk ui开发）。

欢迎有兴趣的同学贡献代码，什么代码都行，小到修改comment，大到完全的架构重构。由于这不是一个严肃的项目，所以commit message没有规范、想加什么feature也都可以加。

# 下载

[这里](https://github.com/zhangfuwen/FunTerm/actions) 有Ubuntu 20.04和Ubuntu 22.04的包。但是我没验证过能不能用 :)。欢迎反馈。
            
