// Filename: eglGraphicsPixmap.cxx
// Created by:  pro-rsoft (13Jun09)
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

#include "eglGraphicsPixmap.h"
#include "eglGraphicsWindow.h"
#include "eglGraphicsStateGuardian.h"
#include "config_egldisplay.h"
#include "eglGraphicsPipe.h"

#include "graphicsPipe.h"
#include "pStatTimer.h"

TypeHandle eglGraphicsPixmap::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
eglGraphicsPixmap::
eglGraphicsPixmap(GraphicsEngine *engine, GraphicsPipe *pipe, 
                  const string &name,
                  const FrameBufferProperties &fb_prop,
                  const WindowProperties &win_prop,
                  int flags,
                  GraphicsStateGuardian *gsg,
                  GraphicsOutput *host) :
  GraphicsBuffer(engine, pipe, name, fb_prop, win_prop, flags, gsg, host)
{
  eglGraphicsPipe *egl_pipe;
  DCAST_INTO_V(egl_pipe, _pipe);
  _display = egl_pipe->get_display();
  _egl_display = egl_pipe->_egl_display;
  _drawable = None;
  _x_pixmap = None;
  _egl_surface = EGL_NO_SURFACE;

  // Since the pixmap never gets flipped, we get screenshots from the
  // same pixmap we draw into.
  _screenshot_buffer_type = _draw_buffer_type;
}

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
eglGraphicsPixmap::
~eglGraphicsPixmap() {
  nassertv(_x_pixmap == None && _egl_surface == EGL_NO_SURFACE);
}

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::begin_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               before beginning rendering for a given frame.  It
//               should do whatever setup is required, and return true
//               if the frame should be rendered, or false if it
//               should be skipped.
////////////////////////////////////////////////////////////////////
bool eglGraphicsPixmap::
begin_frame(FrameMode mode, Thread *current_thread) {
  PStatTimer timer(_make_current_pcollector, current_thread);

  begin_frame_spam(mode);
  if (_gsg == (GraphicsStateGuardian *)NULL) {
    return false;
  }

  eglGraphicsStateGuardian *eglgsg;
  DCAST_INTO_R(eglgsg, _gsg, false);
  if (!eglMakeCurrent(_egl_display, _egl_surface, _egl_surface, eglgsg->_context)) {
    egldisplay_cat.error() << "Failed to call eglMakeCurrent: "
      << get_egl_error_string(eglGetError()) << "\n";
  }

  // Now that we have made the context current to a window, we can
  // reset the GSG state if this is the first time it has been used.
  // (We can't just call reset() when we construct the GSG, because
  // reset() requires having a current context.)
  eglgsg->reset_if_new();

  if (mode == FM_render) {
    for (int i=0; i<count_textures(); i++) {
      if (get_rtm_mode(i) == RTM_bind_or_copy) {
        _textures[i]._rtm_mode = RTM_copy_texture;
      }
    }
    clear_cube_map_selection();
  }
  
  _gsg->set_current_properties(&get_fb_properties());
  return _gsg->begin_frame(current_thread);
}

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::end_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after rendering is completed for a given frame.  It
//               should do whatever finalization is required.
////////////////////////////////////////////////////////////////////
void eglGraphicsPixmap::
end_frame(FrameMode mode, Thread *current_thread) {
  end_frame_spam(mode);
  nassertv(_gsg != (GraphicsStateGuardian *)NULL);

  if (mode == FM_render) {
    copy_to_textures();
  }

  _gsg->end_frame(current_thread);

  if (mode == FM_render) {
    trigger_flip();
    if (_one_shot) {
      prepare_for_deletion();
    }
    clear_cube_map_selection();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::close_buffer
//       Access: Protected, Virtual
//  Description: Closes the pixmap right now.  Called from the window
//               thread.
////////////////////////////////////////////////////////////////////
void eglGraphicsPixmap::
close_buffer() {
  if (_gsg != (GraphicsStateGuardian *)NULL) {
    if (!eglMakeCurrent(_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
      egldisplay_cat.error() << "Failed to call eglMakeCurrent: "
        << get_egl_error_string(eglGetError()) << "\n";
    }
    _gsg.clear();
    _active = false;
  }

  if (_egl_surface != EGL_NO_SURFACE) {
    if (!eglDestroySurface(_egl_display, _egl_surface)) {
      egldisplay_cat.error() << "Failed to destroy surface: "
        << get_egl_error_string(eglGetError()) << "\n";
    }
    _egl_surface = EGL_NO_SURFACE;
  }

  if (_x_pixmap != None) {
    XFreePixmap(_display, _x_pixmap);
    _x_pixmap = None;
  }

  _is_valid = false;
}

////////////////////////////////////////////////////////////////////
//     Function: eglGraphicsPixmap::open_buffer
//       Access: Protected, Virtual
//  Description: Opens the pixmap right now.  Called from the window
//               thread.  Returns true if the pixmap is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool eglGraphicsPixmap::
open_buffer() {
  eglGraphicsPipe *egl_pipe;
  DCAST_INTO_R(egl_pipe, _pipe, false);

  // GSG Creation/Initialization
  eglGraphicsStateGuardian *eglgsg;
  if (_gsg == 0) {
    // There is no old gsg.  Create a new one.
    eglgsg = new eglGraphicsStateGuardian(_engine, _pipe, NULL);
    eglgsg->choose_pixel_format(_fb_properties, _display, egl_pipe->get_screen(), false, true);
    _gsg = eglgsg;
  } else {
    // If the old gsg has the wrong pixel format, create a
    // new one that shares with the old gsg.
    DCAST_INTO_R(eglgsg, _gsg, false);
    if (!eglgsg->get_fb_properties().subsumes(_fb_properties)) {
      eglgsg = new eglGraphicsStateGuardian(_engine, _pipe, eglgsg);
      eglgsg->choose_pixel_format(_fb_properties, _display, egl_pipe->get_screen(), false, true);
      _gsg = eglgsg;
    }
  }
  
  if (eglgsg->_fbconfig == None) {
    // If we didn't use an fbconfig to create the GSG, we can't create
    // a PBuffer.
    return false;
  }

  XVisualInfo *visual_info = eglgsg->_visual;
  if (visual_info == NULL) {
    // No X visual for this fbconfig; how can we create the pixmap?
    egldisplay_cat.error()
      << "No X visual: cannot create pixmap.\n";
    return false;
  }

  _drawable = egl_pipe->get_root();
  if (_host != NULL) {
    if (_host->is_of_type(eglGraphicsWindow::get_class_type())) {
      eglGraphicsWindow *win = DCAST(eglGraphicsWindow, _host);
      _drawable = win->get_xwindow();
    } else if (_host->is_of_type(eglGraphicsPixmap::get_class_type())) {
      eglGraphicsPixmap *pix = DCAST(eglGraphicsPixmap, _host);
      _drawable = pix->_drawable;
    }
  }

  _x_pixmap = XCreatePixmap(_display, _drawable, 
                            _x_size, _y_size, visual_info->depth);
  if (_x_pixmap == None) {
    egldisplay_cat.error()
      << "Failed to create X pixmap.\n";
    close_buffer();
    return false;
  }

  nassertr(eglgsg->_fbconfig, false);
  _egl_surface = eglCreatePixmapSurface(_egl_display, eglgsg->_fbconfig, (NativePixmapType) _x_pixmap, NULL);

  if (_egl_surface == EGL_NO_SURFACE) {
    egldisplay_cat.error()
      << "Failed to create EGL pixmap surface:"
      << get_egl_error_string(eglGetError()) << "\n";
    close_buffer();
    return false;
  }

  eglMakeCurrent(_egl_display, _egl_surface, _egl_surface, eglgsg->_context);
  eglgsg->reset_if_new();
  if (!eglgsg->is_valid()) {
    close_buffer();
    return false;
  }
  if (!eglgsg->get_fb_properties().verify_hardware_software
      (_fb_properties, eglgsg->get_gl_renderer())) {
    close_buffer();
    return false;
  }
  _fb_properties = eglgsg->get_fb_properties();
  
  _is_valid = true;
  return true;
}
