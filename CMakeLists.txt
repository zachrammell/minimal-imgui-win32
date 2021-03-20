cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("minimal-imgui")

add_subdirectory("external")

add_executable(${PROJECT_NAME}
  "source/main.cpp"
  "source/os_win32.h"
  "source/os_win32.cpp"
  "source/render_dx11.h"
  "source/render_dx11.cpp"
  "source/callback_helper.h"
  )

target_link_libraries(${PROJECT_NAME}
  user32
  d3d11
  dxgi
  d3dcompiler
  IMGUI
)
