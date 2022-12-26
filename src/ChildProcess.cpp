#include "ChildProcess.h"
#include "shl_pty.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool closed = false;

static void io_handler(int s) { // TODO
}

static void hup_handler(int s) {
  printf("Signal received: %s\n", strsignal(s));
  closed = true;
  // exit(1);
}

using term_reac_cb = void (*)(struct shl_pty *pty, char *u8, size_t len,
                              void *data);
ChildProcess::~ChildProcess() { shl_pty_close(pty); }
void ChildProcess::Launch(int cols, int rows, term_reac_cb on_read,
                          void *data) {
  // this call will fork
  auto pid = shl_pty_open(&pty, on_read, data, cols, rows);
  //         tsm_screen_get_width(console),
  //  tsm_screen_get_height(console));

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

void ChildProcess::Resize(int cols, int rows) {
  shl_pty_resize(pty, cols, rows);
}

/* called when there is data to be written to the fd master -> slave  (vte
 * output) */
void ChildProcess::term_write_cb(struct tsm_vte *vtelocal, const char *u8,
                                 size_t len, void *data) {
  auto self = (ChildProcess *)data;
  auto r = shl_pty_write(self->pty, u8, len);
  if (r < 0) {
    printf("could not write to pty, %d\n", r);
  }
}

bool ChildProcess::Dispatch() {
  if (closed) {
    return false;
  }
  shl_pty_dispatch(pty);
  return true;
}
