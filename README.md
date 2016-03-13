# Terminal Window

A VT100 type terminal emulator that renders a TrueType font to an OpenGL window.

This is made possible using open source code:

* [https://github.com/memononen/fontstash] for truetype font loading
* [https://github.com/nothings/stb] for truetype font rendering
* [https://www.freedesktop.org/wiki/Software/kmscon/libtsm/] for terminal emulation
* [https://www.libsdl.org/] for window creation and keyboard event handling
* [https://www.opengl.org/] for scene rendering


## Build instructions

On Linux,

    git submodule init
    git submodule update
    make

And run,

    ./togl



## Known issues

* It's slow...



2016 A. Carl Douglas
