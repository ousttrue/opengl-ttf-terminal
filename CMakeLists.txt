cmake_minimum_required(VERSION 3.22.1)
project(togl)

include(FetchContent)
set(BUILD_SHARED_LIBS ON)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(
  ${PROJECT_NAME}
  main.c
  libshl/src/shl_pty.c
  libtsm/src/shl_htable.c
  libtsm/src/tsm_render.c
  libtsm/src/tsm_screen.c
  libtsm/src/tsm_selection.c
  libtsm/src/tsm_unicode.c
  libtsm/src/tsm_vte.c
  libtsm/src/tsm_vte_charsets.c
  libtsm/external/wcwidth.c)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw m OpenGL::GL)
target_compile_definitions(${PROJECT_NAME} PRIVATE FONTSTASH_IMPLEMENTATION
                                                   SIGUNUSED=SIGSYS)
target_include_directories(${PROJECT_NAME} PRIVATE fontstash/src libtsm/src
                                                   libtsm libshl/src)
