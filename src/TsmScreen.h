#pragma once
#include <libtsm.h>

class TsmScreen {
  struct tsm_screen *console = nullptr;
  struct tsm_vte *vte = nullptr;

  tsm_age_t screen_age = {};
  struct tsm_screen_attr attr;
  struct shl_pty *pty = nullptr;

public:
  TsmScreen();
  ~TsmScreen();
  void Launch();
  void Input(uint32_t keysym, uint32_t ascii, unsigned int mods,
             uint32_t unicode);
  void Resize(int cols, int rows);
  struct tsm_screen_attr *Dispatch();
  void Draw(class FontStashRenderer *stash);

  static void term_write_cb(struct tsm_vte *vtelocal, const char *u8,
                            size_t len, void *data);
  static void term_read_cb(struct shl_pty *pty, char *u8, size_t len,
                           void *data);
};
