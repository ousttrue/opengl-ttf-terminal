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

#include "ChildProcess.h"
#include "FontStashRenderer.h"
#include "Renderer.h"
#include "TsmScreen.h"
#include "args.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <external/xkbcommon-keysyms.h>
#include <stdio.h>

static uint32_t GlfwToXkb(int key) {
  switch (key) {
  case GLFW_KEY_ENTER:
    return XKB_KEY_Return;
  case GLFW_KEY_UP:
    return XKB_KEY_Up;
  case GLFW_KEY_DOWN:
    return XKB_KEY_Down;
  case GLFW_KEY_LEFT:
    return XKB_KEY_Left;
  case GLFW_KEY_RIGHT:
    return XKB_KEY_Right;
  default:
    return 0;
  }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mod) {
  auto screen = (TsmScreen *)glfwGetWindowUserPointer(window);

  if (action != GLFW_PRESS) {
    return;
  }

  uint32_t mods = 0;
  if (mod & GLFW_MOD_CONTROL)
    mods |= TSM_CONTROL_MASK;
  if (mod & GLFW_MOD_SHIFT)
    mods |= TSM_SHIFT_MASK;
  if (mod & GLFW_MOD_ALT)
    mods |= TSM_ALT_MASK;
  if (mod & GLFW_MOD_SUPER)
    mods |= TSM_LOGO_MASK;

  /* map cursor keys to XKB scancodes to be escaped by libtsm vte */
  uint32_t keysym = GlfwToXkb(key);

  printf("key: %d, scancode: %d\n", key, scancode);
  screen->Input(keysym, 0, mods, 0);
}

void character_callback(GLFWwindow *window, unsigned int codepoint) {
  auto screen = (TsmScreen *)glfwGetWindowUserPointer(window);
  printf("unicode: 0x%04x\n", codepoint);
  screen->Input(0, 0, 0, codepoint);
}

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
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, character_callback);
  glfwMakeContextCurrent(window);

  ChildProcess child;
  TsmScreen screen(&ChildProcess::term_write_cb, &child);
  screen.Resized = [&child](int cols, int rows) { child.Resize(cols, rows); };
  child.Launch(screen.Cols(), screen.Rows(), &TsmScreen::term_read_cb, &screen);
  glfwSetWindowUserPointer(window, &screen);

  FontStashRenderer stash;
  if (!stash.Initialize(args.fontfile, args.fh)) {
    return 3;
  }

  float glyph_width;
  float glyph_height;
  stash.GlyphSize(&glyph_width, &glyph_height);

  Renderer renderer;

  while (!glfwWindowShouldClose(window)) {
    if (!child.Dispatch()) {
      break;
    }

    glfwPollEvents();
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    screen.Resize((width / glyph_width), (height / glyph_height) - 1);

    // render
    auto attr = screen.Get();

    renderer.Render(width, height, attr->br, attr->bg, attr->bb);

    stash.Update(args.fh);
    screen.Draw(&stash);

    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}
