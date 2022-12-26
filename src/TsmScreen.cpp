#include "TsmScreen.h"
#include "FontStashRenderer.h"
#include "shl_pty.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

extern char **environ;

static void io_handler(int s) {
  // TODO
}

static void hup_handler(int s) {
  printf("Signal received: %s\n", strsignal(s));
  exit(1);
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
void TsmScreen::term_write_cb(struct tsm_vte *vtelocal, const char *u8,
                              size_t len, void *data) {
  auto self = (TsmScreen *)data;
  auto r = shl_pty_write(self->pty, u8, len);
  if (r < 0) {
    printf("could not write to pty, %d\n", r);
  }
}

/* called when data has been read from the fd slave -> master  (vte input )*/
void TsmScreen::term_read_cb(struct shl_pty *pty, char *u8, size_t len,
                             void *data) {
  auto self = (TsmScreen *)data;
  tsm_vte_input(self->vte, u8, len);
}

TsmScreen::TsmScreen() {
  tsm_screen_new(&console, log_tsm, 0);
  tsm_vte_new(&vte, console, term_write_cb, this, log_tsm, 0);
}
TsmScreen::~TsmScreen() { shl_pty_close(pty); }

void TsmScreen::Launch() {
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

void TsmScreen::Resize(int cols, int rows) {
  tsm_screen_resize(console, cols, rows);
  shl_pty_resize(pty, tsm_screen_get_width(console),
                 tsm_screen_get_height(console));

  printf("console width: %d\n", tsm_screen_get_width(console));
  printf("console height: %d\n", tsm_screen_get_height(console));
}

struct tsm_screen_attr *TsmScreen::Dispatch() {
  shl_pty_dispatch(pty);
  tsm_vte_get_def_attr(vte, &attr);
  return &attr;
}

void TsmScreen::Draw(FontStashRenderer *stash) {
  screen_age = tsm_screen_draw(console, &FontStashRenderer::draw_cb, stash);
}
