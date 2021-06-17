// Filename: dynamicTextGlyph.cxx
// Created by:  drose (09Feb02)
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

#include "dynamicTextGlyph.h"

#ifdef HAVE_FREETYPE

#include "dynamicTextPage.h"
#include "geomTextGlyph.h"
#include "geomVertexData.h"
#include "geomVertexFormat.h"
#include "geomTriangles.h"
#include "geomVertexWriter.h"
#include "textureAttrib.h"
#include "transparencyAttrib.h"
#include "colorAttrib.h"
#include "renderState.h"
#include "config_gobj.h"

////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
DynamicTextGlyph::
~DynamicTextGlyph() {
}

////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::get_row
//       Access: Public
//  Description: Returns a pointer to the first byte in the pixel
//               buffer associated with the leftmost pixel in the
//               indicated row, where 0 is the topmost row and _y_size
//               - _margin * 2 - 1 is the bottommost row.
////////////////////////////////////////////////////////////////////
unsigned char *DynamicTextGlyph::
get_row(int y) {
  nassertr(y >= 0 && y < _y_size - _margin * 2, (unsigned char *)NULL);
  nassertr(_page != (DynamicTextPage *)NULL, (unsigned char *)NULL);

  // First, offset y by the glyph's start.
  y += _y + _margin;
  // Also, get the x start.
  int x = _x + _margin;

  // Invert y.
  y = _page->get_y_size() - 1 - y;

  int offset = (y * _page->get_x_size()) + x;
  int pixel_width = _page->get_num_components() * _page->get_component_width();

  return _page->modify_ram_image() + offset * pixel_width; 
}

////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::erase
//       Access: Public
//  Description: Erases the glyph from the texture map.
////////////////////////////////////////////////////////////////////
void DynamicTextGlyph::
erase(DynamicTextFont *font) {
  nassertv(_page != (DynamicTextPage *)NULL);
  nassertv(_page->has_ram_image());

  // The glyph covers the pixels from (_x, _y) over the rectangle
  // (_x_size, _y_size), but it doesn't include _margin pixels around
  // the interior of the rectangle.  Erase all the pixels that the
  // glyph covers.
  _page->fill_region(_x + _margin, 
                     _page->get_y_size() - (_y + _y_size - _margin),
                     _x_size - _margin * 2, _y_size - _margin * 2,
                     font->get_bg());
}

////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::make_geom
//       Access: Public
//  Description: Creates the actual geometry for the glyph.  The
//               parameters bitmap_top and bitmap_left are from
//               FreeType, and indicate the position of the top left
//               corner of the bitmap relative to the glyph's origin.
//               The advance number represents the number of pixels
//               the pen should be advanced after drawing this glyph.
////////////////////////////////////////////////////////////////////
void DynamicTextGlyph::
make_geom(int bitmap_top, int bitmap_left, float advance, float poly_margin, 
          float tex_x_size, float tex_y_size,
          float font_pixels_per_unit, float tex_pixels_per_unit) {
  nassertv(_page != (DynamicTextPage *)NULL);

  // This function should not be called twice.
  nassertv(_geom_count == 0);

  tex_x_size += _margin * 2;
  tex_y_size += _margin * 2;

  // Determine the corners of the rectangle in geometric units.
  float tex_poly_margin = poly_margin / tex_pixels_per_unit;
  float origin_y = bitmap_top / font_pixels_per_unit;
  float origin_x = bitmap_left / font_pixels_per_unit;
  float top = origin_y + tex_poly_margin;
  float left = origin_x - tex_poly_margin;
  float bottom = origin_y - tex_y_size / tex_pixels_per_unit - tex_poly_margin;
  float right = origin_x + tex_x_size / tex_pixels_per_unit + tex_poly_margin;

  // And the corresponding corners in UV units.  We add 0.5f to center
  // the UV in the middle of its texel, to minimize roundoff errors
  // when we are close to 1-to-1 pixel size.
  float uv_top = 1.0f - ((float)(_y - poly_margin) + 0.5f) / _page->get_y_size();
  float uv_left = ((float)(_x - poly_margin) + 0.5f) / _page->get_x_size();
  float uv_bottom = 1.0f - ((float)(_y + poly_margin + tex_y_size) + 0.5f) / _page->get_y_size();
  float uv_right = ((float)(_x + poly_margin + tex_x_size) + 0.5f) / _page->get_x_size();
  // Create a corresponding triangle pair.  We use a pair of indexed
  // triangles rather than a single triangle strip, to avoid the bad
  // vertex duplication behavior with lots of two-triangle strips.
  PT(GeomVertexData) vdata = new GeomVertexData
    (string(), GeomVertexFormat::get_v3t2(),
     Geom::UH_static);
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  GeomVertexWriter texcoord(vdata, InternalName::get_texcoord());
  
  vertex.add_data3f(left, 0, top);
  vertex.add_data3f(left, 0, bottom);
  vertex.add_data3f(right, 0, top);
  vertex.add_data3f(right, 0, bottom);
  
  texcoord.add_data2f(uv_left, uv_top);
  texcoord.add_data2f(uv_left, uv_bottom);
  texcoord.add_data2f(uv_right, uv_top);
  texcoord.add_data2f(uv_right, uv_bottom);
  
  PT(GeomTriangles) tris = new GeomTriangles(Geom::UH_static);
  tris->add_vertex(0);
  tris->add_vertex(1);
  tris->add_vertex(2);
  tris->close_primitive();
  tris->add_vertex(2);
  tris->add_vertex(1);
  tris->add_vertex(3);
  tris->close_primitive();

  PT(Geom) geom = new GeomTextGlyph(this, vdata);
  geom->add_primitive(tris);
  _geom = geom;
  
  // The above will increment our _geom_count to 1.  Reset it back
  // down to 0, since our own internal Geom doesn't count.
  nassertv(_geom_count == 1);
  _geom_count--;
  
  _state = RenderState::make(TextureAttrib::make(_page),
                             TransparencyAttrib::make(TransparencyAttrib::M_alpha));
  _state = _state->add_attrib(ColorAttrib::make_flat(Colorf(1.0f, 1.0f, 1.0f, 1.0f)), -1);
  
  _advance = advance / font_pixels_per_unit;
}


////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::set_geom
//       Access: Public
//  Description: Sets the geom from a pre-built object.
////////////////////////////////////////////////////////////////////
void DynamicTextGlyph::
set_geom(GeomVertexData *vdata, GeomPrimitive *prim, 
         const RenderState *state) {
  // This function is called when _geom_count = 1, because it was
  // constructed via the empty Glyph constructor.
  nassertv(_geom_count == 1);
  _geom_count--;

  PT(Geom) geom = new GeomTextGlyph(this, vdata);
  geom->add_primitive(prim);
  _geom = geom;
  
  // The above will increment our _geom_count to 1.  Reset it back
  // down to 0, since our own internal Geom doesn't count.
  nassertv(_geom_count == 1);
  _geom_count--;
  
  _state = state;
}

////////////////////////////////////////////////////////////////////
//     Function: DynamicTextGlyph::is_whitespace
//       Access: Public, Virtual
//  Description: Returns true if this glyph represents invisible
//               whitespace, or false if it corresponds to some
//               visible character.
////////////////////////////////////////////////////////////////////
bool DynamicTextGlyph::
is_whitespace() const {
  return (_page == (DynamicTextPage *)NULL);
}


#endif  // HAVE_FREETYPE
