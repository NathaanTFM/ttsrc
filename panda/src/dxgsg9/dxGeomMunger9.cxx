// Filename: dxGeomMunger9.cxx
// Created by:  drose (11Mar05)
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

#include "dxGeomMunger9.h"
#include "geomVertexReader.h"
#include "geomVertexWriter.h"
#include "config_dxgsg9.h"

GeomMunger *DXGeomMunger9::_deleted_chain = NULL;
TypeHandle DXGeomMunger9::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
DXGeomMunger9::
~DXGeomMunger9() {
  if (_reffed_filtered_texture) {
    unref_delete(_filtered_texture);
    _reffed_filtered_texture = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::wp_callback
//       Access: Public, Virtual
//  Description: This callback is set to be made whenever the
//               associated _texture or _tex_gen attributes are
//               destructed, in which case the GeomMunger is invalid
//               and should no longer be used.
////////////////////////////////////////////////////////////////////
void DXGeomMunger9::
wp_callback(void *) {
  unregister_myself();

  if (_reffed_filtered_texture) {
    unref_delete(_filtered_texture);
    _reffed_filtered_texture = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::munge_format_impl
//       Access: Protected, Virtual
//  Description: Given a source GeomVertexFormat, converts it if
//               necessary to the appropriate format for rendering.
////////////////////////////////////////////////////////////////////
CPT(GeomVertexFormat) DXGeomMunger9::
munge_format_impl(const GeomVertexFormat *orig,
                  const GeomVertexAnimationSpec &animation) {
  if (dxgsg9_cat.is_debug()) {
    if (animation.get_animation_type() != AT_none) {
      dxgsg9_cat.debug()
        << "preparing animation type " << animation << " for " << *orig
        << "\n";
    }
  }
  // We have to build a completely new format that includes only the
  // appropriate components, in the appropriate order, in just one
  // array.
  PT(GeomVertexFormat) new_format = new GeomVertexFormat(*orig);
  new_format->set_animation(animation);
  PT(GeomVertexArrayFormat) new_array_format = new GeomVertexArrayFormat;

  const GeomVertexColumn *vertex_type = orig->get_vertex_column();
  const GeomVertexColumn *normal_type = orig->get_normal_column();
  const GeomVertexColumn *color_type = orig->get_color_column();

  if (vertex_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_vertex(), 3, NT_float32,
       vertex_type->get_contents());
    new_format->remove_column(vertex_type->get_name());

  } else {
    // If we don't have a vertex type, not much we can do.
    return orig;
  }

  if (animation.get_animation_type() == AT_hardware &&
      animation.get_num_transforms() > 0) {
    if (animation.get_num_transforms() > 1) {
      // If we want hardware animation, we need to reserve space for the
      // blend weights.
      new_array_format->add_column
        (InternalName::get_transform_weight(), animation.get_num_transforms() - 1,
         NT_float32, C_other);
    }

    if (animation.get_indexed_transforms()) {
      // Also, if we'll be indexing into the transform table, reserve
      // space for the index.
      new_array_format->add_column
        (InternalName::get_transform_index(), 1,
         NT_packed_dcba, C_index);
    }

    // Make sure the old weights and indices are removed, just in
    // case.
    new_format->remove_column(InternalName::get_transform_weight());
    new_format->remove_column(InternalName::get_transform_index());

    // And we don't need the transform_blend table any more.
    new_format->remove_column(InternalName::get_transform_blend());
  }

  if (normal_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_normal(), 3, NT_float32, C_vector);
    new_format->remove_column(normal_type->get_name());
  }

  if (color_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_color(), 1, NT_packed_dabc, C_color);
    new_format->remove_column(color_type->get_name());
  }

  // To support multitexture, we will need to add all of the relevant
  // texcoord types, and in the order specified by the TextureAttrib.

  // Now set up each of the active texture coordinate stages--or at
  // least those for which we're not generating texture coordinates
  // automatically.

  if (_filtered_texture != (TextureAttrib *)NULL) {
    int num_stages = _filtered_texture->get_num_on_ff_stages();
    vector_int ff_tc_index(num_stages, 0);

    // Be sure we add the texture coordinates in the right order, as
    // specified by the attrib.  To ensure this, we first walk through
    // the stages of the attrib and get the index numbers in the
    // appropriate order.
    int si, tc_index;
    int max_tc_index = -1;
    for (si = 0; si < num_stages; ++si) {
      int tc_index = _filtered_texture->get_ff_tc_index(si);
      nassertr(tc_index < num_stages, orig);
      ff_tc_index[tc_index] = si;
      max_tc_index = max(tc_index, max_tc_index);
    }

    // Now walk through the texture coordinates in the order they will
    // appear on the final geometry.  For each one, get the texture
    // coordinate name from the associated stage.
    for (tc_index = 0; tc_index <= max_tc_index; ++tc_index) {
      si = ff_tc_index[tc_index];
      TextureStage *stage = _filtered_texture->get_on_ff_stage(si);
      InternalName *name = stage->get_texcoord_name();

      const GeomVertexColumn *texcoord_type = orig->get_column(name);

      if (texcoord_type != (const GeomVertexColumn *)NULL) {
        new_array_format->add_column
          (name, texcoord_type->get_num_values(), NT_float32, C_texcoord);
      } else {
        // We have to add something as a placeholder, even if the
        // texture coordinates aren't defined.
        new_array_format->add_column(name, 2, NT_float32, C_texcoord);
      }
      new_format->remove_column(name);
    }
  }

  // Make sure the FVF-style array we just built up is first in the
  // list.
  new_format->insert_array(0, new_array_format);

  return GeomVertexFormat::register_format(new_format);
}

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::premunge_format_impl
//       Access: Protected, Virtual
//  Description: Given a source GeomVertexFormat, converts it if
//               necessary to the appropriate format for rendering.
////////////////////////////////////////////////////////////////////
CPT(GeomVertexFormat) DXGeomMunger9::
premunge_format_impl(const GeomVertexFormat *orig) {
  // We have to build a completely new format that includes only the
  // appropriate components, in the appropriate order, in just one
  // array.
  PT(GeomVertexFormat) new_format = new GeomVertexFormat(*orig);
  PT(GeomVertexArrayFormat) new_array_format = new GeomVertexArrayFormat;

  const GeomVertexColumn *vertex_type = orig->get_vertex_column();
  const GeomVertexColumn *normal_type = orig->get_normal_column();
  const GeomVertexColumn *color_type = orig->get_color_column();

  if (vertex_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_vertex(), 3, NT_float32,
       vertex_type->get_contents());
    new_format->remove_column(vertex_type->get_name());

  } else {
    // If we don't have a vertex type, not much we can do.
    return orig;
  }

  if (normal_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_normal(), 3, NT_float32, C_vector);
    new_format->remove_column(normal_type->get_name());
  }

  if (color_type != (const GeomVertexColumn *)NULL) {
    new_array_format->add_column
      (InternalName::get_color(), 1, NT_packed_dabc, C_color);
    new_format->remove_column(color_type->get_name());
  }

  // To support multitexture, we will need to add all of the relevant
  // texcoord types, and in the order specified by the TextureAttrib.

  // Now set up each of the active texture coordinate stages--or at
  // least those for which we're not generating texture coordinates
  // automatically.

  if (_filtered_texture != (TextureAttrib *)NULL) {
    int num_stages = _filtered_texture->get_num_on_ff_stages();
    vector_int ff_tc_index(num_stages, 0);

    // Be sure we add the texture coordinates in the right order, as
    // specified by the attrib.  To ensure this, we first walk through
    // the stages of the attrib and get the index numbers in the
    // appropriate order.
    int si, tc_index;
    int max_tc_index = -1;
    for (si = 0; si < num_stages; ++si) {
      int tc_index = _filtered_texture->get_ff_tc_index(si);
      nassertr(tc_index < num_stages, orig);
      ff_tc_index[tc_index] = si;
      max_tc_index = max(tc_index, max_tc_index);
    }

    // Now walk through the texture coordinates in the order they will
    // appear on the final geometry.  For each one, get the texture
    // coordinate name from the associated stage.
    for (tc_index = 0; tc_index <= max_tc_index; ++tc_index) {
      si = ff_tc_index[tc_index];
      TextureStage *stage = _filtered_texture->get_on_ff_stage(si);
      InternalName *name = stage->get_texcoord_name();

      const GeomVertexColumn *texcoord_type = orig->get_column(name);

      if (texcoord_type != (const GeomVertexColumn *)NULL) {
        new_array_format->add_column
          (name, texcoord_type->get_num_values(), NT_float32, C_texcoord);
      } else {
        // We have to add something as a placeholder, even if the
        // texture coordinates aren't defined.
        new_array_format->add_column(name, 2, NT_float32, C_texcoord);
      }
      new_format->remove_column(name);
    }
  }

  // Make sure the FVF-style array we just built up is first in the
  // list.
  new_format->insert_array(0, new_array_format);

  return GeomVertexFormat::register_format(new_format);
}

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::compare_to_impl
//       Access: Protected, Virtual
//  Description: Called to compare two GeomMungers who are known to be
//               of the same type, for an apples-to-apples comparison.
//               This will never be called on two pointers of a
//               different type.
////////////////////////////////////////////////////////////////////
int DXGeomMunger9::
compare_to_impl(const GeomMunger *other) const {
  const DXGeomMunger9 *om = DCAST(DXGeomMunger9, other);
  if (_filtered_texture != om->_filtered_texture) {
    return _filtered_texture < om->_filtered_texture ? -1 : 1;
  }
  if (_tex_gen != om->_tex_gen) {
    return _tex_gen < om->_tex_gen ? -1 : 1;
  }

  return StandardMunger::compare_to_impl(other);
}

////////////////////////////////////////////////////////////////////
//     Function: DXGeomMunger9::geom_compare_to_impl
//       Access: Protected, Virtual
//  Description: Called to compare two GeomMungers who are known to be
//               of the same type, for an apples-to-apples comparison.
//               This will never be called on two pointers of a
//               different type.
////////////////////////////////////////////////////////////////////
int DXGeomMunger9::
geom_compare_to_impl(const GeomMunger *other) const {
  // Unlike GLGeomMunger, we do consider _filtered_texture and
  // _tex_gen important for this purpose, since they control the
  // number and order of texture coordinates we might put into the
  // FVF.
  const DXGeomMunger9 *om = DCAST(DXGeomMunger9, other);
  if (_filtered_texture != om->_filtered_texture) {
    return _filtered_texture < om->_filtered_texture ? -1 : 1;
  }
  if (_tex_gen != om->_tex_gen) {
    return _tex_gen < om->_tex_gen ? -1 : 1;
  }

  return StandardMunger::geom_compare_to_impl(other);
}
