// Filename: freetypeFont.cxx
// Created by:  drose (07Sep03)
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

#include "freetypeFont.h"

#ifdef HAVE_FREETYPE

#include "config_pnmtext.h"
#include "config_util.h"
#include "config_express.h"
#include "virtualFileSystem.h"

FT_Library FreetypeFont::_ft_library;
bool FreetypeFont::_ft_initialized = false;
bool FreetypeFont::_ft_ok = false;

// This constant determines how big a particular point size font
// appears to be.  By convention, 10 points is 1 unit (e.g. 1 foot)
// high.
const float FreetypeFont::_points_per_unit = 10.0f;

// A universal typographic convention.
const float FreetypeFont::_points_per_inch = 72.0f;

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::Constructor
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
FreetypeFont::
FreetypeFont() {
  _font_loaded = false;

  _face = NULL;

  _point_size = text_point_size;
  _requested_pixels_per_unit = text_pixels_per_unit;
  _tex_pixels_per_unit = text_pixels_per_unit;
  _requested_scale_factor = text_scale_factor;
  _scale_factor = text_scale_factor;
  _native_antialias = text_native_antialias;

  _font_pixel_size = 0;
  _line_height = 1.0f;
  _space_advance = 0.25f;

  if (!_ft_initialized) {
    initialize_ft_library();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::load_font
//       Access: Protected
//  Description: This method accepts the name of some font file
//               that FreeType can read, along with face_index,
//               indicating which font within the file to load
//               (usually 0).
////////////////////////////////////////////////////////////////////
bool FreetypeFont::
load_font(const Filename &font_filename, int face_index) {
  if (!_ft_ok) {
    pnmtext_cat.error()
      << "Unable to read font " << font_filename
      << ": FreeType library not initialized properly.\n";
    return false;
  }

  unload_font();

  bool exists = false;
  int error;
  Filename path(font_filename);
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  vfs->resolve_filename(path, get_model_path());
  exists = vfs->read_file(path, _raw_font_data, true);
  if (exists) {
    FT_Face face;
    error = FT_New_Memory_Face(_ft_library, 
                               (const FT_Byte *)_raw_font_data.data(),
                               _raw_font_data.length(),
                               face_index, &face);
    _face = new FreetypeFace();
    _face->set_face(face);
  }

  if (!exists) {
    pnmtext_cat.error()
      << "Unable to find font file " << font_filename << "\n";
  } else {
    if (error == FT_Err_Unknown_File_Format) {
      pnmtext_cat.error()
        << "Unable to read font " << font_filename << ": unknown file format.\n";
    } else if (error) {
      pnmtext_cat.error()
        << "Unable to read font " << font_filename << ": invalid.\n";

    } else {
      return font_loaded();
    }
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::load_font
//       Access: Protected
//  Description: This method accepts a table of data representing
//               the font file, loaded from some source other than a
//               filename on disk.
////////////////////////////////////////////////////////////////////
bool FreetypeFont::
load_font(const char *font_data, int data_length, int face_index) {
  if (!_ft_ok) {
    pnmtext_cat.error()
      << "Unable to read font: FreeType library not initialized properly.\n";
    return false;
  }

  unload_font();

  int error;
  FT_Face face;
  error = FT_New_Memory_Face(_ft_library, 
                             (const FT_Byte *)font_data, data_length,
                             face_index, &face);
  _face = new FreetypeFace();
  _face->set_face(face);

  if (error == FT_Err_Unknown_File_Format) {
    pnmtext_cat.error()
      << "Unable to read font: unknown file format.\n";
  } else if (error) {
    pnmtext_cat.error()
      << "Unable to read font: invalid.\n";
    
  } else {
    return font_loaded();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::unload_font
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
void FreetypeFont::
unload_font() {
  if (_font_loaded) {
    _face = NULL;
    _font_loaded = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::load_glyph
//       Access: Protected
//  Description: Invokes Freetype to load and render the indicated
//               glyph into a bitmap.  Returns true if successful,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool FreetypeFont::
load_glyph(int glyph_index, bool prerender) {
  int flags = FT_LOAD_RENDER;
  if (!_native_antialias) { 
    flags |= FT_LOAD_MONOCHROME;
  }

  if (!prerender) {
    // If we want to render as an outline font, don't pre-render it to
    // a bitmap.
    flags = 0;
  }

  int error = FT_Load_Glyph(_face->get_face(), glyph_index, flags);
  if (error) {
    pnmtext_cat.error()
      << "Unable to render glyph " << glyph_index << "\n";
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::copy_bitmap_to_pnmimage
//       Access: Protected
//  Description: Copies a bitmap as rendered by FreeType into a
//               PNMImage, so it can be rescaled.
////////////////////////////////////////////////////////////////////
void FreetypeFont::
copy_bitmap_to_pnmimage(const FT_Bitmap &bitmap, PNMImage &image) {
  if (bitmap.pixel_mode == ft_pixel_mode_grays && 
      bitmap.num_grays == (int)image.get_maxval() + 1) {
    // This is the easy case: we can copy the rendered glyph
    // directly into our image, one pixel at a time.
    unsigned char *buffer_row = bitmap.buffer;
    for (int yi = 0; yi < bitmap.rows; yi++) {
      for (int xi = 0; xi < bitmap.width; xi++) {
        image.set_gray_val(xi, yi, buffer_row[xi]);
      }
      buffer_row += bitmap.pitch;
    }
    
  } else if (bitmap.pixel_mode == ft_pixel_mode_mono) {
    // This is a little bit more work: we have to expand the
    // one-bit-per-pixel bitmap into a one-byte-per-pixel image.
    unsigned char *buffer_row = bitmap.buffer;
    for (int yi = 0; yi < bitmap.rows; yi++) {
      xelval maxval = image.get_maxval();
      int bit = 0x80;
      unsigned char *b = buffer_row;
      for (int xi = 0; xi < bitmap.width; xi++) {
        if (*b & bit) {
          image.set_gray_val(xi, yi, maxval);
        } else {
          image.set_gray_val(xi, yi, 0);
        }
        bit >>= 1;
        if (bit == 0) {
          ++b;
          bit = 0x80;
        }
      }
      
      buffer_row += bitmap.pitch;
    }
    
    
  } else if (bitmap.pixel_mode == ft_pixel_mode_grays) {
    // Here we must expand a grayscale pixmap with n levels of gray
    // into our 256-level texture.
    unsigned char *buffer_row = bitmap.buffer;
    for (int yi = 0; yi < bitmap.rows; yi++) {
      for (int xi = 0; xi < bitmap.width; xi++) {
        image.set_gray(xi, yi, (float)buffer_row[xi] / (bitmap.num_grays - 1));
      }
      buffer_row += bitmap.pitch;
    }
    
  } else {
    pnmtext_cat.error()
      << "Unexpected pixel mode in bitmap: " << (int)bitmap.pixel_mode << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::font_loaded
//       Access: Private
//  Description: Called after a font has been successfully loaded,
//               either from disk or from memory image.
////////////////////////////////////////////////////////////////////
bool FreetypeFont::
font_loaded() {
  string name = _face->get_face()->family_name;
  if (_face->get_face()->style_name != NULL) {
    name += " ";
    name += _face->get_face()->style_name;
  }
  set_name(name);
  
  pnmtext_cat.info()
    << "Loaded font " << name << "\n";
  _font_loaded = true;
  reset_scale();

  if (pnmtext_cat.is_debug()) {
    pnmtext_cat.debug()
      << name << " has " << _face->get_face()->num_charmaps << " charmaps:\n";
    for (int i = 0; i < _face->get_face()->num_charmaps; i++) {
      pnmtext_cat.debug(false) << " " << (void *)_face->get_face()->charmaps[i];
    }
    pnmtext_cat.debug(false) << "\n";
    pnmtext_cat.debug()
      << "default charmap is " << (void *)_face->get_face()->charmap << "\n";
  }
  if (_face->get_face()->charmap == NULL) {
    // If for some reason FreeType didn't set us up a charmap,
    // then set it up ourselves.
    if (_face->get_face()->num_charmaps == 0) {
      pnmtext_cat.warning()
        << name << " has no charmaps available.\n";
    } else {
      pnmtext_cat.warning()
        << name << " has no default Unicode charmap.\n";
      if (_face->get_face()->num_charmaps > 1) {
        pnmtext_cat.warning()
          << "Arbitrarily choosing first of " 
          << _face->get_face()->num_charmaps << " charmaps.\n";
      }
      FT_Set_Charmap(_face->get_face(), _face->get_face()->charmaps[0]);
    }
  }

  return true;
}
 
////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::reset_scale
//       Access: Private
//  Description: Resets the font based on the current values for
//               _point_size, _tex_pixels_per_unit, and _scale_factor.
//               Returns true if successful, false otherwise.
////////////////////////////////////////////////////////////////////
bool FreetypeFont::
reset_scale() {
  // The font may be rendered larger (by a factor of _scale_factor),
  // and then reduced into the texture.  Hence the difference between
  // _font_pixels_per_unit and _tex_pixels_per_unit.
  _tex_pixels_per_unit = _requested_pixels_per_unit;
  _scale_factor = _requested_scale_factor;
  _font_pixels_per_unit = _tex_pixels_per_unit * _scale_factor;

  float units_per_inch = (_points_per_inch / _points_per_unit);
  int dpi = (int)(_font_pixels_per_unit * units_per_inch);
  
  _font_pixel_size = 0;
  int error = FT_Set_Char_Size(_face->get_face(),
                               (int)(_point_size * 64), (int)(_point_size * 64),
                               dpi, dpi);
  if (error) {
    // If we were unable to set a particular char size, perhaps we
    // have a non-scalable font.  Try to figure out the next larger
    // available size, or the largest size available if nothing is
    // larger.
    int desired_height = (int)(_font_pixels_per_unit * _point_size / _points_per_unit + 0.5f);
    int best_size = -1;
    int largest_size = -1;
    if (_face->get_face()->num_fixed_sizes > 0) {
      largest_size = 0;
      int best_diff = 0;
      for (int i = 0; i < _face->get_face()->num_fixed_sizes; i++) {
        int diff = _face->get_face()->available_sizes[i].height - desired_height;
        if (diff > 0 && (best_size == -1 || diff < best_diff)) {
          best_size = i;
          best_diff = diff;
        }
        if (_face->get_face()->available_sizes[i].height > _face->get_face()->available_sizes[largest_size].height) {
          largest_size = i;
        }
      }
    }
    if (best_size < 0) {
      best_size = largest_size;
    }

    if (best_size >= 0) {
      int pixel_height = _face->get_face()->available_sizes[best_size].height;
      int pixel_width = _face->get_face()->available_sizes[best_size].width;
      error = FT_Set_Pixel_Sizes(_face->get_face(), pixel_width, pixel_height);
      if (!error) {
        _font_pixels_per_unit = pixel_height * _points_per_unit / _point_size;
        _scale_factor = _font_pixels_per_unit / _tex_pixels_per_unit;
        _font_pixel_size = pixel_height;

        if (_scale_factor < 1.0) {
          // No point in enlarging a fixed-point font.
          _scale_factor = 1.0;
          _tex_pixels_per_unit = _font_pixels_per_unit;
        }
      }
    }
  }

  if (error) {
    pnmtext_cat.warning()
      << "Unable to set " << get_name() 
      << " to " << _point_size << "pt at " << dpi << " dpi.\n";
    _line_height = 1.0f;
    return false;
  }

  _line_height = _face->get_face()->size->metrics.height / (_font_pixels_per_unit * 64.0f);

  // Determine the correct width for a space.
  error = FT_Load_Char(_face->get_face(), ' ', FT_LOAD_DEFAULT);
  if (error) {
    // Space isn't defined.  Oh well.
    _space_advance = 0.25f * _line_height;

  } else {
    _space_advance = _face->get_face()->glyph->advance.x / (_font_pixels_per_unit * 64.0f);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FreetypeFont::initialize_ft_library
//       Access: Private, Static
//  Description: Should be called exactly once to initialize the
//               FreeType library.
////////////////////////////////////////////////////////////////////
void FreetypeFont::
initialize_ft_library() {
  if (!_ft_initialized) {
    int error = FT_Init_FreeType(&_ft_library);
    _ft_initialized = true;
    if (error) {
      pnmtext_cat.error()
        << "Unable to initialize FreeType; dynamic fonts will not load.\n";
    } else {
      _ft_ok = true;
    }
  }
}

#endif  // HAVE_FREETYPE
