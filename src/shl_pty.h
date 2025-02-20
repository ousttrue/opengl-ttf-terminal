/*
 * SHL - PTY Helpers
 *
 * Copyright (c) 2011-2013 David Herrmann <dh.herrmann@gmail.com>
 * Dedicated to the Public Domain
 */

/*
 * PTY Helpers
 */

#ifndef SHL_PTY_H
#define SHL_PTY_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pty */

struct shl_pty;

typedef void (*shl_pty_input_cb) (shl_pty *pty, char *u8,
				  size_t len, void *data);

pid_t shl_pty_open(shl_pty **out,
		   shl_pty_input_cb cb,
		   void *data,
		   unsigned short term_width,
		   unsigned short term_height);
void shl_pty_ref(shl_pty *pty);
void shl_pty_unref(shl_pty *pty);
void shl_pty_close(shl_pty *pty);

bool shl_pty_is_open(shl_pty *pty);
int shl_pty_get_fd(shl_pty *pty);
pid_t shl_pty_get_child(shl_pty *pty);

int shl_pty_dispatch(shl_pty *pty);
int shl_pty_write(shl_pty *pty, const char *u8, size_t len);
int shl_pty_signal(shl_pty *pty, int sig);
int shl_pty_resize(shl_pty *pty,
		   unsigned short term_width,
		   unsigned short term_height);

/* pty bridge */

int shl_pty_bridge_new(void);
void shl_pty_bridge_free(int bridge);

int shl_pty_bridge_dispatch(int bridge, int timeout);
int shl_pty_bridge_add(int bridge, shl_pty *pty);
void shl_pty_bridge_remove(int bridge, shl_pty *pty);

#endif  /* SHL_PTY_H */
