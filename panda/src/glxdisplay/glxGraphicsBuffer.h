// Filename: glxGraphicsBuffer.h
// Created by:  drose (09Feb04)
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

#ifndef GLXGRAPHICSBUFFER_H
#define GLXGRAPHICSBUFFER_H

#include "pandabase.h"

#include "glxGraphicsPipe.h"
#include "graphicsBuffer.h"

#ifdef HAVE_GLXFBCONFIG
// This whole class doesn't make sense unless we have the GLXFBConfig
// and associated GLXPbuffer interfaces available.

////////////////////////////////////////////////////////////////////
//       Class : glxGraphicsBuffer
// Description : An offscreen buffer in the GLX environment.  This
//               creates a GLXPbuffer.
////////////////////////////////////////////////////////////////////
class glxGraphicsBuffer : public GraphicsBuffer {
public:
  glxGraphicsBuffer(GraphicsEngine *engine, GraphicsPipe *pipe, 
                    const string &name,
                    const FrameBufferProperties &fb_prop,
                    const WindowProperties &win_prop,
                    int flags,
                    GraphicsStateGuardian *gsg,
                    GraphicsOutput *host);
  virtual ~glxGraphicsBuffer();

  virtual bool begin_frame(FrameMode mode, Thread *current_thread);
  virtual void end_frame(FrameMode mode, Thread *current_thread);

protected:
  virtual void close_buffer();
  virtual bool open_buffer();

private:
  Display *_display;
  GLXPbuffer _pbuffer;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    GraphicsBuffer::init_type();
    register_type(_type_handle, "glxGraphicsBuffer",
                  GraphicsBuffer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "glxGraphicsBuffer.I"

#endif  // HAVE_GLXFBCONFIG

#endif
