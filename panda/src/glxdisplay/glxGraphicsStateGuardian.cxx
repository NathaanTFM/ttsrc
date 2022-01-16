// Filename: glxGraphicsStateGuardian.cxx
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

#include "glxGraphicsStateGuardian.h"
#include "config_glxdisplay.h"
#include "config_glgsg.h"
#include "lightReMutexHolder.h"

#include <dlfcn.h>


TypeHandle glxGraphicsStateGuardian::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
glxGraphicsStateGuardian::
glxGraphicsStateGuardian(GraphicsEngine *engine, GraphicsPipe *pipe,
                         glxGraphicsStateGuardian *share_with) :
  GLGraphicsStateGuardian(engine, pipe)
{
  _share_context=0;
  _context=0;
  _display=0;
  _screen=0;
  _visual=0;
  _visuals=0;
  _fbconfig=0;
  _context_has_pbuffer = false;
  _context_has_pixmap = false;
  _slow = false;

  _supports_swap_control = false;
  _supports_fbconfig = false;
  _supports_pbuffer = false;
  _uses_sgix_pbuffer = false;
  
  if (share_with != (glxGraphicsStateGuardian *)NULL) {
    _prepared_objects = share_with->get_prepared_objects();
    _share_context = share_with->_context;
  }
  
  _libgl_handle = NULL;
  _checked_get_proc_address = false;
  _glXGetProcAddress = NULL;
  _temp_context = (GLXContext)NULL;
  _temp_xwindow = (Window)NULL;
  _temp_colormap = (Colormap)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
glxGraphicsStateGuardian::
~glxGraphicsStateGuardian() {
  destroy_temp_xwindow();
  if (_visuals != (XVisualInfo *)NULL) {
    XFree(_visuals);
  }
  if (_context != (GLXContext)NULL) {
    glXDestroyContext(_display, _context);
    _context = (GLXContext)NULL;
  }
  if (_libgl_handle != (void *)NULL) {
    dlclose(_libgl_handle);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::get_properties
//       Access: Public
//  Description: Gets the FrameBufferProperties to match the
//               indicated visual.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
get_properties(FrameBufferProperties &properties, XVisualInfo *visual) {

  int use_gl, render_mode, double_buffer, stereo,
    red_size, green_size, blue_size,
    alpha_size, ared_size, agreen_size, ablue_size, aalpha_size,
    depth_size, stencil_size;
  
  glXGetConfig(_display, visual, GLX_USE_GL, &use_gl);
  glXGetConfig(_display, visual, GLX_RGBA, &render_mode);
  glXGetConfig(_display, visual, GLX_DOUBLEBUFFER, &double_buffer);
  glXGetConfig(_display, visual, GLX_STEREO, &stereo);
  glXGetConfig(_display, visual, GLX_RED_SIZE, &red_size);
  glXGetConfig(_display, visual, GLX_GREEN_SIZE, &green_size);
  glXGetConfig(_display, visual, GLX_BLUE_SIZE, &blue_size);
  glXGetConfig(_display, visual, GLX_ALPHA_SIZE, &alpha_size);
  glXGetConfig(_display, visual, GLX_ACCUM_RED_SIZE, &ared_size);
  glXGetConfig(_display, visual, GLX_ACCUM_GREEN_SIZE, &agreen_size);
  glXGetConfig(_display, visual, GLX_ACCUM_BLUE_SIZE, &ablue_size);
  glXGetConfig(_display, visual, GLX_ACCUM_ALPHA_SIZE, &aalpha_size);
  glXGetConfig(_display, visual, GLX_DEPTH_SIZE, &depth_size);
  glXGetConfig(_display, visual, GLX_STENCIL_SIZE, &stencil_size);

  properties.clear();

  if (use_gl == 0) {
    // If we return a set of properties without setting either
    // rgb_color or indexed_color, then this indicates a visual
    // that's no good for any kind of rendering.
    return;
  }

  if (double_buffer) {
    properties.set_back_buffers(1);
  }
  if (stereo) {
    properties.set_stereo(1);
  }
  if (render_mode) {
    properties.set_rgb_color(1);
  } else {
    properties.set_indexed_color(1);
  }
  properties.set_color_bits(red_size+green_size+blue_size);
  properties.set_stencil_bits(stencil_size);
  properties.set_depth_bits(depth_size);
  properties.set_alpha_bits(alpha_size);
  properties.set_accum_bits(ared_size+agreen_size+ablue_size+aalpha_size);
  
  // Set both hardware and software bits, indicating not-yet-known.
  properties.set_force_software(1);
  properties.set_force_hardware(1);
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::get_properties_advanced
//       Access: Public
//  Description: Gets the FrameBufferProperties to match the
//               indicated GLXFBConfig
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
get_properties_advanced(FrameBufferProperties &properties, 
                        bool &context_has_pbuffer, bool &context_has_pixmap,
                        bool &slow, GLXFBConfig config) {
  properties.clear();

  if (_supports_fbconfig) {
    // Now update our framebuffer_mode and bit depth appropriately.
    int render_type, double_buffer, stereo, red_size, green_size, blue_size,
      alpha_size, ared_size, agreen_size, ablue_size, aalpha_size,
      depth_size, stencil_size, samples, drawable_type, caveat;
    
    _glXGetFBConfigAttrib(_display, config, GLX_RENDER_TYPE, &render_type);
    _glXGetFBConfigAttrib(_display, config, GLX_DOUBLEBUFFER, &double_buffer);
    _glXGetFBConfigAttrib(_display, config, GLX_STEREO, &stereo);
    _glXGetFBConfigAttrib(_display, config, GLX_RED_SIZE, &red_size);
    _glXGetFBConfigAttrib(_display, config, GLX_GREEN_SIZE, &green_size);
    _glXGetFBConfigAttrib(_display, config, GLX_BLUE_SIZE, &blue_size);
    _glXGetFBConfigAttrib(_display, config, GLX_ALPHA_SIZE, &alpha_size);
    _glXGetFBConfigAttrib(_display, config, GLX_ACCUM_RED_SIZE, &ared_size);
    _glXGetFBConfigAttrib(_display, config, GLX_ACCUM_GREEN_SIZE, &agreen_size);
    _glXGetFBConfigAttrib(_display, config, GLX_ACCUM_BLUE_SIZE, &ablue_size);
    _glXGetFBConfigAttrib(_display, config, GLX_ACCUM_ALPHA_SIZE, &aalpha_size);
    _glXGetFBConfigAttrib(_display, config, GLX_DEPTH_SIZE, &depth_size);
    _glXGetFBConfigAttrib(_display, config, GLX_STENCIL_SIZE, &stencil_size);
    _glXGetFBConfigAttrib(_display, config, GLX_SAMPLES, &samples);
    _glXGetFBConfigAttrib(_display, config, GLX_DRAWABLE_TYPE, &drawable_type);
    _glXGetFBConfigAttrib(_display, config, GLX_CONFIG_CAVEAT, &caveat);
    
    context_has_pbuffer = false;
    if ((drawable_type & GLX_PBUFFER_BIT)!=0) {
      context_has_pbuffer = true;
    }
    
    context_has_pixmap = false;
    if ((drawable_type & GLX_PIXMAP_BIT)!=0) {
      context_has_pixmap = true;
    }
    
    slow = false;
    if (caveat == GLX_SLOW_CONFIG) {
      slow = true;
    }
    
    if ((drawable_type & GLX_WINDOW_BIT)==0) {
      // We insist on having a context that will support an onscreen window.
      return;
    }
    
    if (double_buffer) {
      properties.set_back_buffers(1);
    }
    if (stereo) {
      properties.set_stereo(1);
    }
    
    if ((render_type & GLX_RGBA_BIT)!=0) {
      properties.set_rgb_color(1);
    }
    if ((render_type & GLX_COLOR_INDEX_BIT)!=0) {
      properties.set_indexed_color(1);
    }
    properties.set_color_bits(red_size+green_size+blue_size);
    properties.set_stencil_bits(stencil_size);
    properties.set_depth_bits(depth_size);
    properties.set_alpha_bits(alpha_size);
    properties.set_accum_bits(ared_size+agreen_size+ablue_size+aalpha_size);
    properties.set_multisamples(samples);
    
    // Set both hardware and software bits, indicating not-yet-known.
    properties.set_force_software(1);
    properties.set_force_hardware(1);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::choose_pixel_format
//       Access: Public
//  Description: Selects a visual or fbconfig for all the windows
//               and buffers that use this gsg.  Also creates the GL
//               context and obtains the visual.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
choose_pixel_format(const FrameBufferProperties &properties,
                    Display *display,
                    int screen, bool need_pbuffer, bool need_pixmap) {

  _display = display;
  _screen = screen;
  _context = 0;
  _fbconfig = 0;
  _visual = 0;
  if (_visuals != (XVisualInfo *)NULL) {
    XFree(_visuals);
    _visuals = NULL;
  }

  _fbprops.clear();

  // First, attempt to create a context using the XVisual interface.
  // We need this before we can query the FBConfig interface, because
  // we need an OpenGL context to get the required extension function
  // pointers.
  destroy_temp_xwindow();
  choose_temp_visual(properties);
  if (_temp_context == NULL) {
    // No good.
    return;
  }

  // Now we have to initialize the context so we can query its
  // capabilities and extensions.  This also means creating a
  // temporary window, so we have something to bind the context to and
  // make it current.
  init_temp_context();

  if (!_supports_fbconfig) {
    // We have a good OpenGL context, but it doesn't support the
    // FBConfig interface, so we'll stop there.
    if (glxdisplay_cat.is_debug()) {
      glxdisplay_cat.debug()
        <<" No FBConfig supported; using XVisual only.\n";

      glxdisplay_cat.debug()
        << _fbprops << "\n";

      _context = _temp_context;
      _temp_context = (GLXContext)NULL;

      // By convention, every indirect XVisual that can render to a
      // window can also render to a GLXPixmap.  Direct visuals we're
      // not as sure about.
      _context_has_pixmap = !glXIsDirect(_display, _context);

      // Pbuffers aren't supported at all with the XVisual interface.
      _context_has_pbuffer = false;
    }
    return;
  }

  // The OpenGL context supports the FBConfig interface, so we can use
  // that more advanced interface to choose the actual window format
  // we'll use.  FBConfig provides for more options than the older
  // XVisual interface, so we'd much rather use it if it's available.

  int best_quality = 0;
  int best_result = 0;
  FrameBufferProperties best_props;

  static const int max_attrib_list = 32;
  int attrib_list[max_attrib_list];
  int n = 0;
  attrib_list[n++] = GLX_STEREO;
  attrib_list[n++] = GLX_DONT_CARE;
  attrib_list[n++] = GLX_RENDER_TYPE;
  attrib_list[n++] = GLX_DONT_CARE;
  attrib_list[n++] = GLX_DRAWABLE_TYPE;
  attrib_list[n++] = GLX_DONT_CARE;
  attrib_list[n] = (int)None;
  
  int num_configs = 0;
  GLXFBConfig *configs =
    _glXChooseFBConfig(_display, _screen, attrib_list, &num_configs);
  
  if (configs != 0) {
    bool context_has_pbuffer, context_has_pixmap, slow;
    int quality, i;
    for (i = 0; i < num_configs; ++i) {
      FrameBufferProperties fbprops;
      get_properties_advanced(fbprops, context_has_pbuffer, context_has_pixmap,
                              slow, configs[i]);
      quality = fbprops.get_quality(properties);
      if ((quality > 0)&&(slow)) quality -= 10000000;
      if (glxdisplay_cat.is_debug()) {
        const char *pbuffertext = context_has_pbuffer ? " (pbuffer)" : "";
        const char *pixmaptext = context_has_pixmap ? " (pixmap)" : "";
        const char *slowtext = slow ? " (slow)" : "";
        glxdisplay_cat.debug()
          << i << ": " << fbprops << " quality=" << quality << pbuffertext << pixmaptext << slowtext << "\n";
      }
      if (need_pbuffer && !context_has_pbuffer) {
        continue;
      }
      if (need_pixmap && !context_has_pixmap) {
        continue;
      }
      
      if (quality > best_quality) {
        best_quality = quality;
        best_result = i;
        best_props = fbprops;
      }
    }
  }

  if (best_quality > 0) {
    _fbconfig = configs[best_result];
    _context = 
      _glXCreateNewContext(_display, _fbconfig, GLX_RGBA_TYPE, _share_context,
                           GL_TRUE);
    if (_context) {
      if (_visuals != (XVisualInfo *)NULL) {
        XFree(_visuals);
        _visuals = NULL;
      }
      _visuals = _glXGetVisualFromFBConfig(_display, _fbconfig);
      _visual = _visuals;

      if (_visual) {
        get_properties_advanced(_fbprops, _context_has_pbuffer, _context_has_pixmap,
                                _slow, _fbconfig);

        if (glxdisplay_cat.is_debug()) {
          glxdisplay_cat.debug()
            << "Selected context " << best_result << ": " << _fbprops << "\n";
          glxdisplay_cat.debug()
            << "context_has_pbuffer = " << _context_has_pbuffer
            << ", context_has_pixmap = " << _context_has_pixmap << "\n";
        }

        return;
      }
    }
    // This really shouldn't happen, so I'm not too careful about cleanup.
    glxdisplay_cat.error()
      << "Could not create FBConfig context!\n";
    _fbconfig = 0;
    _context = 0;
    _visual = 0;
    _visuals = 0;
  }

  glxdisplay_cat.info()
    << "No suitable FBConfig contexts available.\n";
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::reset
//       Access: Public, Virtual
//  Description: Resets all internal state as if the gsg were newly
//               created.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
reset() {
  GLGraphicsStateGuardian::reset();

  _supports_swap_control = has_extension("GLX_SGI_swap_control");

  if (_supports_swap_control) {
    _glXSwapIntervalSGI = 
      (PFNGLXSWAPINTERVALSGIPROC)get_extension_func("glX", "SwapIntervalSGI");
    if (_glXSwapIntervalSGI == NULL) {
      glxdisplay_cat.error()
        << "Driver claims to support GLX_SGI_swap_control extension, but does not define all functions.\n";
      _supports_swap_control = false;
    }
  }

  if (_supports_swap_control) {
    // Set the video-sync setting up front, if we have the extension
    // that supports it.
    _glXSwapIntervalSGI(sync_video ? 1 : 0);
  }

  if (glx_support_fbconfig) {
    if (glx_is_at_least_version(1, 3)) {
      // If we have glx 1.3 or better, we have the FBConfig interface.
      _supports_fbconfig = true;
      
      _glXChooseFBConfig = 
        (PFNGLXCHOOSEFBCONFIGPROC)get_extension_func("glX", "ChooseFBConfig");
      _glXCreateNewContext = 
        (PFNGLXCREATENEWCONTEXTPROC)get_extension_func("glX", "CreateNewContext");
      _glXGetVisualFromFBConfig = 
        (PFNGLXGETVISUALFROMFBCONFIGPROC)get_extension_func("glX", "GetVisualFromFBConfig");
      _glXGetFBConfigAttrib = 
        (PFNGLXGETFBCONFIGATTRIBPROC)get_extension_func("glX", "GetFBConfigAttrib");
      _glXCreatePixmap = 
        (PFNGLXCREATEPIXMAPPROC)get_extension_func("glX", "CreatePixmap");
      
      if (_glXChooseFBConfig == NULL ||
          _glXCreateNewContext == NULL ||
          _glXGetVisualFromFBConfig == NULL ||
          _glXGetFBConfigAttrib == NULL ||
          _glXCreatePixmap == NULL) {
        glxdisplay_cat.error()
          << "Driver claims to support GLX_fbconfig extension, but does not define all functions.\n";
        _supports_fbconfig = false;
      }
    } else if (has_extension("GLX_SGIX_fbconfig")) {
      // Or maybe we have the old SGIX extension for FBConfig.  This is
      // the same, but the function names are different--we just remap
      // them to the same function pointers.
      _supports_fbconfig = true;
      
      _glXChooseFBConfig = 
        (PFNGLXCHOOSEFBCONFIGPROC)get_extension_func("glX", "ChooseFBConfigSGIX");
      _glXCreateNewContext = 
        (PFNGLXCREATENEWCONTEXTPROC)get_extension_func("glX", "CreateContextWithConfigSGIX");
      _glXGetVisualFromFBConfig = 
        (PFNGLXGETVISUALFROMFBCONFIGPROC)get_extension_func("glX", "GetVisualFromFBConfigSGIX");
      _glXGetFBConfigAttrib = 
        (PFNGLXGETFBCONFIGATTRIBPROC)get_extension_func("glX", "GetFBConfigAttribSGIX");
      _glXCreatePixmap = 
        (PFNGLXCREATEPIXMAPPROC)get_extension_func("glX", "CreateGLXPixmapWithConfigSGIX");
      
      if (_glXChooseFBConfig == NULL ||
          _glXCreateNewContext == NULL ||
          _glXGetVisualFromFBConfig == NULL ||
          _glXGetFBConfigAttrib == NULL ||
          _glXCreatePixmap == NULL) {
        glxdisplay_cat.error()
          << "Driver claims to support GLX_SGIX_fbconfig extension, but does not define all functions.\n";
        _supports_fbconfig = false;
      }
    }
    
    if (glx_is_at_least_version(1, 3)) {
      // If we have glx 1.3 or better, we have the PBuffer interface.
      _supports_pbuffer = true;
      _uses_sgix_pbuffer = false;
      
      _glXCreatePbuffer = 
        (PFNGLXCREATEPBUFFERPROC)get_extension_func("glX", "CreatePbuffer");
      _glXCreateGLXPbufferSGIX = NULL;
      _glXDestroyPbuffer = 
        (PFNGLXDESTROYPBUFFERPROC)get_extension_func("glX", "DestroyPbuffer");
      if (_glXCreatePbuffer == NULL ||
          _glXDestroyPbuffer == NULL) {
        glxdisplay_cat.error()
          << "Driver claims to support GLX_pbuffer extension, but does not define all functions.\n";
        _supports_pbuffer = false;
      }
      
    } else if (has_extension("GLX_SGIX_pbuffer")) {
      // Or maybe we have the old SGIX extension for PBuffers.
      _uses_sgix_pbuffer = true;
      
      // CreatePbuffer has a different form between SGIX and 1.3,
      // however, so we must treat it specially.  But we can use the
      // same function pointer for DestroyPbuffer.
      _glXCreatePbuffer = NULL;
      _glXCreateGLXPbufferSGIX = 
        (PFNGLXCREATEGLXPBUFFERSGIXPROC)get_extension_func("glX", "CreateGLXPbufferSGIX");
      _glXDestroyPbuffer = 
        (PFNGLXDESTROYPBUFFERPROC)get_extension_func("glX", "DestroyGLXPbufferSGIX");
      if (_glXCreateGLXPbufferSGIX == NULL ||
          _glXDestroyPbuffer == NULL) {
        glxdisplay_cat.error()
          << "Driver claims to support GLX_SGIX_pbuffer extension, but does not define all functions.\n";
        _supports_pbuffer = false;
      }
    }
  }


  if (glxdisplay_cat.is_debug()) {
    glxdisplay_cat.debug()
      << "supports_swap_control = " << _supports_swap_control << "\n";
    glxdisplay_cat.debug()
      << "supports_fbconfig = " << _supports_fbconfig << "\n";
    glxdisplay_cat.debug()
      << "supports_pbuffer = " << _supports_pbuffer
      << " sgix = " << _uses_sgix_pbuffer << "\n";
  }

  // If "Mesa" is present, assume software.  However, if "Mesa DRI" is
  // found, it's actually a Mesa-based OpenGL layer running over a
  // hardware driver.
  if (_gl_renderer.find("Mesa") != string::npos &&
      _gl_renderer.find("Mesa DRI") == string::npos) {
    // It's Mesa, therefore probably a software context.
    _fbprops.set_force_software(1);
    _fbprops.set_force_hardware(0);
  } else {
    _fbprops.set_force_hardware(1);
    _fbprops.set_force_software(0);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::glx_is_at_least_version
//       Access: Public
//  Description: Returns true if the runtime GLX version number is at
//               least the indicated value, false otherwise.
////////////////////////////////////////////////////////////////////
bool glxGraphicsStateGuardian::
glx_is_at_least_version(int major_version, int minor_version) const {
  if (_glx_version_major < major_version) {
    return false;
  }
  if (_glx_version_minor < minor_version) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::gl_flush
//       Access: Protected, Virtual
//  Description: Calls glFlush().
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
gl_flush() const {
  // This call requires synchronization with X.
  LightReMutexHolder holder(glxGraphicsPipe::_x_mutex);
  GLGraphicsStateGuardian::gl_flush();
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::gl_get_error
//       Access: Protected, Virtual
//  Description: Returns the result of glGetError().
////////////////////////////////////////////////////////////////////
GLenum glxGraphicsStateGuardian::
gl_get_error() const {
  // This call requires synchronization with X.
  LightReMutexHolder holder(glxGraphicsPipe::_x_mutex);
  return GLGraphicsStateGuardian::gl_get_error();
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::query_gl_version
//       Access: Protected, Virtual
//  Description: Queries the runtime version of OpenGL in use.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
query_gl_version() {
  GLGraphicsStateGuardian::query_gl_version();

  show_glx_client_string("GLX_VENDOR", GLX_VENDOR);
  show_glx_client_string("GLX_VERSION", GLX_VERSION);
  show_glx_server_string("GLX_VENDOR", GLX_VENDOR);
  show_glx_server_string("GLX_VERSION", GLX_VERSION);

  glXQueryVersion(_display, &_glx_version_major, &_glx_version_minor);

  // We output to glgsg_cat instead of glxdisplay_cat, since this is
  // where the GL version has been output, and it's nice to see the
  // two of these together.
  if (glgsg_cat.is_debug()) {
    glgsg_cat.debug()
      << "GLX_VERSION = " << _glx_version_major << "." << _glx_version_minor 
      << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::get_extra_extensions
//       Access: Protected, Virtual
//  Description: This may be redefined by a derived class (e.g. glx or
//               wgl) to get whatever further extensions strings may
//               be appropriate to that interface, in addition to the
//               GL extension strings return by glGetString().
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
get_extra_extensions() {
  save_extensions(glXQueryExtensionsString(_display, _screen));
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::do_get_extension_func
//       Access: Public, Virtual
//  Description: Returns the pointer to the GL extension function with
//               the indicated name.  It is the responsibility of the
//               caller to ensure that the required extension is
//               defined in the OpenGL runtime prior to calling this;
//               it is an error to call this for a function that is
//               not defined.
////////////////////////////////////////////////////////////////////
void *glxGraphicsStateGuardian::
do_get_extension_func(const char *prefix, const char *name) {
  nassertr(prefix != NULL, NULL);
  nassertr(name != NULL, NULL);
  string fullname = string(prefix) + string(name);

  if (glx_get_proc_address) {
    // First, check if we have glXGetProcAddress available.  This will
    // be superior if we can get it.
    
#if defined(LINK_IN_GLXGETPROCADDRESS) && defined(HAVE_GLXGETPROCADDRESS)
      // If we are confident the system headers defined it, we can
      // call it directly.  This is more reliable than trying to
      // determine its address dynamically, but it may make
      // libpandagl.so fail to load if the symbol isn't in the runtime
      // library.
    return (void *)glXGetProcAddress((const GLubyte *)fullname.c_str());
      
#elif defined(LINK_IN_GLXGETPROCADDRESS) && defined(HAVE_GLXGETPROCADDRESSARB)
    // The ARB extension version is OK too.  Sometimes the prototype
    // isn't supplied for some reason.
    return (void *)glXGetProcAddressARB((const GLubyte *)fullname.c_str());
    
#else
    // Otherwise, we have to fiddle around with the dynamic runtime.
    
    if (!_checked_get_proc_address) {
      const char *funcName = NULL;
      
      if (glx_is_at_least_version(1, 4)) {
        funcName = "glXGetProcAddress";

      } else if (has_extension("GLX_ARB_get_proc_address")) {
        funcName = "glXGetProcAddressARB";
      }
      
      if (funcName != NULL) {
        _glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)get_system_func(funcName);
        if (_glXGetProcAddress == NULL) {
          glxdisplay_cat.warning()
            << "Couldn't load function " << funcName
            << ", GL extensions may be unavailable.\n";
        }
      }

      _checked_get_proc_address = true;
    }
    
    // Use glxGetProcAddress() if we've got it; it should be more robust.
    if (_glXGetProcAddress != NULL) {
      return (void *)_glXGetProcAddress((const GLubyte *)fullname.c_str());
    }
#endif // HAVE_GLXGETPROCADDRESS
  }

  if (glx_get_os_address) {
    // Otherwise, fall back to the OS-provided calls.
    return get_system_func(fullname.c_str());
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::get_system_func
//       Access: Private
//  Description: Support for get_extension_func(), above, that uses
//               system calls to find a GL or GLX function (in the
//               absence of a working glxGetProcAddress() function to
//               call).
////////////////////////////////////////////////////////////////////
void *glxGraphicsStateGuardian::
get_system_func(const char *name) {
  if (_libgl_handle == (void *)NULL) {
    // We open the current executable, rather than naming a particular
    // library.  Presumably libGL.so (or whatever the library should
    // be called) is already available in the current executable
    // address space, so this is more portable than insisting on a
    // particular shared library name.
    _libgl_handle = dlopen(NULL, RTLD_LAZY);
    nassertr(_libgl_handle != (void *)NULL, NULL);

    // If that doesn't locate the symbol we expected, then fall back
    // to loading the GL library by its usual name.
    if (dlsym(_libgl_handle, name) == NULL) {
      dlclose(_libgl_handle);
      glxdisplay_cat.warning()
        << name << " not found in executable; looking in libGL.so instead.\n";
      _libgl_handle = dlopen("libGL.so", RTLD_LAZY);
      nassertr(_libgl_handle != (void *)NULL, NULL);
    }
  }

  return dlsym(_libgl_handle, name);
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::show_glx_client_string
//       Access: Protected
//  Description: Outputs the result of glxGetClientString() on the
//               indicated tag.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
show_glx_client_string(const string &name, int id) {
  if (glgsg_cat.is_debug()) {
    const char *text = glXGetClientString(_display, id);
    if (text == (const char *)NULL) {
      glgsg_cat.debug()
        << "Unable to query " << name << " (client)\n";
    } else {
      glgsg_cat.debug()
        << name << " (client) = " << (const char *)text << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::show_glx_server_string
//       Access: Protected
//  Description: Outputs the result of glxQueryServerString() on the
//               indicated tag.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
show_glx_server_string(const string &name, int id) {
  if (glgsg_cat.is_debug()) {
    const char *text = glXQueryServerString(_display, _screen, id);
    if (text == (const char *)NULL) {
      glgsg_cat.debug()
        << "Unable to query " << name << " (server)\n";
    } else {
      glgsg_cat.debug()
        << name << " (server) = " << (const char *)text << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::choose_temp_visual
//       Access: Private
//  Description: Selects an XVisual for an initial OpenGL context.
//               This may be called initially, to create the first
//               context needed in order to create the fbconfig.  On
//               successful return, _visual and _temp_context will be
//               filled in with a non-NULL value.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
choose_temp_visual(const FrameBufferProperties &properties) {
  nassertv(_temp_context == (GLXContext)NULL);

  int best_quality = 0;
  int best_result = 0;
  FrameBufferProperties best_props;

  // Scan available visuals.
  if (_visuals != (XVisualInfo *)NULL) {
    XFree(_visuals);
    _visuals = NULL;
  }
  int nvisuals = 0;
  _visuals = XGetVisualInfo(_display, 0, 0, &nvisuals);
  if (_visuals != 0) {
    for (int i = 0; i < nvisuals; i++) {
      FrameBufferProperties fbprops;
      get_properties(fbprops, _visuals + i);
      int quality = fbprops.get_quality(properties);
      if (quality > best_quality) {
        best_quality = quality;
        best_result = i;
        best_props = fbprops;
      }
    }
  }
  
  if (best_quality > 0) {
    _visual = _visuals + best_result;
    _temp_context = glXCreateContext(_display, _visual, None, GL_TRUE);    
    if (_temp_context) {
      _fbprops = best_props;
      return;
    }
  }

  glxdisplay_cat.error() 
    << "Could not find a usable pixel format.\n";
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::init_temp_context
//       Access: Private
//  Description: Initializes the context created in
//               choose_temp_visual() by creating a temporary window
//               and binding the context to that window.
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
init_temp_context() {
  x11GraphicsPipe *x11_pipe;
  DCAST_INTO_V(x11_pipe, get_pipe());
  Window root_window = x11_pipe->get_root();

  // Assume everyone uses TrueColor or DirectColor these days.
  Visual *visual = _visual->visual;
  nassertv(visual->c_class == DirectColor || visual->c_class == TrueColor);
  _temp_colormap = XCreateColormap(_display, root_window,
                                   visual, AllocNone);
  XSetWindowAttributes wa;
  wa.colormap = _temp_colormap;
  unsigned long attrib_mask = CWColormap;

  _temp_xwindow = XCreateWindow
    (_display, root_window, 0, 0, 100, 100,
     0, _visual->depth, InputOutput,
     visual, attrib_mask, &wa);
  if (_temp_xwindow == (Window)NULL) {
    glxdisplay_cat.error()
      << "Could not create temporary window for context\n";
    return;
  }

  glXMakeCurrent(_display, _temp_xwindow, _temp_context);
  reset();
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsStateGuardian::destroy_temp_xwindow
//       Access: Private
//  Description: Destroys the temporary unmapped window created by
//               init_temp_context().
////////////////////////////////////////////////////////////////////
void glxGraphicsStateGuardian::
destroy_temp_xwindow() {
  glXMakeCurrent(_display, None, NULL);

  if (_temp_colormap != (Colormap)NULL) {
    XFreeColormap(_display, _temp_colormap);
    _temp_colormap = (Colormap)NULL;
  }
  if (_temp_xwindow != (Window)NULL) {
    XDestroyWindow(_display, _temp_xwindow);
    _temp_xwindow = (Window)NULL;
  }

  if (_temp_context != (GLXContext)NULL){ 
    glXDestroyContext(_display, _temp_context);
    _temp_context = (GLXContext)NULL;
  }
}
