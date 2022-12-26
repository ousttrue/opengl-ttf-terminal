cmake_minimum_required(VERSION 3.22.1)
project(togl)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

add_library(fontstash INTERFACE)
target_include_directories(fontstash INTERFACE fontstash/src)

add_library(
  tsm
  libtsm/src/shl_htable.c
  libtsm/src/tsm_render.c
  libtsm/src/tsm_screen.c
  libtsm/src/tsm_selection.c
  libtsm/src/tsm_unicode.c
  libtsm/src/tsm_vte.c
  libtsm/src/tsm_vte_charsets.c
  libtsm/external/wcwidth.c)
target_include_directories(tsm PUBLIC libtsm/src libtsm)

subdirs(src)
