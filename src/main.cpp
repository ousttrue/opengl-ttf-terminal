/* experimental terminal window using libSDL, OpenGL, stb_truetype, fontstash,
 * and libtsm Copyright (c) 2016 A. Carl Douglas
 */

//
// Copyright (c) 2009 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "args.h"
#include <GLFW/glfw3.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

#include "external/xkbcommon-keysyms.h"
#include "libtsm.h"
#include "shl_pty.h"

extern char **environ;

class FontStash {
  FONScontext *stash_ = nullptr;
  // glyph width, height
  float bounds[4] = {0, 0, 0, 0};
  float lineh = 0;
  float ascender = 0;
  float descender = 0;
  float advance = 0;
  int fontNormal = 0;

public:
  bool Initialize(const char *fontfile, float fh) {
    stash_ = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    if (!stash_) {
      printf("Could not create stash.\n");
      return false;
    }

    fonsSetErrorCallback(stash_, fonsError, stash_);

    fontNormal = fonsAddFont(stash_, "sans", fontfile);
    if (fontNormal == FONS_INVALID) {
      printf("Could not add font.\n");
      return -1;
    }
    /* measure a character to determine the glyph box size */
    fonsClearState(stash_);
    fonsSetFont(stash_, fontNormal);
    fonsSetSize(stash_, fh);
    advance = fonsTextBounds(stash_, 0, 0, "W", NULL, bounds);
    fonsVertMetrics(stash_, &ascender, &descender, &lineh);

    return true;
  }

  static void fonsError(void *uptr, int error, int val) {
    printf("FONT ERROR: %d --- %d\n", error, val);
    exit(-1);
  }

  void GlyphSize(float *w, float *h) const {
    *w = (bounds[2] - bounds[0]) - 1;
    *h = lineh;
  }

  void Update(float fh) {
    fonsClearState(stash_);
    fonsSetFont(stash_, fontNormal);
    fonsSetSize(stash_, fh);
  }

  static int draw_cb(struct tsm_screen *screen, uint32_t id, const uint32_t *ch,
                     size_t len, unsigned int cwidth, unsigned int posx,
                     unsigned int posy, const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data) {

    auto self = (FontStash *)data;

    int i;
    int lh = self->lineh;
    int lw = (self->bounds[2] - self->bounds[0]);
    float dx = posx * lw, dy = posy * lh;
    char buf[32];
    uint8_t fr, fg, fb, br, bg, bb;
    unsigned int color;
    if (attr->inverse) {
      fr = attr->br;
      fg = attr->bg;
      fb = attr->bb;
      br = attr->fr;
      bg = attr->fg;
      bb = attr->fb;
    } else {
      fr = attr->fr;
      fg = attr->fg;
      fb = attr->fb;
      br = attr->br;
      bg = attr->bg;
      bb = attr->bb;
    }

    if (!len) {
      glColor4ub(br, bg, bb, 255);
      glPolygonMode(GL_FRONT, GL_FILL);
      glRectf(dx + lw, dy, dx, dy + lh);
    } else {
      glColor4ub(br, bg, bb, 255);
      glPolygonMode(GL_FRONT, GL_FILL);
      glRectf(dx + lw, dy, dx, dy + lh);

      color = glfonsRGBA(fr, fg, fb, 255);
      fonsSetColor((FONScontext *)data, color);
      for (i = 0; i < len; i += cwidth) {
        sprintf(buf, "%c", ch[i]);
        dx = fonsDrawText((FONScontext *)data, dx,
                          dy + self->ascender /*((bounds[2]-bounds[0]))*/, buf,
                          NULL);
      }
    }
    return 0;
  }
};

class TsmScreen {
  struct tsm_screen *console = nullptr;
  struct tsm_vte *vte = nullptr;

  struct shl_pty *pty = nullptr;
  tsm_age_t screen_age = {};

public:
  TsmScreen() {
    tsm_screen_new(&console, log_tsm, 0);
    tsm_vte_new(&vte, console, term_write_cb, this, log_tsm, 0);
  }
  ~TsmScreen() { shl_pty_close(pty); }

  void Launch() {
    // this call will fork
    auto pid =
        shl_pty_open(&pty, term_read_cb, this, tsm_screen_get_width(console),
                     tsm_screen_get_height(console));

    if (pid < 0) {
      perror("fork problem");
    } else if (pid != 0) {
      /* parent, pty master */
      int fd = shl_pty_get_fd(pty);
      unsigned oflags = 0;
      /* enable SIGIO signal for this process when it has a ready file
       * descriptor
       */
      signal(SIGIO, &io_handler);
      fcntl(fd, F_SETOWN, getpid());
      oflags = fcntl(fd, F_GETFL);
      fcntl(fd, F_SETFL, oflags | FASYNC);
      /* watch for SIGHUP */
      signal(SIGCHLD, &hup_handler);
    } else {
      /* child, shell */
      auto shell = getenv("SHELL") ?: "/bin/bash";
      const char *argv[] = {shell, NULL};
      execve(argv[0], (char **)argv, environ);
      /* never reached except on execve error */
      perror("execve error");
      exit(-2);
    }
  }

  static void io_handler(int s) {
    // TODO
  }

  static void hup_handler(int s) {
    printf("Signal received: %s\n", strsignal(s));
    exit(1);
  }

  /* called when data has been read from the fd slave -> master  (vte input )*/
  static void term_read_cb(struct shl_pty *pty, char *u8, size_t len,
                           void *data) {
    auto self = (TsmScreen *)data;
    tsm_vte_input(self->vte, u8, len);
  }

  void Resize(int cols, int rows) {
    tsm_screen_resize(console, cols, rows);
    shl_pty_resize(pty, tsm_screen_get_width(console),
                   tsm_screen_get_height(console));

    printf("console width: %d\n", tsm_screen_get_width(console));
    printf("console height: %d\n", tsm_screen_get_height(console));
  }

  static void log_tsm(void *data, const char *file, int line, const char *fn,
                      const char *subs, unsigned int sev, const char *format,
                      va_list args) {
    fprintf(stderr, "%d: %s: ", sev, subs);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
  }

  /* called when there is data to be written to the fd master -> slave  (vte
   * output) */
  static void term_write_cb(struct tsm_vte *vtelocal, const char *u8,
                            size_t len, void *data) {
    auto self = (TsmScreen *)data;
    auto r = shl_pty_write(self->pty, u8, len);
    if (r < 0) {
      printf("could not write to pty, %d\n", r);
    }
  }

  struct tsm_screen_attr attr;
  struct tsm_screen_attr *Dispatch() {
    shl_pty_dispatch(pty);
    tsm_vte_get_def_attr(vte, &attr);
    return &attr;
  }

  void Draw(FontStash *stash) {
    screen_age = tsm_screen_draw(console, FontStash::draw_cb, stash);
  }
};

int main(int argc, char *argv[]) {
  Args args;
  args.parse(argc, argv);

  if (!glfwInit()) {
    fprintf(stderr, "glfwInit\n");
    return 1;
  }
  GLFWwindow *window =
      glfwCreateWindow(args.width, args.height, "libtsm", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return 2;
  }
  glfwMakeContextCurrent(window);

  TsmScreen screen;
  screen.Launch();

  FontStash stash;
  if (!stash.Initialize(args.fontfile, args.fh)) {
    return 3;
  }

  float glyph_width;
  float glyph_height;
  stash.GlyphSize(&glyph_width, &glyph_height);

  screen.Resize((args.width / glyph_width), (args.height / glyph_height) - 1);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // if (SDL_WaitEvent(&event)) {
    //   SDL_keysym k;
    //   unsigned int mods = 0;
    //   unsigned int scancode = 0;
    //   switch (event.type) {
    //   case SDL_MOUSEMOTION:
    //     break;
    //   case SDL_MOUSEBUTTONDOWN:
    //     break;
    //   case SDL_KEYDOWN:
    //     k = event.key.keysym;
    //     scancode = k.scancode;
    //     if (k.mod & KMOD_CTRL)
    //       mods |= TSM_CONTROL_MASK;
    //     if (k.mod & KMOD_SHIFT)
    //       mods |= TSM_SHIFT_MASK;
    //     if (k.mod & KMOD_ALT)
    //       mods |= TSM_ALT_MASK;
    //     if (k.mod & KMOD_META)
    //       mods |= TSM_LOGO_MASK;
    //     /* map cursor keys to XKB scancodes to be escaped by libtsm vte */
    //     if (k.sym == SDLK_UP)
    //       scancode = XKB_KEY_Up;
    //     if (k.sym == SDLK_DOWN)
    //       scancode = XKB_KEY_Down;
    //     if (k.sym == SDLK_LEFT)
    //       scancode = XKB_KEY_Left;
    //     if (k.sym == SDLK_RIGHT)
    //       scancode = XKB_KEY_Right;
    //     if (k.unicode != 0 ||
    //         (scancode == XKB_KEY_Up || scancode == XKB_KEY_Down ||
    //          scancode == XKB_KEY_Left || scancode == XKB_KEY_Right)) {
    //       /* only handle when there's non-zero unicode keypress... found
    //       using
    //        * vim */
    //       /*printf("scancode: %d  sym: %d  unicode: %d\n", scancode, k.sym,
    //        * k.unicode);*/
    //       tsm_vte_handle_keyboard(vte, scancode, k.sym, mods, k.unicode);
    //     }
    //     break;
    //     /*case SDL_TEXTINPUT:*/
    //     break;
    //   case SDL_QUIT:
    //     done = 1;
    //     break;
    //   case SDL_USEREVENT: /* sigio - something to read */
    //     break;
    //   case SDL_ACTIVEEVENT:
    //     break;
    //   }
    // }

    // render
    auto attr = screen.Dispatch();
    glViewport(0, 0, args.width, args.height);
    glClearColor(attr->br, attr->bg, attr->bb, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, args.width, args.height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    // glColor4ub(255,255,255,255);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    stash.Update(args.fh);
    screen.Draw(&stash);

    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}
