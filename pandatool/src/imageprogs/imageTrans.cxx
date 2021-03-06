// Filename: imageTrans.cxx
// Created by:  drose (19Jun00)
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

#include "imageTrans.h"
#include "string_utils.h"
#include "pystub.h"

////////////////////////////////////////////////////////////////////
//     Function: ImageTrans::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
ImageTrans::
ImageTrans() : ImageFilter(true) {
  set_program_description
    ("This program reads an image file and writes a similar "
     "image file to the output.  It can implicitly convert from one image "
     "file format to another; it uses the extension of the output filename "
     "to specify the destination file format.");

  add_option
    ("chan", "channels", 50,
     "Elevate (or truncate) the image to the indicated number of channels.  "
     "This may be 1, 2, 3, or 4.  You may also specify one of the keywords "
     "l, la, rgb, or rgba, respectively, or any of the keywords r, g, b, or "
     "a to extract out just the indicated channel as a single grayscale "
     "image.",
     &ImageTrans::dispatch_channels, NULL, &_channels);

  add_option
    ("cscale", "r,g,b[,a]", 50,
     "Apply the indicated color scale to each pixel of the image.",
     &ImageTrans::dispatch_color, &_has_color_scale, &_color_scale);

  _channels = C_default;
  _color_scale.set(1.0f, 1.0f, 1.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////
//     Function: ImageTrans::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void ImageTrans::
run() {
  switch (_channels) {
  case C_default:
    break;

  case C_l:
  case C_la:
  case C_rgb:
  case C_rgba:
    _image.set_num_channels((int)_channels);
    break;
    
  case C_r:
    _image.make_grayscale(1.0, 0.0, 0.0);
    _image.remove_alpha();
    break;

  case C_g:
    _image.make_grayscale(0.0, 1.0, 0.0);
    _image.remove_alpha();
    break;

  case C_b:
    _image.make_grayscale(0.0, 0.0, 1.0);
    _image.remove_alpha();
    break;

  case C_a:
    extract_alpha();
    break;
  }

  if (_has_color_scale) {
    if (_color_scale[0] != 1.0f ||
        _color_scale[1] != 1.0f ||
        _color_scale[2] != 1.0f) {
      for (int yi = 0; yi < _image.get_y_size(); ++yi) {
        for (int xi = 0; xi < _image.get_x_size(); ++xi) {
          RGBColord rgb = _image.get_xel(xi, yi);
          _image.set_xel(xi, yi, 
                         rgb[0] * _color_scale[0],
                         rgb[1] * _color_scale[1],
                         rgb[2] * _color_scale[2]);
        }
      }
    }
    if (_image.has_alpha() && _color_scale[3] != 1.0f) {
      for (int yi = 0; yi < _image.get_y_size(); ++yi) {
        for (int xi = 0; xi < _image.get_x_size(); ++xi) {
          float a = _image.get_alpha(xi, yi);
          _image.set_alpha(xi, yi, a * _color_scale[3]);
        }
      }
    }
  }

  write_image();
}

////////////////////////////////////////////////////////////////////
//     Function: ImageTrans::dispatch_channels
//       Access: Private, Static
//  Description: Interprets the -chan parameter.
////////////////////////////////////////////////////////////////////
bool ImageTrans::
dispatch_channels(const string &opt, const string &arg, void *var) {
  Channels *ip = (Channels *)var;
  if (cmp_nocase(arg, "l") == 0) {
    (*ip) = C_l;
  } else if (cmp_nocase(arg, "la") == 0) {
    (*ip) = C_la;
  } else if (cmp_nocase(arg, "rgb") == 0) {
    (*ip) = C_rgb;
  } else if (cmp_nocase(arg, "rgba") == 0) {
    (*ip) = C_rgba;
  } else if (cmp_nocase(arg, "r") == 0) {
    (*ip) = C_r;
  } else if (cmp_nocase(arg, "g") == 0) {
    (*ip) = C_g;
  } else if (cmp_nocase(arg, "b") == 0) {
    (*ip) = C_b;
  } else if (cmp_nocase(arg, "a") == 0) {
    (*ip) = C_a;
  } else {
    int value;
    if (!string_to_int(arg, value)) {
      nout << "Invalid parameter for -" << opt << ": "
           << arg << "\n";
      return false;
    }
    if (value < 1 || value > 4) {
      nout << "Number of channels must be one of 1, 2, 3, or 4.\n";
      return false;
    }
    (*ip) = (Channels)value;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: ImageTrans::extract_alpha
//       Access: Private
//  Description: Extracts out just the alpha channel and stores it as
//               a grayscale image.
////////////////////////////////////////////////////////////////////
void ImageTrans::
extract_alpha() {
  if (!_image.has_alpha()) {
    nout << "Source image does not have an alpha channel!\n";
    _image.make_grayscale();
    _image.fill();
    return;
  }

  _image.make_grayscale();
  for (int y = 0; y < _image.get_y_size(); y++) {
    for (int x = 0; x < _image.get_x_size(); x++) {
      _image.set_gray_val(x, y, _image.get_alpha_val(x, y));
    }
  }
  _image.remove_alpha();
}


int main(int argc, char *argv[]) {
  // A call to pystub() to force libpystub.so to be linked in.
  pystub();

  ImageTrans prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}
