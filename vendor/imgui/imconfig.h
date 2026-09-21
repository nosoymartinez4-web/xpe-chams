//-----------------------------------------------------------------------------
// DEAR IMGUI COMPILE-TIME OPTIONS
//-----------------------------------------------------------------------------
#pragma once

//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)
//#define IM_ASSERT(_EXPR)  ((void)(_EXPR))     // Disable asserts

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions/enums/behaviors. Consider enabling for projects with a large codebase.
//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement default handlers for Windows (so you can implement them yourself)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS

//---- Don't implement default handlers for OSX (so you can implement them yourself)
//#define IMGUI_DISABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS

//---- Don't include imgui_user.inl in imgui.cpp - saves ~0.25% code size
//#define IMGUI_DISABLE_INCLUDE_IMGUI_USER_INL

//---- Assert when passing wrong values in imgui (e.g. for disabling assert in release builds)
#ifndef IMGUI_DISABLE_ASSERT
#define IMGUI_ASSERT(_EXPR)  ((void)(_EXPR))
#endif

//---- Use 32-bit vertex indices (default is 16-bit) to allow meshes with more than 64k vertices
//#define ImDrawIdx unsigned int

//---- Use 32-bit storage for vertex indices (default is sizeof(ImDrawIdx)*4)
//#define ImDrawIdx unsigned int

//---- Support languages where the cursor position is mirrored (Arabic, Hebrew)
//#define IMGUI_ENABLE_NAV_INPUT

//---- Support multi-viewports (multiple windows, game-embedded editor, etc.)
//#define IMGUI_ENABLE_DOCKING

//---- Use 32-bit vertex indices (default is 16-bit) to allow meshes with more than 64k vertices
//#define ImDrawIdx unsigned int

//---- Don't implement default font loading
//#define IMGUI_DISABLE_DEFAULT_FONT

//---- Use stb_truetype to load fonts (default)
#define IMGUI_STB_TRUETYPE_FILENAME   "imstb_truetype.h"
#define IMGUI_STB_RECT_PACK_FILENAME  "imstb_rectpack.h"
#define IMGUI_STB_TEXTEDIT_FILENAME   "imstb_textedit.h"