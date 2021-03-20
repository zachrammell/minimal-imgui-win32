# Build IMGUI
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_INCLUDE_DIR "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
file(GLOB IMGUI_SRC "${IMGUI_DIR}/*.cpp")
# Append IMGUI SRC with dx11 and win32
set(IMGUI_SRC
  ${IMGUI_SRC}
  ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
  ${IMGUI_DIR}/backends/imgui_impl_win32.cpp)
add_library(IMGUI ${IMGUI_SRC})
target_include_directories(IMGUI
  PUBLIC
  ${IMGUI_INCLUDE_DIR}
)
target_link_libraries(IMGUI
  PRIVATE
  user32
  d3d11
  dxgi
  d3dcompiler
)
