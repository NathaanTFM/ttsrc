// Filename: webcamVideo.h
// Created by: jyelon (01Nov2007)
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

#ifndef WEBCAMVIDEO_H
#define WEBCAMVIDEO_H

#include "movieVideo.h"

////////////////////////////////////////////////////////////////////
//       Class : WebcamVideo
// Description : Allows you to open a webcam or other video capture
//               device as a video stream.
////////////////////////////////////////////////////////////////////
class EXPCL_VISION WebcamVideo : public MovieVideo {

PUBLISHED:
  virtual ~WebcamVideo();

  static int             get_num_options();
  static PT(WebcamVideo) get_option(int n);
  MAKE_SEQ(get_options, get_num_options, get_option);
  
  INLINE int get_size_x() const;
  INLINE int get_size_y() const;
  INLINE int get_fps() const;
  
  virtual PT(MovieVideoCursor) open() = 0;

public:
  static void find_all_webcams();

protected:
  int _size_x;
  int _size_y;
  int _fps;

  static pvector<PT(WebcamVideo)> _all_webcams;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    MovieVideo::init_type();
    register_type(_type_handle, "WebcamVideo",
                  MovieVideo::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "webcamVideo.I"

#endif
