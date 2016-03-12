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
#include <assert.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "libtsm.h"
#include "fontstash.h"
#include "shl-pty.h"

#define FONT_DIR  "/usr/share/fonts/TTF/"

extern char **environ;


struct tsm_screen *console;
struct tsm_vte *vte;
tsm_age_t age;

struct shl_pty *pty = 0;

static void term_read_cb(struct shl_pty *pty,
                                  void *data,
                                  char *u8,
                                  size_t len) {
  /*struct tsm_vte *vte = (struct tsm_vte *)data;*/
  printf("in read cb: %c\n", *u8);
  tsm_vte_input(vte, u8, len);
}

static void log_tsm(void *data, const char *file, int line, const char *fn,
                    const char *subs, unsigned int sev, const char *format,
                    va_list args)
{
        fprintf(stderr, "%d: %s: ", sev, subs);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
}

static void term_write_cb(struct tsm_vte *vtelocal, const char *u8, size_t len, void *data) {
  int r;
  /*struct shl_pty * pty = (struct shl_pty *)data;*/
  printf("vte callback: %c\n", *u8);
  /*tsm_vte_input(vte, u8, len);*/
  /* send */
  r = shl_pty_write(pty, u8, len);
  if (r < 0) {
    printf ("could not write to pty, %d\n", r);
  }
}

static int draw_cb(struct tsm_screen *screen, uint32_t id,
                   const uint32_t *ch, size_t len,
                   unsigned int cwidth, unsigned int posx,
                   unsigned int posy,
                   const struct tsm_screen_attr *attr,
                   tsm_age_t age2, void *data)
{
int i;
  int h;
  int dx=posx*24, dy=(posy*24);
  char buf[32];
h = SDL_GetVideoInfo()->current_h;
  dy = h-(posy*24)-24;
  glColor4ub(0,0,255,255 / 10 * age2);
  glPolygonMode(GL_FRONT, GL_LINE);
  glRectf(dx+1,dy+1,dx+23,dy+23);
  glColor4ub(255,255,255,255);
  /*if (age2 == 0 || age2 <= age) return 0;*/
  for (i=0; i < len;i+=cwidth)  {
    sprintf(buf,"%c",ch[i]);
    sth_draw_text(data, 3, 24.0f, dx+4,dy+4,buf,&dx);
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

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    return -1;
  }

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
  SDL_WM_SetCaption("FontStash Demo", 0);

  stash = sth_create(512,512);
  if (!stash)
  {
    printf("Could not create stash.\n");
    return -1;
  }
  if (!sth_add_font(stash,0,FONT_DIR "DroidSerif-Regular.ttf"))
  {
    printf("Could not add font.\n");
    return -1;
  }
  if (!sth_add_font(stash,1,FONT_DIR "DroidSerif-Italic.ttf"))
  {
    printf("Could not add font.\n");
    return -1;
  }  
  if (!sth_add_font(stash,2,FONT_DIR "DroidSerif-Bold.ttf"))
  {
    printf("Could not add font.\n");
    return -1;
  }  
  if (!sth_add_font(stash,3,FONT_DIR "DroidSansMono.ttf"))
  {
    printf("Could not add font.\n");
    return -1;
  }  

  tsm_screen_new(&console, log_tsm, 0);
  tsm_vte_new(&vte, console, term_write_cb, 0, log_tsm, 0);
  tsm_screen_resize(console, (width / 24)-1, (height / 24)-1);
  assert(vte != 0);
  printf("console width: %d\n", tsm_screen_get_width(console));
  printf("console height: %d\n", tsm_screen_get_height(console));
/*
  tsm_vte_input(vte, "Hello, World!", 14);
  tsm_vte_input(vte, "zxghjulmnvxyaMMNPAS", 20);
*/

  /* this call will fork */
  pid = shl_pty_open(&pty,
                   term_read_cb,
                   NULL,
                   tsm_screen_get_width(console),
                   tsm_screen_get_height(console));

  if (pid < 0) {
    perror("fork problem");
  } else if (pid != 0 ) {
    /* parent */
    printf("in parent, child pid is %d\n", pid);
  } else {
    char **argv = (char*[]) {
                getenv("SHELL") ? : "/bin/bash",
                NULL
        };
    int r;
    setenv("TERM", "xterm-256color", 1);
    printf("in child, SHELL %s\n", argv[0]);
    r = execve(argv[0], argv, environ);
    if (r < 0) { perror("execve failed"); }
    exit(1);
  }

  done = 0;
  while (!done)
  {
    while (SDL_PollEvent(&event))
    {
      SDL_keysym k = event.key.keysym;
      unsigned int xkbsym = 0;
      unsigned int mods = 0;
      switch (event.type)
      {
        case SDL_MOUSEMOTION:
          break;
        case SDL_MOUSEBUTTONDOWN:
          break;
        case SDL_KEYDOWN:
          if (k.mod & KMOD_CTRL)  mods |= TSM_CONTROL_MASK;
          if (k.mod & KMOD_SHIFT) mods |= TSM_SHIFT_MASK;
          if (k.mod & KMOD_ALT)   mods |= TSM_ALT_MASK;
          if (k.mod & KMOD_META)  mods |= TSM_LOGO_MASK;
          if (k.sym == SDLK_ESCAPE)
            done = 1;
          else
            tsm_vte_handle_keyboard(vte, xkbsym, k.unicode, mods, k.unicode);
          /*tsm_screen_write(console, k.unicode, 0);*/
          break;
        /*case SDL_TEXTINPUT:*/
          /*printf("SDL TEXT\n");*/
          break;
        case SDL_QUIT:
          done = 1;
          break;
        default:
          break;
      }
    }
    
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,width,0,height,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor4ub(0,0,255,255);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    sth_begin_draw(stash);
    age = tsm_screen_draw(console, draw_cb, stash);
    sth_end_draw(stash);
    
    glEnable(GL_DEPTH_TEST);
    SDL_GL_SwapBuffers();

    SDL_Delay(20);

    {
      int r = shl_pty_dispatch(pty);
      if (r<0) {
        printf("pty dispatch error %d\n", r);
      }
    }
  }
  shl_pty_close(pty);
  
  SDL_Quit();
  return 0;
}
