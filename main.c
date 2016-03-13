/* experimental terminal window using libSDL, OpenGL, stb_truetype, fontstash, and libtsm
 * Copyright (c) 2016 A. Carl Douglas
 */


//
// Copyright (c) 2009 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "fontstash.h"

#include "libtsm.h"
#include "shl-pty.h"
#include "external/xkbcommon-keysyms.h"

extern char **environ;

int fh = 21; /* font height */

struct tsm_screen *console;
struct tsm_vte *vte;
tsm_age_t screen_age;
struct shl_pty *pty;

void io_handler(int s) {
  SDL_Event e;
  e.type = SDL_USEREVENT;
  SDL_PushEvent(&e);
}

/* called when data has been read from the fd slave -> master  (vte input )*/
static void term_read_cb(struct shl_pty *pty, void *data, char *u8, size_t len) {
  tsm_vte_input(vte, u8, len);
}

/* called when there is data to be written to the fd master -> slave  (vte output) */
static void term_write_cb(struct tsm_vte *vtelocal, const char *u8, size_t len, void *data) {
  int r;
  r = shl_pty_write(pty, u8, len);
  if (r < 0) {
    printf ("could not write to pty, %d\n", r);
  }
}

static void log_tsm(void *data, const char *file, int line, const char *fn,
                    const char *subs, unsigned int sev, const char *format,
                    va_list args)
{
  fprintf(stderr, "%d: %s: ", sev, subs);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}

static int draw_cb(struct tsm_screen *screen, uint32_t id,
                   const uint32_t *ch, size_t len,
                   unsigned int cwidth, unsigned int posx,
                   unsigned int posy,
                   const struct tsm_screen_attr *attr,
                   tsm_age_t age, void *data)
{
  int i;
  int h;
  int lh=fh;
  int lw=(fh / 2);
  float dx=posx*lw, dy=(posy*lh);
  char buf[32];
  uint8_t fr, fg, fb, br, bg, bb;
  h = SDL_GetVideoInfo()->current_h;
  dy = h-(posy*fh)-fh;
  if (attr->inverse) {
    fr = attr->br; fg = attr->bg; fb = attr->bb; br = attr->fr; bg = attr->fg; bb = attr->fb;
  } else {
    fr = attr->fr; fg = attr->fg; fb = attr->fb; br = attr->br; bg = attr->bg; bb = attr->bb;
  }
  if (!len) {
    glColor4ub(br,bg,bb,255);
    glPolygonMode(GL_FRONT, GL_FILL);
    glRectf(dx+0,dy+0,dx+lw,dy+lh);
  } else {
    glColor4ub(br,bg,bb,255);
    glPolygonMode(GL_FRONT, GL_FILL);
    glRectf(dx+0,dy+0,dx+lw,dy+lh);
    glColor4ub(fr,fg,fb,255);
    for (i=0; i < len;i+=cwidth)  {
      sprintf(buf,"%c",ch[i]);
      sth_begin_draw(data);
      sth_draw_text(data, 0, fh, dx, dy + (fh/4), buf, & dx );
      sth_end_draw(data);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int done;
  SDL_Event event;
  SDL_Surface* screen;
  const SDL_VideoInfo* vi;
  int width,height;
  struct sth_stash* stash = 0;
  pid_t pid;
  unsigned int ticks;
  int opt;
  char *fontfile = "VeraMono.ttf";
  while ((opt = getopt(argc, argv, "f:s:")) != -1) {
       switch (opt) {
       case 'f':
           fontfile = optarg;
           break;
       case 's':
           fh = atoi(optarg);
           break; 
       default: /* '?' */
           fprintf(stderr, "Usage: %s [-f ttf file] [-s font size]\n", argv[0]);
           exit(EXIT_FAILURE);
       }
   }


  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    return -1;
  }
  ticks = SDL_GetTicks();

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  
  vi = SDL_GetVideoInfo();
  width = vi->current_w - 20;
  height = vi->current_h - 80;
  screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL);
  if (!screen)
  {
    printf("Could not initialise SDL opengl\n");
    return -1;
  }
  SDL_EnableUNICODE(1);  /* for .keysym.unicode */
  SDL_WM_SetCaption("libtsm/fontstash terminal", 0);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  stash = sth_create(512,512);
  if (!stash) {
    printf("Could not create stash.\n");
    return -1;
  }
  if (!sth_add_font(stash,0, fontfile)) {
    printf("Could not add font.\n");
    return -1;
  }

  tsm_screen_new(&console, log_tsm, 0);
  tsm_vte_new(&vte, console, term_write_cb, 0, log_tsm, 0);
  tsm_screen_resize(console, (width / (fh / 2))-1, (height / fh)-1);
  assert(vte != 0);
  printf("console width: %d\n", tsm_screen_get_width(console));
  printf("console height: %d\n", tsm_screen_get_height(console));

  /* this call will fork */
  pid = shl_pty_open(&pty,
                   term_read_cb,
                   NULL,
                   tsm_screen_get_width(console),
                   tsm_screen_get_height(console));

  if (pid < 0) {
    perror("fork problem");
  } else if (pid != 0 ) {
    /* parent, pty master */
    int fd = shl_pty_get_fd(pty);
    unsigned oflags;
    printf("OpenGL/TTF Terminal [%d] [%d]\n", pid, fd);
#if 1
    /* enable SIGIO signal for this process when it has a ready file descriptor */
    signal(SIGIO, &io_handler);
    fcntl(shl_pty_get_fd(pty), F_SETOWN, getpid(  ));
    oflags = fcntl(shl_pty_get_fd(pty), F_GETFL);
    fcntl(shl_pty_get_fd(pty), F_SETFL, oflags | FASYNC);
#endif
  } else {
    char **argv = (char*[]) {
                getenv("SHELL") ? : "/bin/bash",
                NULL
        };
    int r;
    setenv("TERM", "vt100", 1);
    printf("OpenGL-TTF terminal, %s\n", argv[0]);
    r = execve(argv[0], argv, environ);
    if (r < 0) { perror("execve failed"); }
    exit(1);
  }
  done = 0;
  while (!done) {
    struct tsm_screen_attr attr;
    if (SDL_WaitEvent(&event)) {
      SDL_keysym k = event.key.keysym;
      unsigned int mods = 0;
      unsigned int scancode = k.scancode;
      switch (event.type) {
        case SDL_MOUSEMOTION:
          break;
        case SDL_MOUSEBUTTONDOWN:
          break;
        case SDL_KEYDOWN:
          if (k.mod & KMOD_CTRL)  mods |= TSM_CONTROL_MASK;
          if (k.mod & KMOD_SHIFT) mods |= TSM_SHIFT_MASK;
          if (k.mod & KMOD_ALT)   mods |= TSM_ALT_MASK;
          if (k.mod & KMOD_META)  mods |= TSM_LOGO_MASK;
          /* map cursor keys to XKB scancodes to be escaped by libtsm vte */
          if (k.sym == SDLK_UP) scancode = XKB_KEY_Up;
          if (k.sym == SDLK_DOWN) scancode = XKB_KEY_Down;
          if (k.sym == SDLK_LEFT) scancode = XKB_KEY_Left;
          if (k.sym == SDLK_RIGHT) scancode = XKB_KEY_Right;
          if(k.unicode != 0 || 
              (scancode==XKB_KEY_Up || 
                scancode==XKB_KEY_Down || 
                scancode==XKB_KEY_Left || 
                scancode==XKB_KEY_Right)) {
            /* only handle when there's non-zero unicode keypress... found using vim */
            /*printf("scancode: %d  sym: %d  unicode: %d\n", scancode, k.sym, k.unicode);*/
            tsm_vte_handle_keyboard(vte, scancode, k.sym, mods, k.unicode);
          }
          break;
        /*case SDL_TEXTINPUT:*/
          break;
        case SDL_QUIT:
          done = 1;
          break;
        case SDL_USEREVENT: /* sigio - something to read */
          break;
      }
    }

    {
      int r = shl_pty_dispatch(pty);
      if (r == -5 ) {
        printf("%d: %s\n", r, strerror(r));
        perror("pty error: ");
        done = 1;
      }
    }

    if (SDL_GetTicks() <= (ticks+50))
      continue;
    ticks= SDL_GetTicks();

    tsm_vte_get_def_attr(vte, &attr);
    glViewport(0, 0, width, height);
    glClearColor(attr.br, attr.bg, attr.bb, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#if 1
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,width,0,height,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor4ub(attr.fr,attr.fg,attr.fb,255);
#if 0
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif
 
    screen_age = tsm_screen_draw(console, draw_cb, stash);
    glEnable(GL_DEPTH_TEST);
    
    SDL_GL_SwapBuffers();
  }
  shl_pty_close(pty);
  
  SDL_Quit();
  return 0;
}
