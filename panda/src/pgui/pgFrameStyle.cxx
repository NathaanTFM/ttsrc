// Filename: pgFrameStyle.cxx
// Created by:  drose (03Jul01)
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

#include "pgFrameStyle.h"
#include "geomNode.h"
#include "pandaNode.h"
#include "transparencyAttrib.h"
#include "pointerTo.h"
#include "nodePath.h"
#include "textureAttrib.h"
#include "renderState.h"
#include "shadeModelAttrib.h"
#include "colorAttrib.h"
#include "geom.h"
#include "geomTristrips.h"
#include "geomVertexWriter.h"

// Specifies the UV range of textures applied to the frame.  Maybe
// we'll have a reason to make this a parameter of the frame style one
// day, but for now it's hardcoded to fit the entire texture over the
// rectangular frame.
static const LVecBase4f uv_range = LVecBase4f(0.0f, 1.0f, 0.0f, 1.0f);

ostream &
operator << (ostream &out, PGFrameStyle::Type type) {
  switch (type) {
  case PGFrameStyle::T_none:
    return out << "none";

  case PGFrameStyle::T_flat:
    return out << "flat";

  case PGFrameStyle::T_bevel_out:
    return out << "bevel_out";

  case PGFrameStyle::T_bevel_in:
    return out << "bevel_in";

  case PGFrameStyle::T_groove:
    return out << "groove";

  case PGFrameStyle::T_ridge:
    return out << "ridge";

  case PGFrameStyle::T_texture_border:
    return out << "texture_border";
  }

  return out << "**unknown(" << (int)type << ")**";
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::get_internal_frame
//       Access: Published
//  Description: Computes the size of the internal frame, given the
//               indicated external frame, appropriate for this kind
//               of frame style.  This simply subtracts the border
//               width for those frame styles that include a border.
////////////////////////////////////////////////////////////////////
LVecBase4f PGFrameStyle::
get_internal_frame(const LVecBase4f &frame) const {
  LPoint2f center((frame[0] + frame[1]) / 2.0f,
                  (frame[2] + frame[3]) / 2.0f);
  LVecBase4f scaled_frame
    ((frame[0] - center[0]) * _visible_scale[0] + center[0],
     (frame[1] - center[0]) * _visible_scale[0] + center[0],
     (frame[2] - center[1]) * _visible_scale[1] + center[1],
     (frame[3] - center[1]) * _visible_scale[1] + center[1]);

  switch (_type) {
  case T_none:
  case T_flat:
    return scaled_frame;

  default:
    break;
  }

  return LVecBase4f(scaled_frame[0] + _width[0],
                    scaled_frame[1] - _width[0],
                    scaled_frame[2] + _width[1],
                    scaled_frame[3] - _width[1]);
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::output
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void PGFrameStyle::
output(ostream &out) const {
  out << _type << " color = " << _color << " width = " << _width;
  if (_visible_scale != LVecBase2f(1.0f, 1.0f)) {
    out << "visible_scale = " << get_visible_scale();
  }
  if (has_texture()) {
    out << " texture = " << *get_texture();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::xform
//       Access: Public
//  Description: Applies the indicated transform to the FrameStyle.
//               The return value is true if the frame style is
//               transformed, or false if it was not affected by the
//               transform.
////////////////////////////////////////////////////////////////////
bool PGFrameStyle::
xform(const LMatrix4f &mat) {
  // All we can do is scale the X and Y bevel sizes.

  // Extract the X and Z axes from the matrix.
  LVector3f x, z;
  mat.get_row3(x, 0);
  float x_scale = x.length();
  
  mat.get_row3(z, 2);
  float z_scale = z.length();

  _width[0] *= x_scale;
  _width[1] *= z_scale;

  switch (_type) {
  case T_none:
  case T_flat:
    return false;

  case T_bevel_out:
  case T_bevel_in:
  case T_groove:
  case T_ridge:
  case T_texture_border:
    return true;
  }

  // Shouldn't get here, but this makes the compiler happy.
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::generate_into
//       Access: Public
//  Description: Generates geometry representing a frame of the
//               indicated size, and parents it to the indicated node,
//               with the indicated scene graph sort order.
//
//               The return value is the generated NodePath, if any,
//               or an empty NodePath if nothing is generated.
////////////////////////////////////////////////////////////////////
NodePath PGFrameStyle::
generate_into(const NodePath &parent, const LVecBase4f &frame,
              int sort) {
  PT(PandaNode) new_node;

  LPoint2f center((frame[0] + frame[1]) / 2.0f,
                  (frame[2] + frame[3]) / 2.0f);
  LVecBase4f scaled_frame
    ((frame[0] - center[0]) * _visible_scale[0] + center[0],
     (frame[1] - center[0]) * _visible_scale[0] + center[0],
     (frame[2] - center[1]) * _visible_scale[1] + center[1],
     (frame[3] - center[1]) * _visible_scale[1] + center[1]);

  switch (_type) {
  case T_none:
    return NodePath();

  case T_flat:
    new_node = generate_flat_geom(scaled_frame);
    break;

  case T_bevel_out:
    new_node = generate_bevel_geom(scaled_frame, false);
    break;

  case T_bevel_in:
    new_node = generate_bevel_geom(scaled_frame, true);
    break;

  case T_groove:
    new_node = generate_groove_geom(scaled_frame, true);
    break;

  case T_ridge:
    new_node = generate_groove_geom(scaled_frame, false);
    break;

  case T_texture_border:
    new_node = generate_texture_border_geom(scaled_frame);
    break;

  default:
    break;
  }

  if (new_node != (PandaNode *)NULL && _color[3] != 1.0f) {
    // We've got some alpha on the color; we need transparency.
    new_node->set_attrib(TransparencyAttrib::make(TransparencyAttrib::M_alpha));
  }

  // Adding the node to the parent keeps the reference count.
  return parent.attach_new_node(new_node, sort);
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::generate_flat_geom
//       Access: Private
//  Description: Generates the GeomNode appropriate to a T_flat
//               frame.
////////////////////////////////////////////////////////////////////
PT(PandaNode) PGFrameStyle::
generate_flat_geom(const LVecBase4f &frame) {
  PT(GeomNode) gnode = new GeomNode("flat");

  float left = frame[0];
  float right = frame[1];
  float bottom = frame[2];
  float top = frame[3];

  CPT(GeomVertexFormat) format;
  if (has_texture()) {
    format = GeomVertexFormat::get_v3t2();
  } else {
    format = GeomVertexFormat::get_v3();
  }
  
  PT(GeomVertexData) vdata = new GeomVertexData
    ("PGFrame", format, Geom::UH_static);
  
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  vertex.add_data3f(left, 0.0f, top);
  vertex.add_data3f(left, 0.0f, bottom);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(right, 0.0f, bottom);

  if (has_texture()) {
    // Generate UV's.
    left = uv_range[0];
    right = uv_range[1];
    bottom = uv_range[2];
    top = uv_range[3];
    
    GeomVertexWriter texcoord(vdata, InternalName::get_texcoord());
    texcoord.add_data2f(left, top);
    texcoord.add_data2f(left, bottom);
    texcoord.add_data2f(right, top);
    texcoord.add_data2f(right, bottom);
  }
  
  PT(GeomTristrips) strip = new GeomTristrips(Geom::UH_static);
  strip->add_next_vertices(4);
  strip->close_primitive();
  
  CPT(RenderState) state = RenderState::make(ColorAttrib::make_flat(_color), -1);
  if (has_texture()) {
    state = state->set_attrib(TextureAttrib::make(get_texture()));
  }
  PT(Geom) geom = new Geom(vdata);
  geom->add_primitive(strip);
  gnode->add_geom(geom, state);
  
  return gnode.p();
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::generate_bevel_geom
//       Access: Private
//  Description: Generates the GeomNode appropriate to a T_bevel_in
//               or T_bevel_out frame.
////////////////////////////////////////////////////////////////////
PT(PandaNode) PGFrameStyle::
generate_bevel_geom(const LVecBase4f &frame, bool in) {
  //
  // Colors:
  //
  // 
  //  * * * * * * * * * * * * * * * * * * * * * * *
  //  * *                                       * *
  //  *   *               ctop                *   *
  //  *     *                               *     *
  //  *       * * * * * * * * * * * * * * *       *
  //  *       *                           *       *
  //  *       *                           *       *
  //  * cleft *          _color           * cright*
  //  *       *                           *       *
  //  *       *                           *       *
  //  *       * * * * * * * * * * * * * * *       *
  //  *     *                               *     *
  //  *   *              cbottom              *   *
  //  * *                                       * *
  //  * * * * * * * * * * * * * * * * * * * * * * *
  //
  //
  // Vertices:
  //
  //  tristrip 1:
  //  4 * * * * * * * * * * * * * * * * * * * * * 6
  //  * *                                       *
  //  *   *                                   *
  //  *     *                               *
  //  *       5 * * * * * * * * * * * * * 7
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       3 * * * * * * * * * * * * * 1
  //  *     *                               *
  //  *   *                                   *
  //  * *                                       *
  //  2 * * * * * * * * * * * * * * * * * * * * * 0
  // 
  //  tristrip 2:
  //                                              1
  //                                            * *
  //                                          *   *
  //                                        *     *
  //          5 * * * * * * * * * * * * * 3       *
  //          *                           *       *
  //          *                           *       *
  //          *                           *       *
  //          *                           *       *
  //          *                           *       *
  //          4 * * * * * * * * * * * * * 2       *
  //                                        *     *
  //                                          *   *
  //                                            * *
  //                                              0

  PT(GeomNode) gnode = new GeomNode("bevel");

  float left = frame[0];
  float right = frame[1];
  float bottom = frame[2];
  float top = frame[3];

  float cx = (left + right) * 0.5;
  float cy = (top + bottom) * 0.5;

  float inner_left = min(left + _width[0], cx);
  float inner_right = max(right - _width[0], cx);
  float inner_bottom = min(bottom + _width[1], cy);
  float inner_top = max(top - _width[1], cy);

  float left_color_scale = 1.2;
  float right_color_scale = 0.8;
  float bottom_color_scale = 0.7;
  float top_color_scale = 1.3;

  if (in) {
    right_color_scale = 1.2;
    left_color_scale = 0.8;
    top_color_scale = 0.7;
    bottom_color_scale = 1.3;
  }

  // Clamp all colors at white, and don't scale the alpha.
  Colorf cleft(min(_color[0] * left_color_scale, 1.0f),
               min(_color[1] * left_color_scale, 1.0f),
               min(_color[2] * left_color_scale, 1.0f),
               _color[3]);

  Colorf cright(min(_color[0] * right_color_scale, 1.0f),
                min(_color[1] * right_color_scale, 1.0f),
                min(_color[2] * right_color_scale, 1.0f),
                _color[3]);

  Colorf cbottom(min(_color[0] * bottom_color_scale, 1.0f),
                 min(_color[1] * bottom_color_scale, 1.0f),
                 min(_color[2] * bottom_color_scale, 1.0f),
                 _color[3]);

  Colorf ctop(min(_color[0] * top_color_scale, 1.0f),
              min(_color[1] * top_color_scale, 1.0f),
              min(_color[2] * top_color_scale, 1.0f),
              _color[3]);

  CPT(GeomVertexFormat) format;
  if (has_texture()) {
    format = GeomVertexFormat::get_v3cpt2();
  } else {
    format = GeomVertexFormat::get_v3cp();
  }

  PT(GeomVertexData) vdata = new GeomVertexData
    ("PGFrame", format, Geom::UH_static);
  
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  GeomVertexWriter color(vdata, InternalName::get_color());
  
  PT(GeomTristrips) strip = new GeomTristrips(Geom::UH_static);
  // Tristrip 1.
  vertex.add_data3f(right, 0.0f, bottom);
  vertex.add_data3f(inner_right, 0.0f, inner_bottom);
  vertex.add_data3f(left, 0.0f, bottom);
  vertex.add_data3f(inner_left, 0.0f, inner_bottom);
  vertex.add_data3f(left, 0.0f, top);
  vertex.add_data3f(inner_left, 0.0f, inner_top);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(inner_right, 0.0f, inner_top);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cleft);
  color.add_data4f(cleft);
  color.add_data4f(ctop);
  color.add_data4f(ctop);
  
  strip->add_next_vertices(8);
  strip->close_primitive();
  
  // Tristrip 2.
  vertex.add_data3f(right, 0.0f, bottom);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(inner_right, 0.0f, inner_bottom);
  vertex.add_data3f(inner_right, 0.0f, inner_top);
  vertex.add_data3f(inner_left, 0.0f, inner_bottom);
  vertex.add_data3f(inner_left, 0.0f, inner_top);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(_color);
  color.add_data4f(_color);
  
  strip->add_next_vertices(6);
  strip->close_primitive();
  strip->set_shade_model(Geom::SM_flat_last_vertex);

  if (has_texture()) {
    // Generate UV's.
    float left = uv_range[0];
    float right = uv_range[1];
    float bottom = uv_range[2];
    float top = uv_range[3];

    float cx = (left + right) * 0.5;
    float cy = (top + bottom) * 0.5;
    
    float inner_left = min(left + _uv_width[0], cx);
    float inner_right = max(right - _uv_width[0], cx);
    float inner_bottom = min(bottom + _uv_width[1], cy);
    float inner_top = max(top - _uv_width[1], cy);

    GeomVertexWriter texcoord(vdata, InternalName::get_texcoord());
    texcoord.add_data2f(right, bottom);
    texcoord.add_data2f(inner_right, inner_bottom);
    texcoord.add_data2f(left, bottom);
    texcoord.add_data2f(inner_left, inner_bottom);
    texcoord.add_data2f(left, top);
    texcoord.add_data2f(inner_left, inner_top);
    texcoord.add_data2f(right, top);
    texcoord.add_data2f(inner_right, inner_top);

    texcoord.add_data2f(right, bottom);
    texcoord.add_data2f(right, top);
    texcoord.add_data2f(inner_right, inner_bottom);
    texcoord.add_data2f(inner_right, inner_top);
    texcoord.add_data2f(inner_left, inner_bottom);
    texcoord.add_data2f(inner_left, inner_top);
  }
  PT(Geom) geom = new Geom(vdata);
  geom->add_primitive(strip);
  
  CPT(RenderState) state;
  state = RenderState::make(ShadeModelAttrib::make(ShadeModelAttrib::M_flat),
                            ColorAttrib::make_vertex());
  if (has_texture()) {
    state = state->set_attrib(TextureAttrib::make(get_texture()));
  }
  gnode->add_geom(geom, state);
  
  return gnode.p();
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::generate_groove_geom
//       Access: Private
//  Description: Generates the GeomNode appropriate to a T_groove or
//               T_ridge frame.
////////////////////////////////////////////////////////////////////
PT(PandaNode) PGFrameStyle::
generate_groove_geom(const LVecBase4f &frame, bool in) {
  //
  // Colors:
  //
  // 
  //  * * * * * * * * * * * * * * * * * * * * * * * * * * *
  //  * *                                               * *
  //  *   *                   ctop                    *   *
  //  *     *                                       *     *
  //  *       * * * * * * * * * * * * * * * * * * *       *
  //  *       * *                               * *       *
  //  *       *   *          cbottom          *   *       *
  //  *       *     *                       *     *       *
  //  *       *       * * * * * * * * * * *       *       *
  //  *       *       *                   *       *       *
  //  *       *       *                   *       *       *
  //  * cleft * cright*      _color       * cleft * cright*
  //  *       *       *                   *       *       *
  //  *       *       *                   *       *       *
  //  *       *       * * * * * * * * * * *       *       *
  //  *       *     *                       *     *       *
  //  *       *   *           ctop            *   *       *
  //  *       * *                               * *       *
  //  *       * * * * * * * * * * * * * * * * * * *       *
  //  *     *                                       *     *
  //  *   *                  cbottom                  *   *
  //  * *                                               * *
  //  * * * * * * * * * * * * * * * * * * * * * * * * * * *
  //
  //
  // Vertices:
  //
  //  tristrip 1:
  //  4 * * * * * * * * * * * * * * * * * * * * * * * * * 6
  //  * *                                               *
  //  *   *                                           *
  //  *     *                                       *
  //  *       5 * * * * * * * * * * * * * * * * * 7
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       *
  //  *       3 * * * * * * * * * * * * * * * * * 1
  //  *     *                                       *
  //  *   *                                           *
  //  * *                                               *
  //  2 * * * * * * * * * * * * * * * * * * * * * * * * * 0
  //
  //  tristrip 2:
  //          4 * * * * * * * * * * * * * * * * * 6
  //          * *                               *
  //          *   *                           *
  //          *     *                       *
  //          *       5 * * * * * * * * * 7
  //          *       *
  //          *       *
  //          *       *
  //          *       *
  //          *       *
  //          *       3 * * * * * * * * * 1
  //          *     *                       *
  //          *   *                           *
  //          * *                               *
  //          2 * * * * * * * * * * * * * * * * * 0
  // 
  //  tristrip 3:
  //                                                      1
  //                                                    * *
  //                                                  *   *
  //                                                *     *
  //                                              3       *
  //                                            * *       *
  //                                          *   *       *
  //                                        *     *       *
  //                  7 * * * * * * * * * 5       *       *
  //                  *                   *       *       *
  //                  *                   *       *       *
  //                  *                   *       *       *
  //                  *                   *       *       *
  //                  *                   *       *       *
  //                  6 * * * * * * * * * 4       *       *
  //                                        *     *       *
  //                                          *   *       *
  //                                            * *       *
  //                                              2       *
  //                                                *     *
  //                                                  *   *
  //                                                    * *
  //                                                      0

  PT(GeomNode) gnode = new GeomNode("groove");

  float left = frame[0];
  float right = frame[1];
  float bottom = frame[2];
  float top = frame[3];

  float cx = (left + right) * 0.5;
  float cy = (top + bottom) * 0.5;

  float mid_left = min(left + 0.5f * _width[0], cx);
  float mid_right = max(right - 0.5f * _width[0], cx);
  float mid_bottom = min(bottom + 0.5f * _width[1], cy);
  float mid_top = max(top - 0.5f * _width[1], cy);

  float inner_left = min(left + _width[0], cx);
  float inner_right = max(right - _width[0], cx);
  float inner_bottom = min(bottom + _width[1], cy);
  float inner_top = max(top - _width[1], cy);

  float left_color_scale = 1.2f;
  float right_color_scale = 0.8f;
  float bottom_color_scale = 0.7f;
  float top_color_scale = 1.3f;

  if (in) {
    right_color_scale = 1.2f;
    left_color_scale = 0.8f;
    top_color_scale = 0.7f;
    bottom_color_scale = 1.3f;
  }

  // Clamp all colors at white, and don't scale the alpha.
  Colorf cleft(min(_color[0] * left_color_scale, 1.0f),
               min(_color[1] * left_color_scale, 1.0f),
               min(_color[2] * left_color_scale, 1.0f),
               _color[3]);

  Colorf cright(min(_color[0] * right_color_scale, 1.0f),
                min(_color[1] * right_color_scale, 1.0f),
                min(_color[2] * right_color_scale, 1.0f),
                _color[3]);

  Colorf cbottom(min(_color[0] * bottom_color_scale, 1.0f),
                 min(_color[1] * bottom_color_scale, 1.0f),
                 min(_color[2] * bottom_color_scale, 1.0f),
                 _color[3]);

  Colorf ctop(min(_color[0] * top_color_scale, 1.0f),
              min(_color[1] * top_color_scale, 1.0f),
              min(_color[2] * top_color_scale, 1.0f),
              _color[3]);

  CPT(GeomVertexFormat) format = GeomVertexFormat::get_v3cp();
  PT(GeomVertexData) vdata = new GeomVertexData
    ("PGFrame", format, Geom::UH_static);
  
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  GeomVertexWriter color(vdata, InternalName::get_color());
  
  PT(GeomTristrips) strip = new GeomTristrips(Geom::UH_static);
  // Tristrip 1.
  vertex.add_data3f(right, 0.0f, bottom);
  vertex.add_data3f(mid_right, 0.0f, mid_bottom);
  vertex.add_data3f(left, 0.0f, bottom);
  vertex.add_data3f(mid_left, 0.0f, mid_bottom);
  vertex.add_data3f(left, 0.0f, top);
  vertex.add_data3f(mid_left, 0.0f, mid_top);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(mid_right, 0.0f, mid_top);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  color.add_data4f(cleft);
  color.add_data4f(cleft);
  color.add_data4f(ctop);
  color.add_data4f(ctop);
  
  strip->add_next_vertices(8);
  strip->close_primitive();
  
  // Tristrip 2.
  vertex.add_data3f(mid_right, 0.0f, mid_bottom);
  vertex.add_data3f(inner_right, 0.0f, inner_bottom);
  vertex.add_data3f(mid_left, 0.0f, mid_bottom);
  vertex.add_data3f(inner_left, 0.0f, inner_bottom);
  vertex.add_data3f(mid_left, 0.0f, mid_top);
  vertex.add_data3f(inner_left, 0.0f, inner_top);
  vertex.add_data3f(mid_right, 0.0f, mid_top);
  vertex.add_data3f(inner_right, 0.0f, inner_top);
  color.add_data4f(ctop);
  color.add_data4f(ctop);
  color.add_data4f(ctop);
  color.add_data4f(ctop);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cbottom);
  color.add_data4f(cbottom);
  
  strip->add_next_vertices(8);
  strip->close_primitive();
  
  // Tristrip 3.
  vertex.add_data3f(right, 0.0f, bottom);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(mid_right, 0.0f, mid_bottom);
  vertex.add_data3f(mid_right, 0.0f, mid_top);
  vertex.add_data3f(inner_right, 0.0f, inner_bottom);
  vertex.add_data3f(inner_right, 0.0f, inner_top);
  vertex.add_data3f(inner_left, 0.0f, inner_bottom);
  vertex.add_data3f(inner_left, 0.0f, inner_top);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cright);
  color.add_data4f(cleft);
  color.add_data4f(cleft);
  color.add_data4f(_color);
  color.add_data4f(_color);
  
  strip->add_next_vertices(8);
  strip->close_primitive();
  
  strip->set_shade_model(Geom::SM_flat_last_vertex);
  
  PT(Geom) geom = new Geom(vdata);
  geom->add_primitive(strip);
  
  CPT(RenderState) state = RenderState::make(ShadeModelAttrib::make(ShadeModelAttrib::M_flat),
                                             ColorAttrib::make_vertex());
  gnode->add_geom(geom, state);
  
  // For now, beveled and grooved geoms don't support textures.  Easy
  // to add if anyone really wants this.
  
  return gnode.p();
}

////////////////////////////////////////////////////////////////////
//     Function: PGFrameStyle::generate_texture_border_geom
//       Access: Private
//  Description: Generates the GeomNode appropriate to a
//               T_texture_border frame.
////////////////////////////////////////////////////////////////////
PT(PandaNode) PGFrameStyle::
generate_texture_border_geom(const LVecBase4f &frame) {
  //
  // Vertices:
  //
  //  tristrip 1:
  //  0 * * * 2 * * * * * * * * * * * * * 4 * * * 6
  //  *     * *                     * * * *     * *
  //  *   *   *         * * * * * *       *   *   *
  //  * *     * * * * *                   * *     *
  //  1 * * * 3 * * * * * * * * * * * * * 5 * * * 7
  //
  //  tristrip 2:
  //  1 * * * 3 * * * * * * * * * * * * * 5 * * * 7
  //  *       *                         * *       *
  //  *     * *                     * *   *     * *
  //  *   *   *               * * *       *   *   *
  //  *   *   *         * * *             *   *   *
  //  * *     *     * *                   * *     *
  //  *       * * *                       *       *
  //  8 * * *10 * * * * * * * * * * * * *12 * * *14
  // 
  //  tristrip 3:
  //  8 * * *10 * * * * * * * * * * * * *12 * * *14
  //  *     * *                     * * * *     * *
  //  *   *   *         * * * * * *       *   *   *
  //  * *     * * * * *                   * *     *
  //  9 * * *11 * * * * * * * * * * * * *13 * * *15

  PT(GeomNode) gnode = new GeomNode("flat");

  float left = frame[0];
  float right = frame[1];
  float bottom = frame[2];
  float top = frame[3];

  float cx = (left + right) * 0.5;
  float cy = (top + bottom) * 0.5;

  float inner_left = min(left + _width[0], cx);
  float inner_right = max(right - _width[0], cx);
  float inner_bottom = min(bottom + _width[1], cy);
  float inner_top = max(top - _width[1], cy);

  CPT(GeomVertexFormat) format;
  if (has_texture()) {
    format = GeomVertexFormat::get_v3t2();
  } else {
    format = GeomVertexFormat::get_v3();
  }
  
  PT(GeomVertexData) vdata = new GeomVertexData
    ("PGFrame", format, Geom::UH_static);
  
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  
  // verts 0,1,2,3
  vertex.add_data3f(left, 0.0f, top);
  vertex.add_data3f(left, 0.0f, inner_top);
  vertex.add_data3f(inner_left, 0.0f, top);
  vertex.add_data3f(inner_left, 0.0f, inner_top);
  // verts 4,5,6,7
  vertex.add_data3f(inner_right, 0.0f, top);
  vertex.add_data3f(inner_right, 0.0f, inner_top);
  vertex.add_data3f(right, 0.0f, top);
  vertex.add_data3f(right, 0.0f, inner_top);
  // verts 8,9,10,11
  vertex.add_data3f(left, 0.0f, inner_bottom);
  vertex.add_data3f(left, 0.0f, bottom);
  vertex.add_data3f(inner_left, 0.0f, inner_bottom);
  vertex.add_data3f(inner_left, 0.0f, bottom);
  // verts 12,13,14,15
  vertex.add_data3f(inner_right, 0.0f, inner_bottom);
  vertex.add_data3f(inner_right, 0.0f, bottom);
  vertex.add_data3f(right, 0.0f, inner_bottom);
  vertex.add_data3f(right, 0.0f, bottom);

  if (has_texture()) {
    // Generate UV's.
    float left = uv_range[0];
    float right = uv_range[1];
    float bottom = uv_range[2];
    float top = uv_range[3];

    float cx = (left + right) * 0.5;
    float cy = (top + bottom) * 0.5;
    
    float inner_left = min(left + _uv_width[0], cx);
    float inner_right = max(right - _uv_width[0], cx);
    float inner_bottom = min(bottom + _uv_width[1], cy);
    float inner_top = max(top - _uv_width[1], cy);

    GeomVertexWriter texcoord(vdata, InternalName::get_texcoord());
  
    // verts 0,1,2,3
    texcoord.add_data2f(left, top);
    texcoord.add_data2f(left, inner_top);
    texcoord.add_data2f(inner_left, top);
    texcoord.add_data2f(inner_left, inner_top);
    // verts 4,5,6,7
    texcoord.add_data2f(inner_right, top);
    texcoord.add_data2f(inner_right, inner_top);
    texcoord.add_data2f(right, top);
    texcoord.add_data2f(right, inner_top);
    // verts 8,9,10,11
    texcoord.add_data2f(left, inner_bottom);
    texcoord.add_data2f(left, bottom);
    texcoord.add_data2f(inner_left, inner_bottom);
    texcoord.add_data2f(inner_left, bottom);
    // verts 12,13,14,15
    texcoord.add_data2f(inner_right, inner_bottom);
    texcoord.add_data2f(inner_right, bottom);
    texcoord.add_data2f(right, inner_bottom);
    texcoord.add_data2f(right, bottom);
  }
  
  PT(GeomTristrips) strip = new GeomTristrips(Geom::UH_static);
  
  // tristrip #1
  strip->add_consecutive_vertices(0, 8);
  strip->close_primitive();
  
  // tristrip #2
  strip->add_vertex(1);
  strip->add_vertex(8);
  strip->add_vertex(3);
  strip->add_vertex(10);
  strip->add_vertex(5);
  strip->add_vertex(12);
  strip->add_vertex(7);
  strip->add_vertex(14);
  strip->close_primitive();
  
  // tristrip #3
  strip->add_consecutive_vertices(8, 8);
  strip->close_primitive();
  
  CPT(RenderState) state = RenderState::make(ColorAttrib::make_flat(_color), -1);
  if (has_texture()) {
    state = state->set_attrib(TextureAttrib::make(get_texture()));
  }

  PT(Geom) geom = new Geom(vdata);
  geom->add_primitive(strip);
  gnode->add_geom(geom, state);
  
  return gnode.p();
}
