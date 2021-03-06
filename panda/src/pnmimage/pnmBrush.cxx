// Filename: pnmBrush.cxx
// Created by:  drose (01Feb07)
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

#include "pnmBrush.h"
#include "config_pnmimage.h"
#include "cmath.h"

// A PNMTransparentBrush doesn't draw or fill anything.
class EXPCL_PANDA_PNMIMAGE PNMTransparentBrush : public PNMBrush {
public:
  PNMTransparentBrush() : 
    PNMBrush(0.0, 0.0) { }

  virtual void draw(PNMImage &, int, int, double) {
  }

  virtual void fill(PNMImage &, int, int, int, int, int) {
  }
};

// A PNMPixelBrush is a family of brushes that draw one pixel at a time.
class EXPCL_PANDA_PNMIMAGE PNMPixelBrush : public PNMBrush {
protected:
  PNMPixelBrush(const Colord &color) : 
    PNMBrush(0.5, 0.5), _rgb(color[0], color[1], color[2]), _a(color[3]) { }

  RGBColord _rgb;
  double _a;
};

// Arbitrarily sets the pixel to a particular color, with no antialiasing.
class EXPCL_PANDA_PNMIMAGE PNMSetPixelBrush : public PNMPixelBrush {
public:
  PNMSetPixelBrush(const Colord &color) : PNMPixelBrush(color) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    if (x >= 0 && x < image.get_x_size() && 
        y >= 0 && y < image.get_y_size() &&
        pixel_scale >= 0.5) {
      image.set_xel(x, y, _rgb);
      if (image.has_alpha()) {
        image.set_alpha(x, y, _a);
      }
    }
  }

  virtual void fill(PNMImage &image, int xfrom, int xto, int y,
                    int xo, int yo) {
    if (y >= 0 && y < image.get_y_size()) {
      xfrom = max(xfrom, 0);
      xto = min(xto, image.get_x_size() - 1);
      for (int x = xfrom; x <= xto; ++x) {
        image.set_xel(x, y, _rgb);
      }
      if (image.has_alpha()) {
        for (int x = xfrom; x <= xto; ++x) {
          image.set_alpha(x, y, _a);
        }
      }
    }
  }
};

// Blends the pixel in to the existing background.
class EXPCL_PANDA_PNMIMAGE PNMBlendPixelBrush : public PNMPixelBrush {
public:
  PNMBlendPixelBrush(const Colord &color) : PNMPixelBrush(color) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    if (x >= 0 && x < image.get_x_size() && 
        y >= 0 && y < image.get_y_size()) {
      image.blend(x, y, _rgb, _a * pixel_scale);
    }
  }

  virtual void fill(PNMImage &image, int xfrom, int xto, int y,
                    int xo, int yo) {
    if (y >= 0 && y < image.get_y_size()) {
      xfrom = max(xfrom, 0);
      xto = min(xto, image.get_x_size() - 1);
      for (int x = xfrom; x <= xto; ++x) {
        image.blend(x, y, _rgb, _a);
      }
    }
  }
};

// Darkens the pixel in the existing background.
class EXPCL_PANDA_PNMIMAGE PNMDarkenPixelBrush : public PNMPixelBrush {
public:
  PNMDarkenPixelBrush(const Colord &color) : PNMPixelBrush(color) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    if (x >= 0 && x < image.get_x_size() && 
        y >= 0 && y < image.get_y_size()) {
      RGBColord rgb = image.get_xel(x, y);
      RGBColord p;
      p.set(min(1.0 - (1.0 - _rgb[0]) * pixel_scale, rgb[0]), 
            min(1.0 - (1.0 - _rgb[1]) * pixel_scale, rgb[1]), 
            min(1.0 - (1.0 - _rgb[2]) * pixel_scale, rgb[2]));
      image.set_xel(x, y, p);

      if (image.has_alpha()) {
        double a = image.get_alpha(x, y);
        image.set_alpha(x, y, min(1.0 - (1.0 - _a) * pixel_scale, a));
      }
    }
  }

  virtual void fill(PNMImage &image, int xfrom, int xto, int y,
                    int xo, int yo) {
    if (y >= 0 && y < image.get_y_size()) {
      xfrom = max(xfrom, 0);
      xto = min(xto, image.get_x_size() - 1);
      for (int x = xfrom; x <= xto; ++x) {
        RGBColord rgb = image.get_xel(x, y);
        RGBColord p;
        p.set(min(_rgb[0], rgb[0]), 
              min(_rgb[1], rgb[1]), 
              min(_rgb[2], rgb[2]));
        image.set_xel(x, y, p);
      }
      if (image.has_alpha()) {
        for (int x = xfrom; x <= xto; ++x) {
          double a = image.get_alpha(x, y);
          image.set_alpha(x, y, min(_a, a));
        }
      }
    }
  }
};

// Lightens the pixel in the existing background.
class EXPCL_PANDA_PNMIMAGE PNMLightenPixelBrush : public PNMPixelBrush {
public:
  PNMLightenPixelBrush(const Colord &color) : PNMPixelBrush(color) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    if (x >= 0 && x < image.get_x_size() && 
        y >= 0 && y < image.get_y_size()) {
      RGBColord rgb = image.get_xel(x, y);
      RGBColord p;
      p.set(max(_rgb[0] * pixel_scale, rgb[0]), 
            max(_rgb[1] * pixel_scale, rgb[1]), 
            max(_rgb[2] * pixel_scale, rgb[2]));
      image.set_xel(x, y, p);

      if (image.has_alpha()) {
        double a = image.get_alpha(x, y);
        image.set_alpha(x, y, max(_a * pixel_scale, a));
      }
    }
  }

  virtual void fill(PNMImage &image, int xfrom, int xto, int y,
                    int xo, int yo) {
    if (y >= 0 && y < image.get_y_size()) {
      xfrom = max(xfrom, 0);
      xto = max(xto, image.get_x_size() - 1);
      for (int x = xfrom; x <= xto; ++x) {
        RGBColord rgb = image.get_xel(x, y);
        RGBColord p;
        p.set(max(_rgb[0], rgb[0]), 
              max(_rgb[1], rgb[1]), 
              max(_rgb[2], rgb[2]));
        image.set_xel(x, y, p);
      }
      if (image.has_alpha()) {
        for (int x = xfrom; x <= xto; ++x) {
          double a = image.get_alpha(x, y);
          image.set_alpha(x, y, max(_a, a));
        }
      }
    }
  }
};

// A PNMImageBrush is a family of brushes that draw an image at a time.
class EXPCL_PANDA_PNMIMAGE PNMImageBrush : public PNMBrush {
protected:
  PNMImageBrush(const PNMImage &image, double xc, double yc) : 
    PNMBrush(xc, yc),
    _image(image) 
  {
  }

  virtual void fill(PNMImage &image, int xfrom, int xto, int y,
                    int xo, int yo) {
    if (y >= 0 && y < image.get_y_size()) {
      xfrom = max(xfrom, 0);
      xto = min(xto, image.get_x_size() - 1);

      int x_pat = (xfrom + xo) % _image.get_x_size();
      int y_pat = (y + yo) % _image.get_y_size();

      // Copy the first (right half) of the image scanline.
      int x = xfrom;
      do_scanline(image, x, y, x_pat, y_pat, xto - x + 1, 1);
      // Now repeatedly make more copies of the image scanline.
      x += _image.get_x_size() - x_pat;
      while (x <= xto) {
        do_scanline(image, x, y, 0, y_pat, xto - x + 1, 1);
        x += _image.get_x_size();
      }
    }
  }

  virtual void do_scanline(PNMImage &image, int xto, int yto,
                           int xfrom, int yfrom, int x_size, int y_size)=0;

  PNMImage _image;
};

// Sets the pixels from the rectangular image, with no antialiasing.
class EXPCL_PANDA_PNMIMAGE PNMSetImageBrush : public PNMImageBrush {
public:
  PNMSetImageBrush(const PNMImage &image, double xc, double yc) : 
    PNMImageBrush(image, xc, yc) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    if (pixel_scale >= 0.5) {
      image.copy_sub_image(_image, x, y);
    }
  }

  virtual void do_scanline(PNMImage &image, int xto, int yto, 
                           int xfrom, int yfrom, int x_size, int y_size) {
    image.copy_sub_image(_image, xto, yto, xfrom, yfrom, x_size, y_size);
  }
};

// Blends the pixels in using alpha.
class EXPCL_PANDA_PNMIMAGE PNMBlendImageBrush : public PNMImageBrush {
public:
  PNMBlendImageBrush(const PNMImage &image, double xc, double yc) : 
    PNMImageBrush(image, xc, yc) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    image.blend_sub_image(_image, x, y, 0, 0, -1, -1, pixel_scale);
  }

  virtual void do_scanline(PNMImage &image, int xto, int yto, 
                           int xfrom, int yfrom, int x_size, int y_size) {
    image.blend_sub_image(_image, xto, yto, xfrom, yfrom, x_size, y_size);
  }
};

// Darkens the pixels
class EXPCL_PANDA_PNMIMAGE PNMDarkenImageBrush : public PNMImageBrush {
public:
  PNMDarkenImageBrush(const PNMImage &image, double xc, double yc) : 
    PNMImageBrush(image, xc, yc) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    image.darken_sub_image(_image, x, y, 0, 0, -1, -1, pixel_scale);
  }

  virtual void do_scanline(PNMImage &image, int xto, int yto, 
                           int xfrom, int yfrom, int x_size, int y_size) {
    image.darken_sub_image(_image, xto, yto, xfrom, yfrom, x_size, y_size);
  }
};

// Lightens the pixels
class EXPCL_PANDA_PNMIMAGE PNMLightenImageBrush : public PNMImageBrush {
public:
  PNMLightenImageBrush(const PNMImage &image, double xc, double yc) : 
    PNMImageBrush(image, xc, yc) { }

  virtual void draw(PNMImage &image, int x, int y, double pixel_scale) {
    image.lighten_sub_image(_image, x, y, 0, 0, -1, -1, pixel_scale);
  }

  virtual void do_scanline(PNMImage &image, int xto, int yto, 
                           int xfrom, int yfrom, int x_size, int y_size) {
    image.lighten_sub_image(_image, xto, yto, xfrom, yfrom, x_size, y_size);
  }
};

////////////////////////////////////////////////////////////////////
//     Function: PNMBrush::Destructor
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PNMBrush::
~PNMBrush() {
}

////////////////////////////////////////////////////////////////////
//     Function: PNMBrush::make_transparent
//       Access: Published, Static
//  Description: Returns a new brush that does not paint anything.
//               Can be used as either a pen or a fill brush to make
//               borderless or unfilled shapes, respectively.
////////////////////////////////////////////////////////////////////
PT(PNMBrush) PNMBrush::
make_transparent() {
  return new PNMTransparentBrush();
}
  
////////////////////////////////////////////////////////////////////
//     Function: PNMBrush::make_pixel
//       Access: Published, Static
//  Description: Returns a new brush that paints a single pixel of the
//               indicated color on a border, or paints a solid color
//               in an interior.
////////////////////////////////////////////////////////////////////
PT(PNMBrush) PNMBrush::
make_pixel(const Colord &color, PNMBrush::BrushEffect effect) {
  switch (effect) {
  case BE_set:
    return new PNMSetPixelBrush(color);

  case BE_blend:
    return new PNMBlendPixelBrush(color);

  case BE_darken:
    return new PNMDarkenPixelBrush(color);

  case BE_lighten:
    return new PNMLightenPixelBrush(color);
  }

  pnmimage_cat.error()
    << "**Invalid BrushEffect (" << (int)effect << ")**\n";
  return new PNMSetPixelBrush(color);
}

////////////////////////////////////////////////////////////////////
//     Function: PNMBrush::make_spot
//       Access: Published, Static
//  Description: Returns a new brush that paints a spot of the
//               indicated color and radius.  If fuzzy is true, the
//               spot is fuzzy; otherwise, it is hard-edged.
////////////////////////////////////////////////////////////////////
PT(PNMBrush) PNMBrush::
make_spot(const Colord &color, double radius, bool fuzzy,
          BrushEffect effect) {
  Colord bg;

  switch (effect) {
  case BE_set:
    bg.set(0, 0, 0, 0);
    break;

  case BE_blend:
    bg.set(color[0], color[1], color[2], 0.0);
    break;

  case BE_darken:
    bg.set(1, 1, 1, 1);
    break;

  case BE_lighten:
    bg.set(0, 0, 0, 0);
    break;

  default:
    pnmimage_cat.error()
      << "**Invalid BrushEffect (" << (int)effect << ")**\n";
  }

  int size = (int)cceil(radius * 2.0);
  double half_size = (double)size * 0.5;
  PNMImage spot(size, size, 4);
  double r = half_size / radius;

  if (fuzzy) {
    spot.render_spot(color, bg, 0.0, r);
  } else {
    spot.render_spot(color, bg, r, r);
  }
  return make_image(spot, half_size, half_size, effect);
}
  
////////////////////////////////////////////////////////////////////
//     Function: PNMBrush::make_image
//       Access: Published, Static
//  Description: Returns a new brush that paints with the indicated
//               image.  xc and yc indicate the pixel in the center of
//               the brush.
//
//               The brush makes a copy of the image; it is safe to
//               deallocate or modify the image after making this
//               call.
////////////////////////////////////////////////////////////////////
PT(PNMBrush) PNMBrush::
make_image(const PNMImage &image, double xc, double yc,
           PNMBrush::BrushEffect effect) {
  switch (effect) {
  case BE_set:
    return new PNMSetImageBrush(image, xc, yc);

  case BE_blend:
    return new PNMBlendImageBrush(image, xc, yc);

  case BE_darken:
    return new PNMDarkenImageBrush(image, xc, yc);

  case BE_lighten:
    return new PNMLightenImageBrush(image, xc, yc);
  }

  pnmimage_cat.error()
    << "**Invalid BrushEffect (" << (int)effect << ")**\n";
  return new PNMSetImageBrush(image, xc, yc);
}
