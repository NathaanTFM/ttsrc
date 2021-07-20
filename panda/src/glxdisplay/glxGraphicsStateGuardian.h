// Filename: glxGraphicsStateGuardian.h
// Created by:  drose (27Jan03)
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

#ifndef GLXGRAPHICSSTATEGUARDIAN_H
#define GLXGRAPHICSSTATEGUARDIAN_H

#include "pandabase.h"

#include "glgsg.h"
#include "glxGraphicsPipe.h"

#include <GL/glx.h>

#if defined(GLX_VERSION_1_4)
// If the system header files give us version 1.4, we can assume it's
// safe to compile in a reference to glxGetProcAddress().
#define HAVE_GLXGETPROCADDRESS 1

#elif defined(GLX_ARB_get_proc_address)
// Maybe the system header files give us the corresponding ARB call.
#define HAVE_GLXGETPROCADDRESSARB 1

// Sometimes the system header files don't define this prototype for
// some reason.
extern "C" void (*glXGetProcAddressARB(const GLubyte *procName))( void );

#endif

// This must be included after we have included glgsg.h (which
// includes gl.h).
#include "glxext.h"

// drose: the version of GL/glx.h that ships with Fedora Core 2 seems
// to define GLX_VERSION_1_4, but for some reason does not define
// GLX_SAMPLE_BUFFERS or GLX_SAMPLES.  We work around that here.

#ifndef GLX_SAMPLE_BUFFERS
#define GLX_SAMPLE_BUFFERS                 100000
#endif
#ifndef GLX_SAMPLES
#define GLX_SAMPLES                        100001
#endif

// These typedefs are declared in glxext.h, but we must repeat them
// here, mainly because they will not be included from glxext.h if the
// system GLX version matches or exceeds the GLX version in which
// these functions are defined, and the system glx.h sometimes doesn't
// declare these typedefs.
#ifndef __EDG__  // Protect the following from the Tau instrumentor.
typedef __GLXextFuncPtr (* PFNGLXGETPROCADDRESSPROC) (const GLubyte *procName);
typedef int (* PFNGLXSWAPINTERVALSGIPROC) (int interval);
#endif  // __EDG__

////////////////////////////////////////////////////////////////////
//       Class : glxGraphicsStateGuardian
// Description : A tiny specialization on GLGraphicsStateGuardian to
//               add some glx-specific information.
////////////////////////////////////////////////////////////////////
class glxGraphicsStateGuardian : public GLGraphicsStateGuardian {
public:
#ifdef HAVE_GLXFBCONFIG
  typedef GLXFBConfig fbconfig;
#else
  typedef int         fbconfig;
#endif

  INLINE const FrameBufferProperties &get_fb_properties() const;
  void get_properties(FrameBufferProperties &properties, XVisualInfo *visual);
  void get_properties_advanced(FrameBufferProperties &properties,
                               bool &pbuffer_supported, bool &pixmap_supported,
                               bool &slow, fbconfig config);
  void choose_pixel_format(const FrameBufferProperties &properties, 
                           Display *_display,
                           int _screen,
                           bool need_pbuffer, bool need_pixmap);
  
  glxGraphicsStateGuardian(GraphicsEngine *engine, GraphicsPipe *pipe,
                           glxGraphicsStateGuardian *share_with);

  virtual ~glxGraphicsStateGuardian();

  virtual void reset();

  bool glx_is_at_least_version(int major_version, int minor_version) const;

  GLXContext _share_context;
  GLXContext _context;
  Display *_display;
  int _screen;
  XVisualInfo *_visual;
  XVisualInfo *_visuals;
  fbconfig _fbconfig;
  FrameBufferProperties _fbprops;

public:
  bool _supports_swap_control;
  PFNGLXSWAPINTERVALSGIPROC _glXSwapIntervalSGI;

protected:
  virtual void gl_flush() const;
  virtual GLenum gl_get_error() const;

  virtual void query_gl_version();
  virtual void get_extra_extensions();
  virtual void *do_get_extension_func(const char *prefix, const char *name);

private:
  void *get_system_func(const char *name);
  void show_glx_client_string(const string &name, int id);
  void show_glx_server_string(const string &name, int id);


  int _glx_version_major, _glx_version_minor;

  void *_libgl_handle;
  bool _checked_get_proc_address;
  PFNGLXGETPROCADDRESSPROC _glXGetProcAddress;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    GLGraphicsStateGuardian::init_type();
    register_type(_type_handle, "glxGraphicsStateGuardian",
                  GLGraphicsStateGuardian::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "glxGraphicsStateGuardian.I"

#endif
