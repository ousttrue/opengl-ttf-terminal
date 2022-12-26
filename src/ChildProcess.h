#pragma once
#include <stddef.h>

using term_reac_cb = void (*)(struct shl_pty *pty, char *u8, size_t len,
                              void *data);
class ChildProcess {
  struct shl_pty *pty = nullptr;

public:
  ~ChildProcess();
  void Launch(int cols, int rows, term_reac_cb on_read, void *data);
  void Resize(int cols, int rows);
  static void term_write_cb(struct tsm_vte *vtelocal, const char *u8,
                            size_t len, void *data);
  bool Dispatch();
};
