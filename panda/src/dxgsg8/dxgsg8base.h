// Filename: dxgsg8base.h
// Created by:  georges (07Oct01)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifndef DXGSG8BASE_H
#define DXGSG8BASE_H

#include "pandabase.h"
#include "graphicsWindow.h"
#include "pmap.h"

#define WIN32_LEAN_AND_MEAN   // get rid of mfc win32 hdr stuff
#ifndef STRICT
// enable strict type checking in windows.h, see msdn
#define STRICT
#endif

#define DIRECT3D_VERSION 0x0800
#include <windows.h>
#include <d3d8.h>
#include <d3dx8.h>
#include <dxerr8.h>
#include <d3dx8tex.h>
#undef WIN32_LEAN_AND_MEAN

#ifndef D3DERRORSTRING
#ifdef NDEBUG
#define D3DERRORSTRING(HRESULT) " at (" << __FILE__ << ":" << __LINE__ << "), hr=" <<  DXGetErrorString8(HRESULT) << endl  // leave out descriptions to shrink release build
#else
#define D3DERRORSTRING(HRESULT) " at (" << __FILE__ << ":" << __LINE__ << "), hr=" <<  DXGetErrorString8(HRESULT) << ": " << DXGetErrorDescription8(HRESULT) << endl
#endif
#endif

// imperfect method to ID NVid? could also scan desc str, but that isnt fullproof either
#define IS_NVIDIA(DDDEVICEID) ((DDDEVICEID.VendorId==0x10DE) || (DDDEVICEID.VendorId==0x12D2))
#define IS_ATI(DDDEVICEID) (DDDEVICEID.VendorId==0x1002)
#define IS_MATROX(DDDEVICEID) (DDDEVICEID.VendorId==0x102B)

#define D3D_MAXTEXTURESTAGES 8

typedef enum {VertexShader,PixelShader} ShaderType;
typedef DWORD DXShaderHandle;

#define ISPOW2(X) (((X) & ((X)-1))==0)
#define IS_VALID_PTR(PTR)  (!IsBadWritePtr(PTR,sizeof(void*)))

#define DX_DECLARE_CLEAN(type, var) \
    type var;                       \
    ZeroMemory(&var, sizeof(type)); \
    var.dwSize = sizeof(type);

#define SAFE_DELSHADER(TYPE,HANDLE,PDEVICE)  \
  if((HANDLE!=NULL)&&IS_VALID_PTR(PDEVICE)) { PDEVICE->Delete##TYPE##Shader(HANDLE);  HANDLE=NULL; }

#define SAFE_DELETE(p)       { if(p) { assert(IS_VALID_PTR(p));   delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { assert(IS_VALID_PTR(p));   delete [] (p);   (p)=NULL; } }

// for stuff outside a panda class
#define SAFE_RELEASE(p)      { if(p) { assert(IS_VALID_PTR(p)); (p)->Release(); (p)=NULL; } }
#define SAFE_FREELIB(hDLL)   { if(hDLL!=NULL) {  FreeLibrary(hDLL);hDLL = NULL; } }

// this is bDoDownToZero argument to RELEASE()
#define RELEASE_DOWN_TO_ZERO true
#define RELEASE_ONCE false


// uncomment to add refcnt debug output
#define DEBUG_RELEASES

#ifdef DEBUG_RELEASES
#define RELEASE(OBJECT,MODULE,DBGSTR,bDoDownToZero)             {  \
   ULONG refcnt;                                                \
   if(IS_VALID_PTR(OBJECT)) {                                   \
        refcnt = (OBJECT)->Release();                           \
        MODULE##_cat.debug() << DBGSTR << " released, refcnt = " << refcnt << " at " << __FILE__ << ":" << __LINE__ << endl; \
        if((bDoDownToZero) && (refcnt>0)) {                     \
              MODULE##_cat.warning() << DBGSTR << " released but still has a non-zero refcnt(" << refcnt << "), multi-releasing it down to zero!\n"; \
              do {                                \
                refcnt = (OBJECT)->Release();     \
              } while(refcnt>0);                  \
        }                                         \
        (OBJECT) = NULL;                          \
      } else {                                    \
        MODULE##_cat.debug() << DBGSTR << " not released, ptr == NULL" << endl;  \
      }}

#define PRINT_REFCNT(MODULE,p) { ULONG refcnt;  (p)->AddRef();  refcnt=(p)->Release(); \
                                 MODULE##_cat.debug() << #p << " has refcnt = " << refcnt << " at " << __FILE__ << ":" << __LINE__ << endl; }

#else
#define RELEASE(OBJECT,MODULE,DBGSTR,bDoDownToZero)   { \
   ULONG refcnt;                                        \
   if(IS_VALID_PTR(OBJECT))                           { \
        refcnt=(OBJECT)->Release();                     \
        if((bDoDownToZero) && (refcnt>0)) {             \
              MODULE##_cat.warning() << DBGSTR << " released but still has a non-zero refcnt(" << refcnt << "), multi-releasing it down to zero!\n"; \
              do {                                \
                refcnt = (OBJECT)->Release();     \
              } while(refcnt>0);                  \
        }                                         \
        (OBJECT) = NULL;                          \
   }}

#define PRINT_REFCNT(MODULE,p)
#endif

#ifdef DO_PSTATS
#define DO_PSTATS_STUFF(XX) XX;
#else
#define DO_PSTATS_STUFF(XX)
#endif

#define PANDA_MAXNUMVERTS 0xFFFF  // Note Device may support more than this if it supports D3DFMT_INDEX32 indexbufs.

#define FLG(NN) (1<<NN)
#define MAX_POSSIBLE_TEXFMTS 32
typedef enum {
    R8G8B8_FLAG =       FLG(0),
    A8R8G8B8_FLAG =     FLG(1),
    X8R8G8B8_FLAG =     FLG(2),
    R5G6B5_FLAG =       FLG(3),
    X1R5G5B5_FLAG =     FLG(4),
    A1R5G5B5_FLAG =     FLG(5),
    A4R4G4B4_FLAG =     FLG(6),
    R3G3B2_FLAG =       FLG(7),
    A8_FLAG =           FLG(8),
    A8R3G3B2_FLAG =     FLG(9),
    X4R4G4B4_FLAG =     FLG(10),
    A2B10G10R10_FLAG =  FLG(11),
    G16R16_FLAG =       FLG(12),
    A8P8_FLAG =         FLG(13),
    P8_FLAG =           FLG(14),
    L8_FLAG =           FLG(15),
    A8L8_FLAG =         FLG(16),
    A4L4_FLAG =         FLG(17),
    V8U8_FLAG =         FLG(18),
    L6V5U5_FLAG =       FLG(19),
    X8L8V8U8_FLAG =     FLG(20),
    Q8W8V8U8_FLAG =     FLG(21),
    V16U16_FLAG =       FLG(22),
    W11V11U10_FLAG =    FLG(23),
    A2W10V10U10_FLAG =  FLG(24),
    UYVY_FLAG =         FLG(25),
    YUY2_FLAG =         FLG(26),
    DXT1_FLAG =         FLG(27),
    DXT2_FLAG =         FLG(28),
    DXT3_FLAG =         FLG(29),
    DXT4_FLAG =         FLG(30),
    DXT5_FLAG =         FLG(31)
} D3DFORMAT_FLAG;

// this is only used in conjunction w/rendertgt fmts, so just make it something that can never be a rtgt
#define DISPLAY_32BPP_REQUIRES_16BPP_ZBUFFER_FLAG DXT1_FLAG
#define DISPLAY_16BPP_REQUIRES_16BPP_ZBUFFER_FLAG DXT2_FLAG

#define IS_16BPP_DISPLAY_FORMAT(FMT) (((FMT)==D3DFMT_R5G6B5)||((FMT)==D3DFMT_X1R5G5B5)||((FMT)==D3DFMT_A1R5G5B5))
#define IS_16BPP_ZBUFFER(FMT) ((FMT==D3DFMT_D16)||(FMT==D3DFMT_D15S1))
#define IS_STENCIL_FORMAT(FMT) (((FMT)==D3DFMT_D24S8) || ((FMT)==D3DFMT_D15S1) || ((FMT)==D3DFMT_D24X4S4))
#define RECT_XSIZE(REC) (REC.right-REC.left)
#define RECT_YSIZE(REC) (REC.bottom-REC.top)

class DXGraphicsStateGuardian8;

struct DXScreenData {
  LPDIRECT3DDEVICE8 _d3d_device;
  IDirect3DSwapChain8 *_swap_chain;
  LPDIRECT3D8 _d3d8;  // copied from DXGraphicsPipe8 for convenience
  HWND _window;
  HMONITOR _monitor;
  DWORD _max_available_video_memory;
  ushort _card_id;  // adapter ID
  ushort _depth_buffer_bitdepth;  //GetSurfaceDesc is not reliable so must store this explicitly
  bool _can_direct_disable_color_writes;  // if true, dont need blending for this
  bool _is_low_memory_card;
  bool _is_tnl_device;
  bool _can_use_hw_vertex_shaders;
  bool _can_use_pixel_shaders;
  bool _is_dx8_1;
  UINT _supported_screen_depths_mask;
  UINT _supported_tex_formats_mask;
  D3DCAPS8 _d3dcaps;
  D3DDISPLAYMODE _display_mode;
  D3DPRESENT_PARAMETERS _presentation_params;  // not redundant with _display_mode since width/height must be 0 for windowed mode
  D3DADAPTER_IDENTIFIER8 _dx_device_id;
  D3DFORMAT _render_to_texture_d3d_format;
  D3DFORMAT _framebuffer_d3d_format;

  DXGraphicsStateGuardian8 *_dxgsg8;
};


//utility stuff
extern pmap<D3DFORMAT_FLAG,D3DFORMAT> g_D3DFORMATmap;
extern void Init_D3DFORMAT_map();
extern const char *D3DFormatStr(D3DFORMAT fmt);

#endif

