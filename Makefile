# XPE CHAMS v2 - MinGW-w64 Makefile
# Author: xpe.nettt

# ============================================================
# Configuration
# ============================================================
CXX      := g++
CC       := gcc
WINDRES  := windres
STRIP    := strip

# Architecture (x86_64)
ARCH     := -m64

# Compiler flags
CXXFLAGS := $(ARCH) -std=c++17 -O2 -s -fvisibility=hidden
CFLAGS   := $(ARCH) -O2 -s
LDFLAGS  := $(ARCH) -shared -static-libgcc -static-libstdc++ -Wl,--exclude-all-symbols

# Include paths
INCLUDES := -Isrc -Ivendor/imgui

# Libraries
LIBS     := -lopengl32 -ld3d11 -ldxgi -lwininet -lshlwapi -ldwmapi -lpsapi -lgdi32 -luser32

# Directories
SRC_DIR  := src
VENDOR_DIR := vendor/imgui
BUILD_DIR := build
DIST_DIR  := dist

# ============================================================
# Source files
# ============================================================
XPE_SRCS := \
	$(SRC_DIR)/dllmain.cpp \
	$(SRC_DIR)/config.cpp \
	$(SRC_DIR)/overlay.cpp \
	$(SRC_DIR)/glchams.cpp \
	$(SRC_DIR)/keyauth.cpp

IMGUI_SRCS := \
	$(VENDOR_DIR)/imgui.cpp \
	$(VENDOR_DIR)/imgui_draw.cpp \
	$(VENDOR_DIR)/imgui_tables.cpp \
	$(VENDOR_DIR)/imgui_widgets.cpp \
	$(VENDOR_DIR)/imgui_impl_dx11.cpp \
	$(VENDOR_DIR)/imgui_impl_win32.cpp

LOADER_SRC := $(SRC_DIR)/loader_proxy.cpp

# Resource file
RC_FILE := $(SRC_DIR)/resource.rc
RC_OBJ  := $(BUILD_DIR)/resource.o

# Output files
XPE_DLL    := $(DIST_DIR)/XPE_CHAMS.dll
LOADER_DLL := $(DIST_DIR)/ldopengl32.dll

# ============================================================
# Object files
# ============================================================
XPE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(XPE_SRCS)) \
            $(patsubst $(VENDOR_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(IMGUI_SRCS))

LOADER_OBJS := $(BUILD_DIR)/loader_proxy.o

# ============================================================
# Default target
# ============================================================
all: directories $(XPE_DLL) $(LOADER_DLL)

# ============================================================
# Create directories
# ============================================================
directories:
	@mkdir -p $(BUILD_DIR) $(DIST_DIR)

# ============================================================
# Compile resource file
# ============================================================
$(RC_OBJ): $(RC_FILE)
	$(WINDRES) -i $< -o $@ -O coff

# ============================================================
# Compile source files
# ============================================================
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(VENDOR_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ============================================================
# Link XPE_CHAMS.dll
# ============================================================
$(XPE_DLL): $(XPE_OBJS) $(RC_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $@
	@echo ">>> Built: $@"

# ============================================================
# Compile & link loader proxy
# ============================================================
$(BUILD_DIR)/loader_proxy.o: $(LOADER_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(LOADER_DLL): $(LOADER_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lopengl32
	$(STRIP) $@
	@echo ">>> Built: $@"

# ============================================================
# Clean
# ============================================================
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
	@echo ">>> Cleaned"

# ============================================================
# Install (copy to LDPlayer directory example)
# ============================================================
install: all
	@echo ">>> Copy to LDPlayer directory:"
	@echo "    copy $(DIST_DIR)\XPE_CHAMS.dll C:\LDPlayer\"
	@echo "    copy $(DIST_DIR)\ldopengl32.dll C:\LDPlayer\ldopengl32.dll"
	@echo "    (Backup original ldopengl32.dll first!)"

# ============================================================
# Phony targets
# ============================================================
.PHONY: all directories clean install