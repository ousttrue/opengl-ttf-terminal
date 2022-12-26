#include "args.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Args::parse(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "f:s:g:m")) != -1) {
    switch (opt) {
    case 'f':
      fontfile = optarg;
      break;
    case 's':
      fh = atoi(optarg);
      break;
    case 'g':
      width = atoi(strtok(optarg, "x"));
      height = atoi(strtok(NULL, "x"));
      break;
    case 'm':
      fullscreen = true;
      break;
    default: /* '?' */
      fprintf(stderr,
              "Usage: %s [-f ttf file] [-s font size] [-g geometry] [-m "
              "fullscreen mode]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  printf("Using: %s    at: %f \n", fontfile, fh);
}
