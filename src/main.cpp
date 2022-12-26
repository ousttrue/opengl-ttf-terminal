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

#include "FontStashRenderer.h"
#include "TsmScreen.h"
#include "args.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

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

  FontStashRenderer stash;
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
