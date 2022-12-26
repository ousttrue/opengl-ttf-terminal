#include "TsmScreen.h"
#include "FontStashRenderer.h"
#include <cstdint>
#include <stdio.h>
#include <string.h>

extern char **environ;

static void log_tsm(void *data, const char *file, int line, const char *fn,
                    const char *subs, unsigned int sev, const char *format,
                    va_list args) {
  fprintf(stderr, "%d: %s: ", sev, subs);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}

/* called when data has been read from the fd slave -> master  (vte input )*/
void TsmScreen::term_read_cb(struct shl_pty *pty, char *u8, size_t len,
                             void *data) {
  auto self = (TsmScreen *)data;
  tsm_vte_input(self->vte, u8, len);
}

TsmScreen::TsmScreen(tsm_vte_write_cb on_write, void *data) {
  tsm_screen_new(&console, log_tsm, 0);
  tsm_vte_new(&vte, console, on_write, data, log_tsm, 0);
}
TsmScreen::~TsmScreen() {}

void TsmScreen::Input(uint32_t keysym, uint32_t ascii, unsigned int mods,
                      uint32_t unicode) {
  tsm_vte_handle_keyboard(vte, keysym, ascii, mods, unicode);
}

int TsmScreen::Cols() const { return tsm_screen_get_width(console); }
int TsmScreen::Rows() const { return tsm_screen_get_height(console); }
void TsmScreen::Resize(int cols, int rows) {
  tsm_screen_resize(console, cols, rows);
  Resized(Cols(), Rows());
  printf("console width: %d\n", Cols());
  printf("console height: %d\n", Rows());
}

struct tsm_screen_attr *TsmScreen::Get() {
  tsm_vte_get_def_attr(vte, &attr);
  return &attr;
}

void TsmScreen::Draw(FontStashRenderer *stash) {
  screen_age = tsm_screen_draw(console, &FontStashRenderer::draw_cb, stash);
}
