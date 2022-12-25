# Terminal Window

A VT100 type terminal emulator that renders a TrueType font to an OpenGL window.

This is made possible using open source code:

* [Fontstash][1] for truetype font loading
* [stb][2] for truetype font rendering
* [libtsm][3] for terminal emulation
* [libsdl][4] for window creation and keyboard event handling
* [opengl][5] for scene rendering

[1]: https://github.com/memononen/fontstash
[2]: https://github.com/nothings/stb
[3]: https://github.com/syuu1228/libtsm.git
[4]: https://www.libsdl.org
[5]: https://www.opengl.org

![Screenshot](/screen.png?raw=true)

## Build instructions

* replace SDL-1.2 to latest GLFW
* cmake build

On Linux,

    git submodule init
    git submodule update
    cmake -S . -B build
    cmake --build build

And run,

    ./build/togl

## Known issues

* It's slow...

2016 A. Carl Douglas
