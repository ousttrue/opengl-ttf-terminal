#pragma once

struct Args {
  int width = 1024;
  int height = 764;
  bool fullscreen = 0;
  // font height
  float fh = 21.0f;
  const char *fontfile = "VeraMono.ttf";

  void parse(int argc, char **argv);
};
