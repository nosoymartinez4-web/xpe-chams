cmake_minimum_required(VERSION 3.10)
project(XPE_CHAMS VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)

# Source files
set(SOURCES
    src/config.cpp
    src/dllmain.cpp
    src/keyauth.cpp
    src/glchams.cpp
    src/overlay.cpp
    src/resource.rc
)

# ImGui sources
file(GLOB IMGUI_SOURCES
    vendor/imgui/*.cpp
)

# Include directories
include_directories(
    src
    vendor
    vendor/imgui
)

# Build DLL
add_library(XPE_CHAMS SHARED ${SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(XPE_CHAMS
    ${OPENGL_LIBRARIES}
    d3d11
    dxgi
    wininet
    shlwapi
    dwmapi
    psapi
    gdi32
    user32
)

# Windows subsystem
set_target_properties(XPE_CHAMS PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
    LINK_FLAGS "/NODEFAULTLIB:msvcrt"
)

# ============================================================
# Proxy Loader (ldopengl32.dll / opengl32.dll proxy)
# ============================================================
add_library(ldopengl32 SHARED
    src/loader_proxy.cpp
)

target_link_libraries(ldopengl32
    opengl32
    XPE_CHAMS
)

set_target_properties(ldopengl32 PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
)

# Install targets
install(TARGETS XPE_CHAMS ldopengl32
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)