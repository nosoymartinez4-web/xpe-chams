# CAMBIOS DE BRANDING - CHAMSMENU.dll → XPE CHAMS v2

## Cambios Realizados

### 1. Título del Menú
- **Antes**: `CHAMS MENU` (genérico)
- **Después**: `XPE CHAMS v2`

### 2. Título de la Ventana
- **Antes**: (sin título específico)
- **Después**: `XPE CHAMS Overlay`

### 3. Watermark
- **Antes**: (sin watermark o genérico)
- **Después**: `XPE CHAMS v2 | xpe.nettt` (color cyan)

### 4. Barra de Estado
- **Antes**: (sin información de autor)
- **Después**: `XPE CHAMS | xpe.nettt` (azul claro)

### 5. Ventana de Login
- **Antes**: (sin título)
- **Después**: `XPE CHAMS - Login` con texto "Welcome to XPE CHAMS\nAuthor: xpe.nettt"

### 6. Version Info (resource.rc)
- **Antes**: Sin version info o genérico
- **Después**:
  - CompanyName: `xpe.nettt`
  - FileDescription: `XPE CHAMS v2 - OpenGL Chams + D3D11 Overlay`
  - LegalCopyright: `Copyright (c) 2026 xpe.nettt. All rights reserved.`
  - ProductName: `XPE CHAMS`
  - OriginalFilename: `XPE_CHAMS.dll`

### 7. Nombre del DLL
- **Antes**: `CHAMSMENU.dll`
- **Después**: `XPE_CHAMS.dll`

### 8. Clase de Ventana Overlay
- **Antes**: (clase genérica)
- **Después**: `XPE_OVERLAY_CLASS`

### 9. Config File
- **Antes**: (nombre desconocido)
- **Después**: `xpe_chams_config.ini`

### 10. Log File
- **Antes**: (sin logging)
- **Después**: `xpe_chams.log`

### 11. KeyAuth
- **Antes**: ownerid genérico
- **Después**: `MfJpimy9Dd` (mantenido del original, pero ahora controlado por nosotros)

### 12. Código fuente
- **Antes**: CERRADO (solo DLL binario)
- **Después**: ABIERTO (C++ completo con comentarios y documentación)

### 13. Loader Proxy
- **Antes**: (ninguno)
- **Después**: `ldopengl32.dll` - Universal proxy loader para LDPlayer y BlueStacks