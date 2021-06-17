// Filename: cullableObject.cxx
// Created by:  drose (04Mar02)
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

#include "cullableObject.h"
#include "lightAttrib.h"
#include "nodePath.h"
#include "texGenAttrib.h"
#include "renderState.h"
#include "clockObject.h"
#include "cullTraverser.h"
#include "sceneSetup.h"
#include "lens.h"
#include "stateMunger.h"
#include "pStatTimer.h"
#include "geomVertexWriter.h"
#include "geomVertexReader.h"
#include "geomTriangles.h"
#include "light.h"
#include "lightMutexHolder.h"
#include "geomDrawCallbackData.h"

CullableObject::FormatMap CullableObject::_format_map;
LightMutex CullableObject::_format_lock;

PStatCollector CullableObject::_munge_geom_pcollector("*:Munge:Geom");
PStatCollector CullableObject::_munge_sprites_pcollector("*:Munge:Sprites");
PStatCollector CullableObject::_munge_sprites_verts_pcollector("*:Munge:Sprites:Verts");
PStatCollector CullableObject::_munge_sprites_prims_pcollector("*:Munge:Sprites:Prims");
PStatCollector CullableObject::_munge_light_vector_pcollector("*:Munge:Light Vector");
PStatCollector CullableObject::_sw_sprites_pcollector("SW Sprites");

TypeHandle CullableObject::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::munge_geom
//       Access: Public
//  Description: Uses the indicated GeomMunger to transform the geom
//               and/or its vertices.
//
//               If force is false, this may do nothing and return
//               false if the vertex data is nonresident.  If force is
//               true, this will always return true, but it may have
//               to block while the vertex data is paged in.
////////////////////////////////////////////////////////////////////
bool CullableObject::
munge_geom(GraphicsStateGuardianBase *gsg,
           GeomMunger *munger, const CullTraverser *traverser,
           bool force) {
  nassertr(munger != (GeomMunger *)NULL, false);
  Thread *current_thread = traverser->get_current_thread();
  PStatTimer timer(_munge_geom_pcollector, current_thread);
  if (_geom != (Geom *)NULL) {
    _munger = munger;

    int geom_rendering;

    {
      GeomPipelineReader geom_reader(_geom, current_thread);
      _munged_data = geom_reader.get_vertex_data();
      
#ifdef _DEBUG
      {
        GeomVertexDataPipelineReader data_reader(_munged_data, current_thread);
        data_reader.check_array_readers();
        nassertr(geom_reader.check_valid(&data_reader), false);
      }
#endif  // _DEBUG
      
      geom_rendering = geom_reader.get_geom_rendering();
      geom_rendering = _state->get_geom_rendering(geom_rendering);
      geom_rendering = _modelview_transform->get_geom_rendering(geom_rendering);
      
      if (geom_rendering & Geom::GR_point_bits) {
        if (geom_reader.get_primitive_type() != Geom::PT_points) {
          if (singular_points) {
            // Isolate the points so there's no unneeded overlap.
            _geom = _geom->make_points();
          }
        }
      }
    }
    
    GraphicsStateGuardianBase *gsg = traverser->get_gsg();
    int gsg_bits = gsg->get_supported_geom_rendering();
    if (!hardware_point_sprites) {
      // If support for hardware point sprites or perspective-scaled
      // points is disabled, we don't allow the GSG to tell us it
      // supports them.
      gsg_bits &= ~(Geom::GR_point_perspective | Geom::GR_point_sprite);
    }
    if (!hardware_points) {
      // If hardware-points is off, we don't allow any kind of point
      // rendering, except plain old one-pixel points;
      gsg_bits &= ~(Geom::GR_point_bits & ~Geom::GR_point);
    }
    int unsupported_bits = geom_rendering & ~gsg_bits;

    if ((unsupported_bits & Geom::GR_point_bits) != 0) {
      // The GSG doesn't support rendering these fancy points
      // directly; we have to render them in software instead.
      // Munge them into quads.  This will replace the _geom and
      // _munged_data, and might also replace _state.
      if (pgraph_cat.is_spam()) {
        pgraph_cat.spam()
          << "munge_points_to_quads() for geometry with bits: " 
          << hex << geom_rendering << ", unsupported: "
          << (unsupported_bits & Geom::GR_point_bits) << dec << "\n";
      }
      if (!munge_points_to_quads(traverser, force)) {
        return false;
      }
    }

    bool cpu_animated = false;

    if (unsupported_bits & Geom::GR_texcoord_light_vector) {
      // If we have to compute the light vector, we have to animate
      // the vertices in the CPU--and we have to do it before we call
      // munge_geom(), which might lose the tangent and binormal.
      CPT(GeomVertexData) animated_vertices = 
        _munged_data->animate_vertices(force, current_thread);
      if (animated_vertices != _munged_data) {
        cpu_animated = true;
        _munged_data = animated_vertices;
      }
      if (!munge_texcoord_light_vector(traverser, force)) {
        return false;
      }
    }

    // Now invoke the munger to ensure the resulting geometry is in
    // a GSG-friendly form.
    if (!munger->munge_geom(_geom, _munged_data, force, current_thread)) {
      return false;
    }

    StateMunger *state_munger;
    DCAST_INTO_R(state_munger, munger, false);
    _state = state_munger->munge_state(_state);

    if (!cpu_animated) {
      // If there is any animation left in the vertex data after it
      // has been munged--that is, we couldn't arrange to handle the
      // animation in hardware--then we have to calculate that
      // animation now.
      CPT(GeomVertexData) animated_vertices = 
        _munged_data->animate_vertices(force, current_thread);
      if (animated_vertices != _munged_data) {
        cpu_animated = true;
        _munged_data = animated_vertices;
      }
    }

#ifndef NDEBUG
    if (show_vertex_animation) {
      GeomVertexDataPipelineReader data_reader(_munged_data, current_thread);
      bool hardware_animated = (data_reader.get_format()->get_animation().get_animation_type() == Geom::AT_hardware);
      if (cpu_animated || hardware_animated) {
        // These vertices were animated, so flash them red or blue.
        static const double flash_rate = 1.0;  // 1 state change per second
        int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * flash_rate);
        if ((cycle & 1) == 0) {
          _state = cpu_animated ? get_flash_cpu_state() : get_flash_hardware_state();
        }
      }
    }
#endif
  }

  if (_fancy) {
    // Only check the _next pointer if the _fancy flag is set.
    if (_next != (CullableObject *)NULL) {
      if (_next->_state != (RenderState *)NULL) {
        _next->munge_geom(gsg, gsg->get_geom_munger(_next->_state, current_thread),
                          traverser, force);
      } else {
        _next->munge_geom(gsg, munger, traverser, force);
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::Destructor
//       Access: Public
//  Description: Automatically deletes the whole chain of these things.
////////////////////////////////////////////////////////////////////
CullableObject::
~CullableObject() {
  if (_fancy) {
    // Only check the _next pointer if the _fancy flag is set.
    if (_next != (CullableObject *)NULL) {
      delete _next;
    }
    set_draw_callback(NULL);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::output
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void CullableObject::
output(ostream &out) const {
  if (_geom != (Geom *)NULL) {
    out << *_geom;
  } else {
    out << "(null)";
  }
}


////////////////////////////////////////////////////////////////////
//     Function: CullableObject::munge_points_to_quads
//       Access: Private
//  Description: Converts a table of points to quads for rendering on
//               systems that don't support fancy points.
//
//               This may replace _geom, _munged_data, and _state.
////////////////////////////////////////////////////////////////////
bool CullableObject::
munge_points_to_quads(const CullTraverser *traverser, bool force) {
  Thread *current_thread = traverser->get_current_thread();

  // Better get the animated vertices, in case we're showing sprites
  // on an animated model for some reason.
  CPT(GeomVertexData) source_data = 
    _munged_data->animate_vertices(force, current_thread);

  if (!force && !source_data->request_resident()) {
    return false;
  }

  PStatTimer timer(_munge_sprites_pcollector, current_thread);
  _sw_sprites_pcollector.add_level(source_data->get_num_rows());

  GraphicsStateGuardianBase *gsg = traverser->get_gsg();

  GeomVertexReader vertex(source_data, InternalName::get_vertex(),
                          current_thread);
  GeomVertexReader normal(source_data, InternalName::get_normal(),
                          current_thread);
  GeomVertexReader color(source_data, InternalName::get_color(),
                         current_thread);
  GeomVertexReader texcoord(source_data, InternalName::get_texcoord(),
                            current_thread);
  GeomVertexReader rotate(source_data, InternalName::get_rotate(),
                          current_thread);
  GeomVertexReader size(source_data, InternalName::get_size(),
                        current_thread);
  GeomVertexReader aspect_ratio(source_data, InternalName::get_aspect_ratio(),
                                current_thread);

  bool has_normal = (normal.has_column());
  bool has_color = (color.has_column());
  bool has_texcoord = (texcoord.has_column());
  bool has_rotate = (rotate.has_column());
  bool has_size = (size.has_column());
  bool has_aspect_ratio = (aspect_ratio.has_column());

  bool sprite_texcoord = false;
  const TexGenAttrib *tex_gen = DCAST(TexGenAttrib, _state->get_attrib(TexGenAttrib::get_class_slot()));
  if (tex_gen != (TexGenAttrib *)NULL) {
    if (tex_gen->get_mode(TextureStage::get_default()) == TexGenAttrib::M_point_sprite) {
      sprite_texcoord = true;

      // Turn off the TexGenAttrib, since we don't want it now.
      _state = _state->set_attrib(tex_gen->remove_stage(TextureStage::get_default()));
    }
  }

  float point_size = 1.0f;
  bool perspective = false;
  const RenderModeAttrib *render_mode = DCAST(RenderModeAttrib, _state->get_attrib(RenderModeAttrib::get_class_slot()));
  if (render_mode != (RenderModeAttrib *)NULL) {
    point_size = render_mode->get_thickness();
    perspective = render_mode->get_perspective();

    if (render_mode->get_mode() != RenderModeAttrib::M_filled_flat) {
      // Render the new polygons with M_filled_flat, for a slight
      // performance advantage when software rendering.
      _state = _state->set_attrib(RenderModeAttrib::make(RenderModeAttrib::M_filled_flat));
    }
  }

  // Get the vertex format of the newly created geometry.
  CPT(GeomVertexFormat) new_format;

  {
    LightMutexHolder holder(_format_lock);
    SourceFormat sformat(source_data->get_format(), sprite_texcoord);
    FormatMap::iterator fmi = _format_map.find(sformat);
    if (fmi != _format_map.end()) {
      new_format = (*fmi).second;
      
    } else {
      // We have to construct the format now.
      PT(GeomVertexArrayFormat) new_array_format;
      if (sformat._retransform_sprites) {
        // With retransform_sprites in effect, we will be sending ordinary
        // 3-D points to the graphics API.
        new_array_format = 
          new GeomVertexArrayFormat(InternalName::get_vertex(), 3, 
                                    Geom::NT_float32,
                                    Geom::C_point);
      } else {
        // Without retransform_sprites, we will be sending 4-component
        // clip-space points.
        new_array_format = 
          new GeomVertexArrayFormat(InternalName::get_vertex(), 4, 
                                    Geom::NT_float32,
                                    Geom::C_clip_point);
      }
      if (has_normal) {
        const GeomVertexColumn *c = normal.get_column();
        new_array_format->add_column
          (InternalName::get_normal(), c->get_num_components(),
           c->get_numeric_type(), c->get_contents());
      }
      if (has_color) {
        const GeomVertexColumn *c = color.get_column();
        new_array_format->add_column
          (InternalName::get_color(), c->get_num_components(),
           c->get_numeric_type(), c->get_contents());
      }
      if (sprite_texcoord) {
        new_array_format->add_column
          (InternalName::get_texcoord(), 2,
           Geom::NT_float32,
           Geom::C_texcoord);
        
      } else if (has_texcoord) {
        const GeomVertexColumn *c = texcoord.get_column();
        new_array_format->add_column
          (InternalName::get_texcoord(), c->get_num_components(),
           c->get_numeric_type(), c->get_contents());
      }
      
      new_format = GeomVertexFormat::register_format(new_array_format);
      _format_map[sformat] = new_format;
    }
  }

  const LMatrix4f &modelview = _modelview_transform->get_mat();

  SceneSetup *scene = traverser->get_scene();
  const Lens *lens = scene->get_lens();
  const LMatrix4f &projection = lens->get_projection_mat();

  int viewport_width = scene->get_viewport_width();
  int viewport_height = scene->get_viewport_height();

  // We need a standard projection matrix, in a known coordinate
  // system, to compute the perspective height.
  LMatrix4f height_projection;
  if (perspective) {
    height_projection =
      LMatrix4f::convert_mat(CS_yup_right, lens->get_coordinate_system()) *
      projection;
  }

  LMatrix4f render_transform = modelview * projection;
  LMatrix4f inv_render_transform;
  inv_render_transform.invert_from(render_transform);

  // Now convert all of the vertices in the GeomVertexData to quads.
  // We always convert all the vertices, assuming all the vertices are
  // referenced by GeomPrimitives, because we want to optimize for the
  // most common case.
  int orig_verts = source_data->get_num_rows();
  int new_verts = 4 * orig_verts;        // each vertex becomes four.

  PT(GeomVertexData) new_data = new GeomVertexData
    (source_data->get_name(), new_format, Geom::UH_stream);
  new_data->unclean_set_num_rows(new_verts);

  GeomVertexWriter new_vertex(new_data, InternalName::get_vertex());
  GeomVertexWriter new_normal(new_data, InternalName::get_normal());
  GeomVertexWriter new_color(new_data, InternalName::get_color());
  GeomVertexWriter new_texcoord(new_data, InternalName::get_texcoord());

  // We'll keep an array of all of the points' eye-space coordinates,
  // and their distance from the camera, so we can sort the points for
  // each primitive, below.
  PointData *points;
  {
    PStatTimer t2(_munge_sprites_verts_pcollector, current_thread);
    points = (PointData *)alloca(orig_verts * sizeof(PointData));
    int vi = 0;
    while (!vertex.is_at_end()) {
      // Get the point in eye-space coordinates.
      LPoint3f eye = modelview.xform_point(vertex.get_data3f());
      points[vi]._eye = eye;
      points[vi]._dist = gsg->compute_distance_to(points[vi]._eye);
    
      // The point in clip coordinates.
      LPoint4f p4 = LPoint4f(eye[0], eye[1], eye[2], 1.0f) * projection;

      if (has_size) {
        point_size = size.get_data1f();
      }

      float scale_y = point_size;
      if (perspective) {
        // Perspective-sized points.  Here point_size is the point's
        // height in 3-d units.  To arrange that, we need to figure out
        // the appropriate scaling factor based on the current viewport
        // and projection matrix.
        float scale = _modelview_transform->get_scale()[1];
        LVector3f height(0.0f, point_size * scale, scale);
        height = height * height_projection;
        scale_y = height[1] * viewport_height;

        // We should then divide the radius by the distance from the
        // camera plane, to emulate the glPointParameters() behavior.
        if (!lens->is_orthographic()) {
          scale_y /= gsg->compute_distance_to(eye);
        }
      }
      
      // Also factor in the homogeneous scale for being in clip
      // coordinates still.
      scale_y *= p4[3];

      float scale_x = scale_y;
      if (has_aspect_ratio) {
        scale_x *= aspect_ratio.get_data1f();
      }

      // Define the first two corners based on the scales in X and Y.
      LPoint2f c0(scale_x, scale_y);
      LPoint2f c1(-scale_x, scale_y);

      if (has_rotate) { 
        // If we have a rotate factor, apply it to those two corners.
        float r = rotate.get_data1f();
        LMatrix3f mat = LMatrix3f::rotate_mat(r);
        c0 = c0 * mat;
        c1 = c1 * mat;
      }

      // Finally, scale the corners in their newly-rotated position,
      // to compensate for the aspect ratio of the viewport.
      float rx = 1.0f / viewport_width;
      float ry = 1.0f / viewport_height;
      c0.set(c0[0] * rx, c0[1] * ry);
      c1.set(c1[0] * rx, c1[1] * ry);
    
      if (retransform_sprites) {
        // With retransform_sprites in effect, we must reconvert the
        // resulting quad back into the original 3-D space.
        new_vertex.set_data4f(inv_render_transform.xform(LPoint4f(p4[0] + c0[0], p4[1] + c0[1], p4[2], p4[3])));
        new_vertex.set_data4f(inv_render_transform.xform(LPoint4f(p4[0] + c1[0], p4[1] + c1[1], p4[2], p4[3])));
        new_vertex.set_data4f(inv_render_transform.xform(LPoint4f(p4[0] - c1[0], p4[1] - c1[1], p4[2], p4[3])));
        new_vertex.set_data4f(inv_render_transform.xform(LPoint4f(p4[0] - c0[0], p4[1] - c0[1], p4[2], p4[3])));
      
        if (has_normal) {
          const Normalf &c = normal.get_data3f();
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
        }
      
      } else {
        // Without retransform_sprites, we can simply load the
        // clip-space coordinates.
        new_vertex.set_data4f(p4[0] + c0[0], p4[1] + c0[1], p4[2], p4[3]);
        new_vertex.set_data4f(p4[0] + c1[0], p4[1] + c1[1], p4[2], p4[3]);
        new_vertex.set_data4f(p4[0] - c1[0], p4[1] - c1[1], p4[2], p4[3]);
        new_vertex.set_data4f(p4[0] - c0[0], p4[1] - c0[1], p4[2], p4[3]);
      
        if (has_normal) {
          Normalf c = render_transform.xform_vec(normal.get_data3f());
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
          new_normal.set_data3f(c);
        }
      }
      if (has_color) {
        const Colorf &c = color.get_data4f();
        new_color.set_data4f(c);
        new_color.set_data4f(c);
        new_color.set_data4f(c);
        new_color.set_data4f(c);
      }
      if (sprite_texcoord) {
        new_texcoord.set_data2f(1.0f, 0.0f);
        new_texcoord.set_data2f(0.0f, 0.0f);
        new_texcoord.set_data2f(1.0f, 1.0f);
        new_texcoord.set_data2f(0.0f, 1.0f);
      } else if (has_texcoord) {
        const LVecBase4f &c = texcoord.get_data4f();
        new_texcoord.set_data4f(c);
        new_texcoord.set_data4f(c);
        new_texcoord.set_data4f(c);
        new_texcoord.set_data4f(c);
      }

      ++vi;
    }

    nassertr(vi == orig_verts, false);
    nassertr(new_data->get_num_rows() == new_verts, false);
  }

  PT(Geom) new_geom = new Geom(new_data);
    
  // Create an appropriate GeomVertexArrayFormat for the primitive
  // index.
  static CPT(GeomVertexArrayFormat) new_prim_format;
  if (new_prim_format == (GeomVertexArrayFormat *)NULL) {
    new_prim_format =
      GeomVertexArrayFormat::register_format
      (new GeomVertexArrayFormat(InternalName::get_index(), 1, 
                                 GeomEnums::NT_uint16, GeomEnums::C_index));
  }

  // Replace each primitive in the Geom (it's presumably a GeomPoints
  // primitive, although it might be some other kind of primitive if
  // we got here because RenderModeAttrib::M_point is enabled) with a
  // new primitive that replaces each vertex with a quad of the
  // appropriate scale and orientation.

  // BUG: if we're rendering polygons in M_point mode with a
  // CullFaceAttrib in effect, we won't actually apply the
  // CullFaceAttrib but will always render all of the vertices of the
  // polygons.  This is certainly a bug, but a very minor one; and in
  // order to fix it we'd have to do the face culling ourselves--not
  // sure if it's worth it.

  {
    PStatTimer t3(_munge_sprites_prims_pcollector, current_thread);
    GeomPipelineReader geom_reader(_geom, current_thread);
    int num_primitives = geom_reader.get_num_primitives();
    for (int pi = 0; pi < num_primitives; ++pi) {
      const GeomPrimitive *primitive = geom_reader.get_primitive(pi);
      if (primitive->get_num_vertices() != 0) {
        // Extract out the list of vertices referenced by the primitive.
        int num_vertices = primitive->get_num_vertices();
        unsigned int *vertices = (unsigned int *)alloca(num_vertices * sizeof(unsigned int));
        unsigned int *vertices_end = vertices + num_vertices;

        if (primitive->is_indexed()) {
          // Indexed case.
          GeomVertexReader index(primitive->get_vertices(), 0, current_thread);
          for (unsigned int *vi = vertices; vi != vertices_end; ++vi) {
            unsigned int v = index.get_data1i();
            nassertr(v < (unsigned int)orig_verts, false);
            (*vi) = v;
          }
        } else {
          // Nonindexed case.
          unsigned int first_vertex = primitive->get_first_vertex();
          for (int i = 0; i < num_vertices; ++i) {
            unsigned int v = i + first_vertex;
            nassertr(v < (unsigned int)orig_verts, false);
            vertices[i] = v;
          }
        }
  
        // Now sort the points in order from back-to-front so they will
        // render properly with transparency, at least with each other.
        sort(vertices, vertices_end, SortPoints(points));
  
        // Go through the points, now in sorted order, and generate a pair
        // of triangles for each one.  We generate indexed triangles
        // instead of two-triangle strips, since this seems to be
        // generally faster on PC hardware (otherwise, we'd have to nearly
        // double the vertices to stitch all the little triangle strips
        // together).
        PT(GeomPrimitive) new_primitive = new GeomTriangles(Geom::UH_stream);
        int new_prim_verts = 6 * num_vertices;  // two triangles per point.

        PT(GeomVertexArrayData) new_index 
          = new GeomVertexArrayData(new_prim_format, GeomEnums::UH_stream);
        new_index->unclean_set_num_rows(new_prim_verts);

        GeomVertexWriter index(new_index, 0);
        nassertr(index.has_column(), false);
        for (unsigned int *vi = vertices; vi != vertices_end; ++vi) {
          int new_vi = (*vi) * 4;
          nassertr(index.get_write_row() + 6 <= new_prim_verts, false);
          index.set_data1i(new_vi);
          index.set_data1i(new_vi + 1);
          index.set_data1i(new_vi + 2);
          index.set_data1i(new_vi + 2);
          index.set_data1i(new_vi + 1);
          index.set_data1i(new_vi + 3);
        }
        new_primitive->set_vertices(new_index, new_prim_verts);

        int min_vi = primitive->get_min_vertex();
        int max_vi = primitive->get_max_vertex();
        new_primitive->set_minmax(min_vi * 4, max_vi * 4 + 3, NULL, NULL);

        new_geom->add_primitive(new_primitive);
      }
    }
  }

  _geom = new_geom.p();
  _munged_data = new_data;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::munge_texcoord_light_vector
//       Access: Private
//  Description: Generates the vector from each vertex to the
//               indicated light as a 3-d texture coordinate.
//
//               This may replace _geom, _munged_data, and _state.
////////////////////////////////////////////////////////////////////
bool CullableObject::
munge_texcoord_light_vector(const CullTraverser *traverser, bool force) {
  Thread *current_thread = traverser->get_current_thread();
  PStatTimer timer(_munge_light_vector_pcollector, current_thread);

  if (_net_transform->is_singular()) {
    // If we're under a singular transform, never mind.
    return true;
  }

  if (!_munged_data->has_column(InternalName::get_vertex()) || 
      !_munged_data->has_column(InternalName::get_normal())) {
    // No vertex or normal; can't compute light vector.
    return true;
  }

  CPT(TexGenAttrib) tex_gen = DCAST(TexGenAttrib, _state->get_attrib(TexGenAttrib::get_class_slot()));
  nassertr(tex_gen != (TexGenAttrib *)NULL, false);

  const TexGenAttrib::LightVectors &light_vectors = tex_gen->get_light_vectors();
  TexGenAttrib::LightVectors::const_iterator lvi;
  for (lvi = light_vectors.begin();
       lvi != light_vectors.end();
       ++lvi) {
    TextureStage *stage = (*lvi);
    NodePath light = tex_gen->get_light(stage);
    if (light.is_empty()) {
      // If a particular light isn't specified in the TexGenAttrib,
      // use the most important light in the current state.
      CPT(RenderAttrib) attrib = _state->get_attrib(LightAttrib::get_class_slot());
      if (attrib != (RenderAttrib *)NULL) {
        CPT(LightAttrib) la = DCAST(LightAttrib, attrib);
        light = la->get_most_important_light();
      }
    }
    if (!light.is_empty()) {
      string source_name = tex_gen->get_source_name(stage);
      Light *light_obj = light.node()->as_light();
      nassertr(light_obj != (Light *)NULL, false);
      
      // Determine the names of the tangent and binormal columns
      // associated with the stage's texcoord name.
      PT(InternalName) tangent_name = InternalName::get_tangent_name(source_name);
      PT(InternalName) binormal_name = InternalName::get_binormal_name(source_name);
      
      PT(InternalName) texcoord_name = stage->get_texcoord_name();
      
      if (_munged_data->has_column(tangent_name) &&
          _munged_data->has_column(binormal_name)) {

        if (!force && !_munged_data->request_resident()) {
          // The data isn't resident; give up.
          return false;
        }

        // Create a new column for the new texcoords.
        PT(GeomVertexData) new_data = _munged_data->replace_column
          (texcoord_name, 3, Geom::NT_float32, Geom::C_texcoord);
        _munged_data = new_data;

        // Remove this TexGen stage from the state, since we're handling
        // it now.
        _state = _state->add_attrib(tex_gen->remove_stage(stage));

        // Get the transform from the light to the object.
        CPT(TransformState) light_transform =
          _net_transform->invert_compose(light.get_net_transform());
        const LMatrix4f &light_mat = light_transform->get_mat();

        GeomVertexWriter texcoord(new_data, texcoord_name, current_thread);
        GeomVertexReader vertex(new_data, InternalName::get_vertex(),
                                current_thread);
        GeomVertexReader tangent(new_data, tangent_name, current_thread);
        GeomVertexReader binormal(new_data, binormal_name, current_thread);
        GeomVertexReader normal(new_data, InternalName::get_normal(),
                                current_thread);

        while (!vertex.is_at_end()) {
          LPoint3f p = vertex.get_data3f();
          LVector3f t = tangent.get_data3f();
          LVector3f b = binormal.get_data3f();
          LVector3f n = normal.get_data3f();

          LVector3f lv;
          if (light_obj->get_vector_to_light(lv, p, light_mat)) {
            texcoord.add_data3f(lv.dot(t), lv.dot(b), lv.dot(n));
          }
        }
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::get_flash_cpu_state
//       Access: Private, Static
//  Description: Returns a RenderState for flashing the object red, to
//               show it is animated by the CPU when
//               show-vertex-animation is on.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullableObject::
get_flash_cpu_state() {
  static const Colorf flash_cpu_color(0.8f, 0.2f, 0.2f, 1.0f);

  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) flash_cpu_state = (const RenderState *)NULL;
  if (flash_cpu_state == (const RenderState *)NULL) {
    flash_cpu_state = RenderState::make
      (LightAttrib::make_all_off(),
       TextureAttrib::make_off(),
       ColorAttrib::make_flat(flash_cpu_color));
  }

  return flash_cpu_state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::get_flash_hardware_state
//       Access: Private, Static
//  Description: Returns a RenderState for flashing the object blue,
//               to show it is animated by the hardware when
//               show-vertex-animation is on.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullableObject::
get_flash_hardware_state() {
  static const Colorf flash_hardware_color(0.2f, 0.2f, 0.8f, 1.0f);

  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) flash_hardware_state = (const RenderState *)NULL;
  if (flash_hardware_state == (const RenderState *)NULL) {
    flash_hardware_state = RenderState::make
      (LightAttrib::make_all_off(),
       TextureAttrib::make_off(),
       ColorAttrib::make_flat(flash_hardware_color));
  }

  return flash_hardware_state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::draw_fancy
//       Access: Private
//  Description: Something fancy about this object.  Draw it properly.
////////////////////////////////////////////////////////////////////
void CullableObject::
draw_fancy(GraphicsStateGuardianBase *gsg, bool force, 
           Thread *current_thread) {
  nassertv(_fancy);
  if (_draw_callback != (CallbackObject *)NULL) {
    // It has a callback associated.
    gsg->clear_before_callback();
    gsg->set_state_and_transform(_state, _internal_transform);
    GeomDrawCallbackData cbdata(this, gsg, force);
    _draw_callback->do_callback(&cbdata);
    if (cbdata.get_lost_state()) {
      // Tell the GSG to forget its state.
      gsg->clear_state_and_transform();
    }
    // Now the callback has taken care of drawing.

  } else if (_next != (CullableObject *)NULL) {
    // It has decals.
    draw_with_decals(gsg, force, current_thread);

  } else {
    // Huh, nothing fancy after all.  Somehow the _fancy flag got set
    // incorrectly; that's a bug.
    gsg->set_state_and_transform(_state, _internal_transform);
    draw_inline(gsg, force, current_thread);
    nassertv(false);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::draw_with_decals
//       Access: Private
//  Description: Draws the current CullableObject, assuming it has
//               attached decals.
////////////////////////////////////////////////////////////////////
void CullableObject::
draw_with_decals(GraphicsStateGuardianBase *gsg, bool force, 
                 Thread *current_thread) {
  nassertv(_fancy && _next != (CullableObject *)NULL);  
  // We draw with a three-step process.

  // First, render all of the base geometry for the first pass.
  CPT(RenderState) state = gsg->begin_decal_base_first();

  CullableObject *base = this;
  while (base != (CullableObject *)NULL && base->_geom != (Geom *)NULL) {
    gsg->set_state_and_transform(base->_state->compose(state), base->_internal_transform);
    base->draw_inline(gsg, force, current_thread);
    
    base = base->_next;
  }

  if (base != (CullableObject *)NULL) {
    // Now, draw all the decals.
    state = gsg->begin_decal_nested();

    CullableObject *decal = base->_next;
    while (decal != (CullableObject *)NULL) {
      gsg->set_state_and_transform(decal->_state->compose(state), decal->_internal_transform);
      decal->draw_inline(gsg, force, current_thread);
      decal = decal->_next;
    }
  }

  // And now, re-draw the base geometry, if required.
  state = gsg->begin_decal_base_second();
  if (state != (const RenderState *)NULL) {
    base = this;
    while (base != (CullableObject *)NULL && base->_geom != (Geom *)NULL) {
      gsg->set_state_and_transform(base->_state->compose(state), base->_internal_transform);
      base->draw_inline(gsg, force, current_thread);
      
      base = base->_next;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullableObject::SourceFormat::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
CullableObject::SourceFormat::
SourceFormat(const GeomVertexFormat *format, bool sprite_texcoord) :
  _format(format),
  _sprite_texcoord(sprite_texcoord) 
{
  _retransform_sprites = retransform_sprites;
}
