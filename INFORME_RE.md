# INFORME DE INGENIERÍA INVERSA - CHAMSMENU.dll → XPE CHAMS v2

## 1. Información del DLL Original

| Campo | Valor |
|-------|-------|
| **Archivo** | CHAMSMENU.dll |
| **Tamaño** | 817,152 bytes |
| **Entry Point** | 0x18008fc0c (DllMain) |
| **Compilador** | Microsoft Visual C++ 2022 (MSVC 14.3x) |
| **Arquitectura** | x86-64 |
| **Subsistema** | Windows DLL |
| **Enlace** | Dinámico (/DYNAMICBASE) |

## 2. Imports Detectados

### D3D11/DXGI
- `D3D11CreateDeviceAndSwapChain` - Creación del device para hookear
- `IDXGISwapChain::Present` - Hookeado en vtable slot 8
- `IDXGISwapChain::ResizeBuffers` - Hookeado en vtable slot 13

### OpenGL (opengl32.dll)
- `glEnable` / `glDisable` - Control de estados GL
- `glClear` - Limpieza de buffers
- `glColor4fv` - Seteo de color para chams
- `glDepthFunc` / `glDepthMask` / `glDepthRange` - Control de profundidad
- `glStencilFunc` / `glStencilOp` - Stencil buffer para wallhack chams
- `glColorMask` - Renderizado selectivo por canales
- `glPolygonMode` / `glLineWidth` - Wireframe chams
- `glGetIntegerv` - Guardar/restaurar estado GL
- `glMatrixMode` / `glLoadIdentity` / `glPushMatrix` / `glPopMatrix` - Matrices

### KeyAuth (WININET)
- `InternetOpenA` - Inicializar sesión HTTP
- `InternetConnectA` - Conectar a keyauth.win:443
- `HttpOpenRequestA` - POST request
- `HttpSendRequestA` - Enviar datos
- `InternetReadFile` - Leer respuesta JSON
- `InternetCloseHandle` - Limpiar

### Windows API
- `CreateWindowExA` - Ventana overlay (WS_EX_LAYERED | WS_EX_TRANSPARENT)
- `SetLayeredWindowAttributes` - Transparencia (color key = RGB(0,0,0))
- `CreateThread` - Threads separados para auth y overlay
- `CreateToolhelp32Snapshot` / `Thread32First` / `Thread32Next` - Thread injection
- `SuspendThread` / `ResumeThread` / `GetThreadContext` / `SetThreadContext` - Inyección

## 3. Strings del Menú (extraídos del DLL original)

```
"Visuals" - Pestaña de configuración visual
"Wallhack" - Pestaña/opción wallhack
"Chams" - Pestaña de chams
"Glow hack" - Opción de glow
"ESP" - Extra Sensory Perception toggle
"Aimbot" - Pestaña de aimbot
"Radar" - Pestaña de radar
"Triggerbot" - Opción de trigger
"Streamer Mode" - Modo streamer
"Capture" - Detección de captura
"Invisible" - Color invisible (detrás de paredes)
"Visible" - Color visible (adelante)
"Box" / "Name" / "Health" / "Armor" / "Weapon" / "Distance" / "Line"
"FOV" / "Smooth" / "Bone" / "Head" / "Neck" / "Chest" / "Pelvis"
"Crosshair" / "Watermark" / "Save" / "Load" / "Config"
```

## 4. KeyAuth Credentials (extraídas del DLL)

| Campo | Valor |
|-------|-------|
| **ownerid** | `MfJpimy9Dd` |
| **appname** | `xpe_chams` |
| **version** | `1.0` |
| **API URL** | `https://keyauth.win/api/1.2/` |

## 5. Lógica de Chams Reconstruida

### Técnica de Stencil Buffer (Wallhack Chams)
1. **PASS 1 (Invisible)**: Desactiva depth test, renderiza con stencil → marca todos los píxeles
2. **PASS 2 (Visible)**: Activa depth test, renderiza normalmente
3. **PASS 3 (Wallhack)**: Usa `glDepthRange(-1.0, 0.0)` para forzar renderizado detrás de paredes
4. **PASS 4 (Glow)**: Renderiza modelo agrandado con blending aditivo (GL_SRC_ALPHA, GL_ONE)

### Técnica de VTable Hook (D3D11)
1. Obtiene vtable de IDXGISwapChain
2. Slot 8 = Present, Slot 13 = ResizeBuffers
3. Reemplaza punteros con detour de 14 bytes (JMP [RIP+0])
4. En Present hook: llama al original, luego renderiza ImGui overlay

## 6. Estructura de Archivos Reconstruida

```
XPE CHAMS v2/
├── src/
│   ├── dllmain.cpp       - Entry point, init threads
│   ├── config.h/.cpp     - Config global, save/load
│   ├── overlay.h/.cpp    - D3D11 ImGui overlay + menu
│   ├── glchams.h/.cpp    - OpenGL chams (stencil, depth, color)
│   ├── keyauth.h/.cpp    - KeyAuth authentication
│   ├── hooks.h           - x64 detour hook (14-byte JMP)
│   ├── streammode.h      - Streamer mode detection
│   ├── loader_proxy.cpp  - Proxy ldopengl32.dll loader
│   └── resource.rc       - Version resources
├── vendor/imgui/         - Dear ImGui library
├── CMakeLists.txt        - CMake build system
├── Makefile              - MinGW-w64 Makefile
└── dist/                 - Output DLLs
    ├── XPE_CHAMS.dll     - Main DLL
    └── ldopengl32.dll    - Proxy loader
```

## 7. Notas Técnicas

- **Anti-debug**: El DLL original tiene cookie check con valor mágico `0x2b992ddfa232`
- **Thread Safety**: Usa CreateThread con Sleep(100) para esperar contexto OpenGL
- **Compatibilidad**: Funciona con LDPlayer (clase "LDPlayerMainFrame") y BlueStacks (clase "BlueStacksApp")
- **Streamer Mode**: Detecta OBS/SLOBS/Discord por nombre de ventana y clase