#include "FontStashRenderer.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

static void fonsError(void *uptr, int error, int val) {
  printf("FONT ERROR: %d --- %d\n", error, val);
  exit(-1);
}

bool FontStashRenderer::Initialize(const char *fontfile, float fh) {
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

int FontStashRenderer::draw_cb(struct tsm_screen *screen, uint32_t id,
                               const uint32_t *ch, size_t len,
                               unsigned int cwidth, unsigned int posx,
                               unsigned int posy,
                               const struct tsm_screen_attr *attr,
                               tsm_age_t age, void *data) {

  auto self = (FontStashRenderer *)data;

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
    fonsSetColor(self->stash_, color);
    for (i = 0; i < len; i += cwidth) {
      sprintf(buf, "%c", ch[i]);
      dx = fonsDrawText(self->stash_, dx,
                        dy + self->ascender /*((bounds[2]-bounds[0]))*/, buf,
                        NULL);
    }
  }
  return 0;
}
