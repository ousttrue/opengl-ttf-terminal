# Terminal Window

A VT100 type terminal emulator that renders a TrueType font to an OpenGL window.

This is made possible using open source code:

* [Fontstash][] for truetype font loading
* [stb][] for truetype font rendering
* [libtsm][] for terminal emulation
* [libsdl][] for window creation and keyboard event handling
* [opengl][] for scene rendering

![Screenshot](/screen.png?raw=true)

Links:

* [Fontstash]: https://github.com/memononen/fontstash
* [stb]: https://github.com/nothings/stb
* [libtsm]: https://github.com/syuu1228/libtsm.git
* [libsdl]: https://www.libsdl.org
* [opengl]: https://www.opengl.org


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
