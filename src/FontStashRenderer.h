#pragma once
#include <libtsm.h>

class FontStashRenderer {
  struct FONScontext *stash_ = nullptr;
  // glyph width, height
  float bounds[4] = {0, 0, 0, 0};
  float lineh = 0;
  float ascender = 0;
  float descender = 0;
  float advance = 0;
  int fontNormal = 0;

public:
  bool Initialize(const char *fontfile, float fh);
  void GlyphSize(float *w, float *h) const;
  void Update(float fh);
  void Clear(int width, int height, float r, float g, float b);
  static int draw_cb(struct tsm_screen *screen, uint32_t id, const uint32_t *ch,
                     size_t len, unsigned int cwidth, unsigned int posx,
                     unsigned int posy, const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);
};
