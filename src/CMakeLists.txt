find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} main.cpp shl_pty.cpp args.cpp
                               FontStashRenderer.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw m OpenGL::GL tsm fontstash)
target_compile_definitions(${PROJECT_NAME} PRIVATE SIGUNUSED=SIGSYS)
