#define BUILD_DIRECTORY $[HAVE_DX8]

// DIR_TYPE "metalib" indicates we are building a shared library that
// consists mostly of references to other shared libraries.  Under
// Windows, this directly produces a DLL (as opposed to the regular
// src libraries, which don't produce anything but a pile of OBJ files
// under Windows).

#define DIR_TYPE metalib
#define BUILDING_DLL BUILDING_PANDADX

#define COMPONENT_LIBS \
    dxgsg8
#define LOCAL_LIBS gsgbase display express gobj
#define OTHER_LIBS dtoolconfig dtool

#begin metalib_target
  #define TARGET pandadx8
  #define SOURCES pandadx8.cxx
  #define WIN_SYS_LIBS \
     ddraw.lib dxguid.lib winmm.lib \
     kernel32.lib gdi32.lib user32.lib advapi32.lib $[WIN_SYS_LIBS]  
#end metalib_target

