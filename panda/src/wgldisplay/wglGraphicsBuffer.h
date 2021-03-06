// Filename: wglGraphicsBuffer.h
// Created by:  drose (08Feb04)
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

#ifndef WGLGRAPHICSBUFFER_H
#define WGLGRAPHICSBUFFER_H

#include "pandabase.h"
#include "graphicsBuffer.h"
#include "glgsg.h"

// This must be included after we have included glgsg.h (which
// includes gl.h).
#include "wglext.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

////////////////////////////////////////////////////////////////////
//       Class : wglGraphicsBuffer
// Description : An offscreen render buffer.  In OpenGL under Windows,
//               this simply renders into a window that is never made
//               visible.  There's a Windows interface for rendering
//               into a DIB, but this puts restrictions on the kind of
//               pixelformat we can use, and thus makes it difficult
//               to support one GSG rendering into an offscreen buffer
//               and also into a window.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAGL wglGraphicsBuffer : public GraphicsBuffer {
public:
  wglGraphicsBuffer(GraphicsEngine *engine, GraphicsPipe *pipe, 
                    const string &name,
                    const FrameBufferProperties &fb_prop,
                    const WindowProperties &win_prop,
                    int flags,
                    GraphicsStateGuardian *gsg,
                    GraphicsOutput *host);
  virtual ~wglGraphicsBuffer();

  virtual bool begin_frame(FrameMode mode, Thread *current_thread);
  virtual void end_frame(FrameMode mode, Thread *current_thread);

  virtual void select_cube_map(int cube_map_index);

  virtual void process_events();

protected:
  virtual void close_buffer();
  virtual bool open_buffer();

private:
  void bind_texture_to_pbuffer();
  bool rebuild_bitplanes();
  void release_pbuffer();

  static void process_1_event();

  HPBUFFERARB _pbuffer;
  HDC _pbuffer_dc;
  bool _pbuffer_mipmap;
  Texture::TextureType _pbuffer_type;
  int _pbuffer_sizex;
  int _pbuffer_sizey;
  PT(Texture) _pbuffer_bound;
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    GraphicsBuffer::init_type();
    register_type(_type_handle, "wglGraphicsBuffer",
                  GraphicsBuffer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class wglGraphicsStateGuardian;
};

#include "wglGraphicsBuffer.I"

#endif
