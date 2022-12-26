#include "FontStashRenderer.h"
#include <GL/gl.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string_view>
#include <unordered_set>

#include "fontstash.h"
#include "glfontstash.h"

static void fonsError(void *uptr, int error, int val) {
  printf("FONT ERROR: %d --- %d\n", error, val);
  exit(-1);
}

bool FontStashRenderer::Initialize(const char *fontfile, float fh) {
  std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "GL_VERNDOR: " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;

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

void FontStashRenderer::GlyphSize(float *w, float *h) const {
  *w = (bounds[2] - bounds[0]) - 1;
  *h = lineh;
}

void FontStashRenderer::Update(float fh) {
  fonsClearState(stash_);
  fonsSetFont(stash_, fontNormal);
  fonsSetSize(stash_, fh);
}

void FontStashRenderer::Clear(int width, int height, float r, float g,
                              float b) {
  glViewport(0, 0, width, height);
  glClearColor(r, g, b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  // glColor4ub(255,255,255,255);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
}

std::unordered_set<uint32_t> used_;

char c[5] = {0};
static std::string_view cp2utf8(uint32_t cp) {
  if (cp <= 0x7F) {
    c[0] = cp;
    c[1] = 0;
    return {c, 1};
  } else if (cp <= 0x7FF) {
    c[0] = (cp >> 6) + 192;
    c[1] = (cp & 63) + 128;
    c[2] = 0;
    return {c, 2};
  } else if (0xd800 <= cp && cp <= 0xdfff) {
  } // invalid block of utf8
  else if (cp <= 0xFFFF) {
    c[0] = (cp >> 12) + 224;
    c[1] = ((cp >> 6) & 63) + 128;
    c[2] = (cp & 63) + 128;
    c[3] = 0;
    return {c, 3};
  } else if (cp <= 0x10FFFF) {
    c[0] = (cp >> 18) + 240;
    c[1] = ((cp >> 12) & 63) + 128;
    c[2] = ((cp >> 6) & 63) + 128;
    c[3] = (cp & 63) + 128;
    c[4] = 0;
    return {c, 4};
  } else {
  }
  throw std::runtime_error("invalid cp");
}

int FontStashRenderer::draw_cb(struct tsm_screen *screen, uint32_t id,
                               const uint32_t *ch, size_t len,
                               unsigned int cwidth, unsigned int posx,
                               unsigned int posy,
                               const struct tsm_screen_attr *attr,
                               tsm_age_t age, void *data) {

  auto self = (FontStashRenderer *)data;

  uint8_t fr, fg, fb, br, bg, bb;
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

  // rect
  int lh = self->lineh;
  int lw = (self->bounds[2] - self->bounds[0]);
  float dx = posx * lw;
  float dy = posy * lh;

  // clear bg color
  glColor4ub(br, bg, bb, 255);
  glPolygonMode(GL_FRONT, GL_FILL);
  glRectf(dx + lw, dy, dx, dy + lh);

  if (len) {
    auto color = glfonsRGBA(fr, fg, fb, 255);
    fonsSetColor(self->stash_, color);
    for (int i = 0; i < len; i += cwidth) {
      // unicode code points
      auto found = used_.find(ch[i]);
      auto utf8 = cp2utf8(ch[i]);
      if (found == used_.end()) {
        used_.insert(ch[i]);
        auto char_type = "unknown";
        if (ch[i] < 128) {
          char_type = "ascii";
        } else {
          auto a = 0;
        }
        printf("U+%04x: [%s] %s\n", ch[i], char_type, utf8.data());
      }
      dx = fonsDrawText(self->stash_, dx,
                        dy + self->ascender /*((bounds[2]-bounds[0]))*/,
                        utf8.begin(), utf8.end());
    }
  }
  return 0;
}
