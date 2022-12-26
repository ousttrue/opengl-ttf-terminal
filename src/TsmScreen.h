#pragma once
#include <functional>
#include <libtsm.h>

class TsmScreen {
  struct tsm_screen *console = nullptr;
  struct tsm_vte *vte = nullptr;

  tsm_age_t screen_age = {};
  struct tsm_screen_attr attr;

public:
  std::function<void(int cols, int rows)> Resized;
  TsmScreen(tsm_vte_write_cb on_write, void *data);
  ~TsmScreen();
  void Input(uint32_t keysym, uint32_t ascii, unsigned int mods,
             uint32_t unicode);
  void Resize(int cols, int rows);
  int Cols() const;
  int Rows() const;
  struct tsm_screen_attr *Get();
  void Draw(tsm_screen_draw_cb draw_cb, void *data);
  static void term_read_cb(struct shl_pty *pty, char *u8, size_t len,
                           void *data);
};
